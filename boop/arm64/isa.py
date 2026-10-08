# the arm64 instructions boop needs, their groups, and their encodings (INS -> 32-bit word).
# encodings are checked against the system assembler in test/test_isa.py.
#
# every instruction is 4 bytes. registers are ints 0..31: x/w regs for int ops, v regs (as h/s/d) for float ops.
# 31 means xzr/wzr in most int ops, and sp in ADDi/SUBi and as a load/store base.
# sf: True = 64-bit (x regs), False = 32-bit (w regs). ft: float type, one of FT.
from enum import IntEnum, auto
from boop.uop.ops import FastEnum

class Arm64Ops(FastEnum):
  # pseudo: emit no bytes. LABEL marks a branch target, DEFINE pins a value to a register without an instruction
  LABEL = auto(); DEFINE = auto()
  # moves / constants. MOV is ORR with xzr. a 32-bit constant is MOVZ + MOVK, or MOVN for small negatives
  MOV = auto(); MOVZ = auto(); MOVK = auto(); MOVN = auto()
  # pc-relative addresses (rodata strings)
  ADR = auto(); ADRP = auto()
  # int alu. i suffix = immediate form. % is SDIV then MSUB: a - (a/b)*b
  ADD = auto(); ADDi = auto(); SUB = auto(); SUBi = auto(); MUL = auto(); SDIV = auto(); MSUB = auto()
  AND = auto(); ORR = auto(); EOR = auto()
  # compare / select. CMP sets flags, CSET/CSEL/FCSEL/Bcond read them
  CMP = auto(); CMPi = auto(); CSET = auto(); CSEL = auto()
  # float alu (h, s or d)
  FMOV = auto(); FADD = auto(); FSUB = auto(); FMUL = auto(); FDIV = auto(); FNEG = auto(); FMADD = auto()
  FCMP = auto(); FCSEL = auto()
  # conversions. FCVT is float<->float, SCVTF int->float, FCVTZS float->int (truncates). FMOVtoF/FMOVtoG copy raw bits
  FCVT = auto(); SCVTF = auto(); FCVTZS = auto(); FMOVtoF = auto(); FMOVtoG = auto()
  # memory. i = [base, #imm] (imm in bytes, must be a multiple of the access size), r = [base, wIdx, sxtw #log2(size)]
  LDRi = auto(); STRi = auto(); LDRr = auto(); STRr = auto()
  # pairs, for the prologue/epilogue: stp x29, x30, [sp, #-16]! / ldp x29, x30, [sp], #16
  STPpre = auto(); LDPpost = auto()
  # control flow. branch offsets are in bytes relative to the branch itself
  B = auto(); Bcond = auto(); CBZ = auto(); CBNZ = auto(); BL = auto(); RET = auto()
  # syscall: x16 = number, x0.. = args, svc #0x80 on macOS
  SVC = auto()

class Arm64GroupOp:
  Pseudo = {Arm64Ops.LABEL, Arm64Ops.DEFINE}
  # register to register copies, regalloc can try to give dst and src the same register and delete them
  Copy = {Arm64Ops.MOV, Arm64Ops.FMOV}
  WriteFlags = {Arm64Ops.CMP, Arm64Ops.CMPi, Arm64Ops.FCMP}
  ReadFlags = {Arm64Ops.CSET, Arm64Ops.CSEL, Arm64Ops.FCSEL, Arm64Ops.Bcond}
  Load = {Arm64Ops.LDRi, Arm64Ops.LDRr, Arm64Ops.LDPpost}
  Store = {Arm64Ops.STRi, Arm64Ops.STRr, Arm64Ops.STPpre}
  # ops whose last operand is a label that render resolves to a pc-relative offset
  Branch = {Arm64Ops.B, Arm64Ops.Bcond, Arm64Ops.CBZ, Arm64Ops.CBNZ, Arm64Ops.BL}
  # no successor: the instruction after these is only reachable by a label
  Terminator = {Arm64Ops.B, Arm64Ops.RET}
  Float = {Arm64Ops.FMOV, Arm64Ops.FADD, Arm64Ops.FSUB, Arm64Ops.FMUL, Arm64Ops.FDIV, Arm64Ops.FNEG, Arm64Ops.FMADD,
           Arm64Ops.FCMP, Arm64Ops.FCSEL, Arm64Ops.FCVT}

