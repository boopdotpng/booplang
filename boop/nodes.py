# the ast. it's untyped: types only exist once the builder walks it from fn main.
from __future__ import annotations
from dataclasses import dataclass, field, fields

@dataclass
class Node:
  line: int = field(default=0, kw_only=True, repr=False, compare=False)
  col: int = field(default=0, kw_only=True, repr=False, compare=False)

# ** expressions **

@dataclass
class Num(Node):
  value: int|float
  dtype: str           # "int" | "float" | "half"

@dataclass
class Str(Node):       # only allowed as a direct print() argument
  value: str

@dataclass
class Name(Node):
  id: str

@dataclass
class Unary(Node):
  op: str              # "-" | "not"
  x: Node

@dataclass
class Binary(Node):
  op: str              # + - * / % == != < <= > >= and or
  a: Node
  b: Node

@dataclass
class Cast(Node):      # float(x), int(x), half(x), bool(x)
  dtype: str
  x: Node

@dataclass
class Call(Node):
  fn: str
  args: list[Node]

@dataclass
class Index(Node):     # a[i]
  arr: Node
  idx: Node

@dataclass
class ArrayLit(Node):  # [1.0, 2.0]
  elems: list[Node]

@dataclass
class ArrayAlloc(Node):  # float[16], zeroed. size is a compile time int
  dtype: str
  size: int

# ** statements **

@dataclass
class Assign(Node):    # x = e
  name: str
  value: Node

@dataclass
class Store(Node):     # a[i] = e
  arr: Node
  idx: Node
  value: Node

@dataclass
class If(Node):
  cond: Node
  body: list[Node]
  orelse: list[Node]   # else if -> [If(...)]

@dataclass
class For(Node):       # for var in count
  var: str
  count: Node
  body: list[Node]

@dataclass
class While(Node):
  cond: Node
  body: list[Node]

@dataclass
class Return(Node):
  value: Node|None

@dataclass
class Print(Node):
  args: list[Node]

@dataclass
class ExprStmt(Node):  # a bare call, e.g. scale(v, 0.5)
  expr: Node

@dataclass
class Fn(Node):
  name: str
  params: list[str]
  body: list[Node]

@dataclass
class Program(Node):
  fns: dict[str, Fn]

def dump(n, indent:int=0) -> str:
  pad = "  "*indent
  if isinstance(n, list): return "\n".join(dump(x, indent) for x in n) if n else pad+"[]"
  if not isinstance(n, Node): return pad+repr(n)
  if isinstance(n, Program): return "\n".join(dump(f, indent) for f in n.fns.values())
  leaves, kids = [], []
  for f in fields(n):
    if f.name in ("line", "col"): continue
    v = getattr(n, f.name)
    if isinstance(v, Node) or (isinstance(v, list) and f.name != "params"):
      if v != []: kids.append((f.name, v))
    else: leaves.append(repr(v))
  out = [f"{pad}Fn {n.name}({', '.join(n.params)})" if isinstance(n, Fn) else f"{pad}{type(n).__name__}({', '.join(leaves)})"]
  for name, v in kids:
    if isinstance(v, list): out += [f"{pad}  .{name}", dump(v, indent+2)]
    else: out.append(dump(v, indent+1))
  return "\n".join(out)
