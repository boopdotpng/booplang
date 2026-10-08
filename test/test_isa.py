# every encoding is checked against the system assembler
import os, re, subprocess, tempfile, unittest
from boop.arm64.isa import Arm64Ops as A, FT, Cond, encode, ZR, SP, FP, LR

# (assembly the system assembler sees, our encoding). branches are written as raw offsets: "b .+8"
CASES = [
  ("mov x0, x1", encode(A.MOV, True, 0, 1)), ("mov w3, w4", encode(A.MOV, False, 3, 4)),
  ("movz w0, #0x1234", encode(A.MOVZ, False, 0, 0x1234)), ("movz x5, #0xbeef, lsl #48", encode(A.MOVZ, True, 5, 0xbeef, 48)),
  ("movk w1, #0xffff, lsl #16", encode(A.MOVK, False, 1, 0xffff, 16)), ("movn w2, #4", encode(A.MOVN, False, 2, 4)),
  ("adr x1, .+28", encode(A.ADR, 1, 28)), ("adr x1, .-8", encode(A.ADR, 1, -8)),
  ("add w0, w1, w2", encode(A.ADD, False, 0, 1, 2)), ("add x0, x1, x2", encode(A.ADD, True, 0, 1, 2)),
  ("add x0, sp, #16", encode(A.ADDi, True, 0, SP, 16)), ("add w9, w9, #1", encode(A.ADDi, False, 9, 9, 1)),
  ("sub sp, sp, #0x1000", encode(A.SUBi, True, SP, SP, 0x1000)), ("sub sp, sp, #4080", encode(A.SUBi, True, SP, SP, 4080)),
  ("sub w0, w1, w2", encode(A.SUB, False, 0, 1, 2)), ("mul w0, w1, w2", encode(A.MUL, False, 0, 1, 2)),
  ("sdiv w0, w1, w2", encode(A.SDIV, False, 0, 1, 2)), ("msub w0, w1, w2, w3", encode(A.MSUB, False, 0, 1, 2, 3)),
  ("and w0, w1, w2", encode(A.AND, False, 0, 1, 2)), ("orr w0, w1, w2", encode(A.ORR, False, 0, 1, 2)),
  ("eor w0, w1, w2", encode(A.EOR, False, 0, 1, 2)),
  ("cmp w1, w2", encode(A.CMP, False, 1, 2)), ("cmp x1, #10", encode(A.CMPi, True, 1, 10)),
  ("cset w0, lt", encode(A.CSET, False, 0, Cond.LT)), ("cset w0, eq", encode(A.CSET, False, 0, Cond.EQ)),
  ("csel w0, w1, w2, ne", encode(A.CSEL, False, 0, 1, 2, Cond.NE)),
  ("fmov s0, s1", encode(A.FMOV, FT.S, 0, 1)), ("fmov h0, h1", encode(A.FMOV, FT.H, 0, 1)),
  ("fadd s0, s1, s2", encode(A.FADD, FT.S, 0, 1, 2)), ("fadd h0, h1, h2", encode(A.FADD, FT.H, 0, 1, 2)),
  ("fadd d0, d1, d2", encode(A.FADD, FT.D, 0, 1, 2)), ("fsub s3, s4, s5", encode(A.FSUB, FT.S, 3, 4, 5)),
  ("fmul h0, h1, h2", encode(A.FMUL, FT.H, 0, 1, 2)), ("fdiv s0, s1, s2", encode(A.FDIV, FT.S, 0, 1, 2)),
  ("fneg s0, s1", encode(A.FNEG, FT.S, 0, 1)), ("fmadd s0, s1, s2, s3", encode(A.FMADD, FT.S, 0, 1, 2, 3)),
  ("fcmp s0, s1", encode(A.FCMP, FT.S, 0, 1)), ("fcmp h2, h3", encode(A.FCMP, FT.H, 2, 3)),
  ("fcsel s0, s1, s2, mi", encode(A.FCSEL, FT.S, 0, 1, 2, Cond.MI)),
  ("fcvt s0, h1", encode(A.FCVT, FT.S, FT.H, 0, 1)), ("fcvt h0, s1", encode(A.FCVT, FT.H, FT.S, 0, 1)),
  ("fcvt d0, s1", encode(A.FCVT, FT.D, FT.S, 0, 1)),
  ("scvtf s0, w1", encode(A.SCVTF, False, FT.S, 0, 1)), ("scvtf h0, w1", encode(A.SCVTF, False, FT.H, 0, 1)),
  ("scvtf d0, x1", encode(A.SCVTF, True, FT.D, 0, 1)),
  ("fcvtzs w0, s1", encode(A.FCVTZS, False, FT.S, 0, 1)), ("fcvtzs w0, h1", encode(A.FCVTZS, False, FT.H, 0, 1)),
  ("fmov s0, w1", encode(A.FMOVtoF, False, FT.S, 0, 1)), ("fmov h0, w1", encode(A.FMOVtoF, False, FT.H, 0, 1)),
  ("fmov w0, s1", encode(A.FMOVtoG, False, FT.S, 0, 1)),
  ("ldr w0, [x1, #8]", encode(A.LDRi, 0, 1, 8, 4)), ("ldr x0, [sp, #16]", encode(A.LDRi, 0, SP, 16, 8)),
  ("ldrb w0, [x1, #3]", encode(A.LDRi, 0, 1, 3, 1)), ("str w0, [sp]", encode(A.STRi, 0, SP, 0, 4)),
  ("ldr s0, [x1, #4]", encode(A.LDRi, 0, 1, 4, 4, True)), ("str h0, [x1, #6]", encode(A.STRi, 0, 1, 6, 2, True)),
  ("ldr w0, [x1, w2, sxtw #2]", encode(A.LDRr, 0, 1, 2, 4)), ("str w0, [x1, w2, sxtw #2]", encode(A.STRr, 0, 1, 2, 4)),
  ("ldr s0, [x1, w2, sxtw #2]", encode(A.LDRr, 0, 1, 2, 4, True)), ("ldr h0, [x1, w2, sxtw #1]", encode(A.LDRr, 0, 1, 2, 2, True)),
  ("ldrb w0, [x1, w2, sxtw]", encode(A.LDRr, 0, 1, 2, 1)), ("strb w0, [x1, w2, sxtw]", encode(A.STRr, 0, 1, 2, 1)),
  ("stp x29, x30, [sp, #-16]!", encode(A.STPpre, FP, LR, SP, -16)), ("ldp x29, x30, [sp], #16", encode(A.LDPpost, FP, LR, SP, 16)),
  ("b .+8", encode(A.B, 8)), ("b .-4", encode(A.B, -4)), ("bl .+64", encode(A.BL, 64)),
  ("b.ne .+12", encode(A.Bcond, Cond.NE, 12)), ("b.lt .-16", encode(A.Bcond, Cond.LT, -16)),
  ("cbz w3, .+20", encode(A.CBZ, False, 3, 20)), ("cbnz x3, .-20", encode(A.CBNZ, True, 3, -20)),
  ("ret", encode(A.RET)), ("svc #0x80", encode(A.SVC)),
]