# ** registers (apple arm64 abi) **
ZR = SP = 31
FP, LR = 29, 30
ARG_REGS = list(range(8))                         # x0-x7 / v0-v7: args and return value (x0 / v0)
SYSCALL_NUM = 16                                  # x16 holds the syscall number
# x18 is reserved by apple: never touch it. x16/x17 can be clobbered by the linker between calls (we have no linker, but still)
CALLER_SAVED = [*range(16)]                       # x0-x15: a CALL clobbers these
CALLEE_SAVED = [*range(19, 29)]                   # x19-x28: if you use them, save them in the prologue
F_CALLER_SAVED = [*range(8), *range(16, 32)]      # v0-v7, v16-v31
F_CALLEE_SAVED = [*range(8, 16)]                  # v8-v15 (only the low 64 bits are preserved)

class FT(IntEnum):
  """float type field (bits 23:22) of the fp data processing instructions"""
  S = 0; D = 1; H = 3
FT_BYTES = {FT.H: 2, FT.S: 4, FT.D: 8}

class Cond(IntEnum):
  EQ = 0; NE = 1; HS = 2; LO = 3; MI = 4; PL = 5; VS = 6; VC = 7; HI = 8; LS = 9; GE = 10; LT = 11; GT = 12; LE = 13; AL = 14
  def invert(self) -> "Cond": return Cond(self ^ 1)
# after CMP (signed ints) and after FCMP (floats). for floats, MI/LS instead of LT/LE so NaN compares false
INT_COND = {"==": Cond.EQ, "!=": Cond.NE, "<": Cond.LT, "<=": Cond.LE, ">": Cond.GT, ">=": Cond.GE}
FLOAT_COND = {"==": Cond.EQ, "!=": Cond.NE, "<": Cond.MI, "<=": Cond.LS, ">": Cond.GT, ">=": Cond.GE}

# ** encoding **

def _r(*regs:int):
  for r in regs: assert 0 <= r <= 31, f"bad register {r}"
def _simm(v:int, bits:int, scale:int=1) -> int:
  assert v % scale == 0, f"offset {v} isn't a multiple of {scale}"
  v //= scale
  assert -(1 << (bits-1)) <= v < (1 << (bits-1)), f"offset {v*scale} out of range for {bits} bits"
  return v & ((1 << bits) - 1)

def _rrr(base:int, sf:bool, rd:int, rn:int, rm:int) -> int: _r(rd, rn, rm); return base | sf << 31 | rm << 16 | rn << 5 | rd
def _rri(base:int, sf:bool, rd:int, rn:int, imm:int) -> int:
  _r(rd, rn)
  if imm >= 4096 and imm % 4096 == 0 and imm >> 12 < 4096: return base | sf << 31 | 1 << 22 | (imm >> 12) << 10 | rn << 5 | rd
  assert 0 <= imm < 4096, f"immediate {imm} doesn't fit in 12 bits"
  return base | sf << 31 | imm << 10 | rn << 5 | rd
