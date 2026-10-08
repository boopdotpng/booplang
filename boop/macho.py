# minimal arm64 mach-o executable writer. you hand it machine code + read-only data, it hands you a runnable binary.
#
# layout (all of it is one r-x __TEXT segment, then __LINKEDIT for the symbol table + code signature):
#
#   file 0x0000  vm TEXT_BASE           mach header + load commands
#   file 0x1000  vm TEXT_BASE+0x1000    __text   your code (CODE_ADDR)
#                                       __const  your rodata, at code start + rodata_offset(len(code))
#
# code and rodata sit at fixed distances from each other, so reach rodata with pc-relative adr / adrp+add.
# dyld calls the entry like C main: w0 = argc, x1 = argv, return value in w0 is the exit code.
# there's no __DATA segment: everything mutable lives on the stack.
import os, struct, subprocess

TEXT_BASE = 0x100000000
CODE_OFF = 0x1000                 # room for header + load commands (+ the LC_CODE_SIGNATURE codesign inserts)
CODE_ADDR = TEXT_BASE + CODE_OFF
PAGE = 0x4000                     # arm64 macOS pages are 16K

def align(x:int, a:int) -> int: return (x + a - 1) // a * a
def rodata_offset(code_len:int) -> int:
  """distance from the first code byte to the first rodata byte"""
  return align(code_len, 16)

def _pad(name:str, base:int) -> tuple[int, bytes]:
  b = name.encode() + b"\0"
  size = align(base + len(b), 8)
  return size, b.ljust(size - base, b"\0")

def _segment(name:str, vmaddr:int, vmsize:int, fileoff:int, filesize:int, prot:int, sects:list[bytes]) -> bytes:
  return struct.pack("<II16sQQQQiiII", 0x19, 72 + 80*len(sects), name.encode(), vmaddr, vmsize, fileoff, filesize,
                     prot, prot, len(sects), 0) + b"".join(sects)

def _section(sect:str, seg:str, addr:int, size:int, off:int, align_pow2:int, flags:int) -> bytes:
  return struct.pack("<16s16sQQIIIIIIII", sect.encode(), seg.encode(), addr, size, off, align_pow2, 0, 0, flags, 0, 0, 0)

def build(code:bytes|list[int], rodata:bytes=b"", entry:int=0, symbols:dict[str, int]|None=None) -> bytes:
  """code: bytes or a list of 32-bit instruction words. entry, symbols: byte offsets into code.
  symbols are only for debugging (otool -tvV, lldb), nothing needs them to run."""
  if isinstance(code, list): code = struct.pack(f"<{len(code)}I", *code)
  assert len(code) % 4 == 0 and len(code) > 0, "code must be whole instructions"
  assert entry % 4 == 0 and 0 <= entry < len(code), "entry must point at an instruction in code"
  symbols = symbols or {}

  ro_off = CODE_OFF + rodata_offset(len(code))
  text_filesize = align(ro_off + len(rodata), PAGE)

  # __LINKEDIT: local symbols (nlist_64) + string table
  strtab, nlist = b"\0", b""
  for name, off in sorted(symbols.items(), key=lambda kv: kv[1]):
    nlist += struct.pack("<IBBHQ", len(strtab), 0x0e, 1, 0, CODE_ADDR + off)  # N_SECT, local, section 1 (__text)
    strtab += b"_" + name.encode() + b"\0"
  strtab = strtab.ljust(align(len(strtab), 8), b"\0")
  linkedit = nlist + strtab
  le_off = text_filesize

  sects = [_section("__text", "__TEXT", CODE_ADDR, len(code), CODE_OFF, 2, 0x80000400)]  # pure + some instructions
  if rodata: sects.append(_section("__const", "__TEXT", TEXT_BASE + ro_off, len(rodata), ro_off, 4, 0))
  dylinker_size, dylinker = _pad("/usr/lib/dyld", 12)
  dylib_size, dylib = _pad("/usr/lib/libSystem.B.dylib", 24)
  cmds = [
    _segment("__PAGEZERO", 0, TEXT_BASE, 0, 0, 0, []),
    _segment("__TEXT", TEXT_BASE, text_filesize, 0, text_filesize, 5, sects),                     # r-x
    _segment("__LINKEDIT", TEXT_BASE + text_filesize, align(len(linkedit), PAGE), le_off, len(linkedit), 1, []),  # r--
    struct.pack("<IIIIII", 0x2, 24, le_off, len(symbols), le_off + len(nlist), len(strtab)),     # LC_SYMTAB
    struct.pack("<II18I", 0xb, 80, 0, len(symbols), *[0]*16),                                    # LC_DYSYMTAB
    struct.pack("<III", 0xe, dylinker_size, 12) + dylinker,                                      # LC_LOAD_DYLINKER
    struct.pack("<IIIIII", 0x32, 24, 1, 11 << 16, 11 << 16, 0),                                  # LC_BUILD_VERSION macos 11
    struct.pack("<IIQQ", 0x80000028, 24, CODE_OFF + entry, 0),                                   # LC_MAIN
    struct.pack("<IIIIII", 0xc, dylib_size, 24, 2, 0x10000, 0x10000) + dylib,                    # LC_LOAD_DYLIB libSystem
  ]
  # MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE
  header = struct.pack("<IiiIIIII", 0xfeedfacf, 0x0100000c, 0, 2, len(cmds), sum(map(len, cmds)), 0x1 | 0x4 | 0x80 | 0x200000, 0)
  head = header + b"".join(cmds)
  assert len(head) + 32 <= CODE_OFF, "load commands overflowed the header pad"

  out = bytearray(text_filesize)
  out[:len(head)] = head
  out[CODE_OFF:CODE_OFF+len(code)] = code
  out[ro_off:ro_off+len(rodata)] = rodata
  return bytes(out) + linkedit

def write_executable(path:str, code:bytes|list[int], rodata:bytes=b"", entry:int=0, symbols:dict[str, int]|None=None) -> str:
  """build, write, chmod +x, ad-hoc sign (arm64 macOS kills unsigned binaries). returns path."""
  with open(path, "wb") as f: f.write(build(code, rodata, entry, symbols))
  os.chmod(path, 0o755)
  subprocess.run(["codesign", "-s", "-", "-f", path], check=True, capture_output=True)
  return path
