import os, subprocess, tempfile, unittest
from boop.macho import write_executable, rodata_offset

MOV_X0_1, MOV_X2_6, MOV_X16_4, SVC, MOV_X0_7, RET, NOP = 0xd2800020, 0xd28000c2, 0xd2800090, 0xd4001001, 0xd28000e0, 0xd65f03c0, 0xd503201f
def adr(rd:int, imm:int) -> int: return 0x10000000 | ((imm & 3) << 29) | (((imm >> 2) & 0x7ffff) << 5) | rd

class TestMachO(unittest.TestCase):
  def run_exe(self, *args, **kwargs) -> subprocess.CompletedProcess:
    with tempfile.TemporaryDirectory() as d:
      return subprocess.run([write_executable(os.path.join(d, "a.out"), *args, **kwargs), "x", "y"], capture_output=True)

  def test_hello(self):
    # write(1, msg, 6); return 7
    code = [MOV_X0_1, None, MOV_X2_6, MOV_X16_4, SVC, MOV_X0_7, RET]
    code[1] = adr(1, rodata_offset(len(code)*4) - 4)
    r = self.run_exe(code, b"hello\n")
    self.assertEqual((r.stdout, r.returncode), (b"hello\n", 7))

  def test_argc(self):
    # main returns argc unchanged: program + 2 args
    self.assertEqual(self.run_exe([RET]).returncode, 3)

  def test_entry_and_big(self):
    # entry past a page of nops, rodata spanning pages
    code = [NOP]*5000 + [MOV_X0_7, RET]
    self.assertEqual(self.run_exe(code, b"\xaa"*40000, entry=5000*4).returncode, 7)

if __name__ == "__main__": unittest.main()