def _fff(base:int, ft:FT, rd:int, rn:int, rm:int) -> int: _r(rd, rn, rm); return base | ft << 22 | rm << 16 | rn << 5 | rd
def _mov16(base:int, sf:bool, rd:int, imm:int, shift:int) -> int:
  _r(rd); assert 0 <= imm < 1 << 16 and shift in ((0, 16, 32, 48) if sf else (0, 16)), f"bad imm {imm} / shift {shift}"
  return base | sf << 31 | (shift // 16) << 21 | imm << 5 | rd
def _adr(base:int, rd:int, imm:int) -> int:
  _r(rd); v = _simm(imm, 21)
  return base | (v & 3) << 29 | (v >> 2) << 5 | rd
def _pages(off:int) -> int:
  assert off % 4096 == 0, f"adrp offset {off} isn't page aligned"
  return off // 4096
def _ldst_size(size:int) -> int: return {1: 0, 2: 1, 4: 2, 8: 3}[size]
def _ldst_i(opc:int, rt:int, rn:int, off:int, size:int, fp:bool) -> int:
  _r(rt, rn); assert off >= 0 and off % size == 0 and off // size < 4096, f"offset {off} not encodable for size {size}"
  return 0x39000000 | _ldst_size(size) << 30 | fp << 26 | opc << 22 | (off // size) << 10 | rn << 5 | rt
def _ldst_r(opc:int, rt:int, rn:int, rm:int, size:int, fp:bool) -> int:
  # option 110 = sxtw (index is a w register, sign extended), S=1 scales it by the access size
  _r(rt, rn, rm)
  return 0x38200800 | _ldst_size(size) << 30 | fp << 26 | opc << 22 | rm << 16 | 0b110 << 13 | (size > 1) << 12 | rn << 5 | rt

ENCODE = {
  Arm64Ops.MOV: lambda sf, rd, rm: _rrr(0x2A000000, sf, rd, ZR, rm),            # orr rd, zr, rm
  Arm64Ops.MOVZ: lambda sf, rd, imm, shift=0: _mov16(0x52800000, sf, rd, imm, shift),
  Arm64Ops.MOVK: lambda sf, rd, imm, shift=0: _mov16(0x72800000, sf, rd, imm, shift),
  Arm64Ops.MOVN: lambda sf, rd, imm, shift=0: _mov16(0x12800000, sf, rd, imm, shift),  # rd = ~(imm << shift)
  Arm64Ops.ADR: lambda rd, off: _adr(0x10000000, rd, off),                      # off in bytes, +-1MB
  Arm64Ops.ADRP: lambda rd, off: _adr(0x90000000, rd, _pages(off)),            # off = target page - this page, in bytes
  Arm64Ops.ADD: lambda sf, rd, rn, rm: _rrr(0x0B000000, sf, rd, rn, rm),
  Arm64Ops.ADDi: lambda sf, rd, rn, imm: _rri(0x11000000, sf, rd, rn, imm),
  Arm64Ops.SUB: lambda sf, rd, rn, rm: _rrr(0x4B000000, sf, rd, rn, rm),
  Arm64Ops.SUBi: lambda sf, rd, rn, imm: _rri(0x51000000, sf, rd, rn, imm),
  Arm64Ops.MUL: lambda sf, rd, rn, rm: _rrr(0x1B007C00, sf, rd, rn, rm),         # madd rd, rn, rm, zr
  Arm64Ops.SDIV: lambda sf, rd, rn, rm: _rrr(0x1AC00C00, sf, rd, rn, rm),
  Arm64Ops.MSUB: lambda sf, rd, rn, rm, ra: _rrr(0x1B008000, sf, rd, rn, rm) | ra << 10,  # rd = ra - rn*rm
  Arm64Ops.AND: lambda sf, rd, rn, rm: _rrr(0x0A000000, sf, rd, rn, rm),
  Arm64Ops.ORR: lambda sf, rd, rn, rm: _rrr(0x2A000000, sf, rd, rn, rm),
  Arm64Ops.EOR: lambda sf, rd, rn, rm: _rrr(0x4A000000, sf, rd, rn, rm),
  Arm64Ops.CMP: lambda sf, rn, rm: _rrr(0x6B000000, sf, ZR, rn, rm),            # subs zr, rn, rm
  Arm64Ops.CMPi: lambda sf, rn, imm: _rri(0x71000000, sf, ZR, rn, imm),
  Arm64Ops.CSET: lambda sf, rd, cond: _rrr(0x1A800400, sf, rd, ZR, ZR) | Cond(cond).invert() << 12,  # csinc rd, zr, zr, !cond
  Arm64Ops.CSEL: lambda sf, rd, rn, rm, cond: _rrr(0x1A800000, sf, rd, rn, rm) | cond << 12,  # rd = cond ? rn : rm
  Arm64Ops.FMOV: lambda ft, rd, rn: _fff(0x1E204000, ft, rd, rn, 0),
  Arm64Ops.FADD: lambda ft, rd, rn, rm: _fff(0x1E202800, ft, rd, rn, rm),
  Arm64Ops.FSUB: lambda ft, rd, rn, rm: _fff(0x1E203800, ft, rd, rn, rm),
  Arm64Ops.FMUL: lambda ft, rd, rn, rm: _fff(0x1E200800, ft, rd, rn, rm),
  Arm64Ops.FDIV: lambda ft, rd, rn, rm: _fff(0x1E201800, ft, rd, rn, rm),
  Arm64Ops.FNEG: lambda ft, rd, rn: _fff(0x1E214000, ft, rd, rn, 0),
  Arm64Ops.FMADD: lambda ft, rd, rn, rm, ra: _fff(0x1F000000, ft, rd, rn, rm) | ra << 10,  # rd = ra + rn*rm
  Arm64Ops.FCMP: lambda ft, rn, rm: _fff(0x1E202000, ft, 0, rn, rm),
  Arm64Ops.FCSEL: lambda ft, rd, rn, rm, cond: _fff(0x1E200C00, ft, rd, rn, rm) | cond << 12,
  Arm64Ops.FCVT: lambda to, frm, rd, rn: _fff(0x1E224000, frm, rd, rn, 0) | to << 15,
  Arm64Ops.SCVTF: lambda sf, ft, rd, rn: _rrr(0x1E220000, sf, rd, rn, 0) | ft << 22,   # float rd = int rn
  Arm64Ops.FCVTZS: lambda sf, ft, rd, rn: _rrr(0x1E380000, sf, rd, rn, 0) | ft << 22,  # int rd = float rn
  Arm64Ops.FMOVtoF: lambda sf, ft, rd, rn: _rrr(0x1E270000, sf, rd, rn, 0) | ft << 22, # bits of int rn -> float rd
  Arm64Ops.FMOVtoG: lambda sf, ft, rd, rn: _rrr(0x1E260000, sf, rd, rn, 0) | ft << 22, # bits of float rn -> int rd
  Arm64Ops.LDRi: lambda rt, rn, off, size, fp=False: _ldst_i(1, rt, rn, off, size, fp),
  Arm64Ops.STRi: lambda rt, rn, off, size, fp=False: _ldst_i(0, rt, rn, off, size, fp),
  Arm64Ops.LDRr: lambda rt, rn, rm, size, fp=False: _ldst_r(1, rt, rn, rm, size, fp),
  Arm64Ops.STRr: lambda rt, rn, rm, size, fp=False: _ldst_r(0, rt, rn, rm, size, fp),
  Arm64Ops.STPpre: lambda rt, rt2, rn, off: (_r(rt, rt2, rn), 0xA9800000 | _simm(off, 7, 8) << 15 | rt2 << 10 | rn << 5 | rt)[1],
  Arm64Ops.LDPpost: lambda rt, rt2, rn, off: (_r(rt, rt2, rn), 0xA8C00000 | _simm(off, 7, 8) << 15 | rt2 << 10 | rn << 5 | rt)[1],
  Arm64Ops.B: lambda off: 0x14000000 | _simm(off, 26, 4),
  Arm64Ops.BL: lambda off: 0x94000000 | _simm(off, 26, 4),
  Arm64Ops.Bcond: lambda cond, off: 0x54000000 | _simm(off, 19, 4) << 5 | cond,
  Arm64Ops.CBZ: lambda sf, rt, off: (_r(rt), 0x34000000 | sf << 31 | _simm(off, 19, 4) << 5 | rt)[1],
  Arm64Ops.CBNZ: lambda sf, rt, off: (_r(rt), 0x35000000 | sf << 31 | _simm(off, 19, 4) << 5 | rt)[1],
  Arm64Ops.RET: lambda: 0xD65F03C0,
  Arm64Ops.SVC: lambda imm=0x80: 0xD4000001 | imm << 5,
}

def encode(op:Arm64Ops, *args, **kwargs) -> int:
  """encode one instruction into a 32-bit word. args: type flags first (sf / ft), then operands in assembly order"""
  assert op not in Arm64GroupOp.Pseudo, f"{op} is a pseudo op, it has no encoding"
  return ENCODE[op](*args, **kwargs)
