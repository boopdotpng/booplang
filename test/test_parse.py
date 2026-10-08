import pathlib, re, unittest
from boop.lexer import BoopError
from boop.parser import parse
from boop.nodes import *

EXAMPLES = pathlib.Path(__file__).parent.parent / "examples"
# errors that the parser catches. the rest of examples/errors are well formed and fail later, in the builder
PARSE_ERRORS = {"toplevel.boop", "string_var.boop", "main_args.boop"}

class TestExamples(unittest.TestCase):
  def test_examples_parse(self):
    for f in sorted(EXAMPLES.glob("*.boop")):
      with self.subTest(f.name): self.assertIn("main", parse(f.read_text()).fns)

  def test_errors(self):
    for f in sorted((EXAMPLES / "errors").glob("*.boop")):
      with self.subTest(f.name):
        src = f.read_text()
        if f.name in PARSE_ERRORS:
          want = re.search(r"; error: (.*)", src).group(1)
          with self.assertRaises(BoopError) as cm: parse(src)
          self.assertEqual(cm.exception.msg, want)
        else: parse(src)

def body(src:str) -> list[Node]: return parse("fn main()\n" + "".join("  "+l+"\n" for l in src.split("\n"))).fns["main"].body
def expr(src:str) -> Node: return body(f"x = {src}")[0].value

class TestExprs(unittest.TestCase):
  def test_precedence(self):
    self.assertEqual(expr("1 + 2 * 3"), Binary("+", Num(1, "int"), Binary("*", Num(2, "int"), Num(3, "int"))))
    self.assertEqual(expr("a - b - c"), Binary("-", Binary("-", Name("a"), Name("b")), Name("c")))
    self.assertEqual(expr("-a * b"), Binary("*", Unary("-", Name("a")), Name("b")))
    self.assertEqual(expr("not a < b and c"), Binary("and", Unary("not", Binary("<", Name("a"), Name("b"))), Name("c")))
    self.assertEqual(expr("a or b and c"), Binary("or", Name("a"), Binary("and", Name("b"), Name("c"))))

  def test_literals(self):
    self.assertEqual(expr("1.5h"), Num(1.5, "half"))
    self.assertEqual(expr("2.0"), Num(2.0, "float"))
    self.assertEqual(expr("float[16]"), ArrayAlloc("float", 16))
    self.assertEqual(expr("half(x)"), Cast("half", Name("x")))
    self.assertEqual(expr("[1, 2]"), ArrayLit([Num(1, "int"), Num(2, "int")]))
    self.assertEqual(expr("a[i*4 + k]"), Index(Name("a"), Binary("+", Binary("*", Name("i"), Num(4, "int")), Name("k"))))

  def test_stmts(self):
    self.assertEqual(body("a[0] = 1"), [Store(Name("a"), Num(0, "int"), Num(1, "int"))])
    self.assertEqual(body("f(1)"), [ExprStmt(Call("f", [Num(1, "int")]))])
    self.assertEqual(body('print("x", x)'), [Print([Str("x"), Name("x")])])
    self.assertEqual(body("if a\n  return 1\nelse if b\n  return 2\nelse\n  return"),
      [If(Name("a"), [Return(Num(1, "int"))], [If(Name("b"), [Return(Num(2, "int"))], [Return(None)])])])

  def test_bad(self):
    for src, msg in [("x = a < b < c", "comparisons can't be chained, use and"), ("1 + 2", "expression result is unused"),
                     ("x = float[0]", "array size must be positive"), ("x = []", "empty array literal, use T[n]"),
                     ("f(1) = 2", "can only assign to a name or a[i]"), ("x = 1.", "expected digits after '.'")]:
      with self.subTest(src):
        with self.assertRaises(BoopError) as cm: body(src)
        self.assertEqual(cm.exception.msg, msg)

if __name__ == "__main__": unittest.main()