def assemble(lines:list[str]) -> list[int]:
  with tempfile.TemporaryDirectory() as d:
    src, obj = os.path.join(d, "a.s"), os.path.join(d, "a.o")
    with open(src, "w") as f: f.write("".join(l + "\n" for l in lines))
    subprocess.run(["clang", "-c", "-arch", "arm64", src, "-o", obj], check=True)
    out = subprocess.run(["otool", "-t", obj], check=True, capture_output=True, text=True).stdout
  return [int(w, 16) for l in out.splitlines() if re.match(r"^[0-9a-f]{16}\s", l) for w in l.split()[1:]]

class TestISA(unittest.TestCase):
  def test_against_assembler(self):
    want = assemble([".arch armv8.2-a+fp16"] + [asm for asm, _ in CASES])
    self.assertEqual(len(want), len(CASES))
    for (asm, ours), theirs in zip(CASES, want):
      with self.subTest(asm): self.assertEqual(f"{ours:08x}", f"{theirs:08x}")

  def test_out_of_range(self):
    for f in [lambda: encode(A.ADDi, True, 0, 1, 5000), lambda: encode(A.B, 2), lambda: encode(A.LDRi, 0, 1, 6, 4),
              lambda: encode(A.ADD, True, 0, 1, 32), lambda: encode(A.LABEL)]:
      with self.assertRaises(AssertionError): f()

if __name__ == "__main__": unittest.main()
