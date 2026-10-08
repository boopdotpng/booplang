from boop.lexer import Tok, BoopError, lex
from boop.nodes import *

CMP_OPS = {"==", "!=", "<", "<=", ">", ">="}

class Parser:
  def __init__(self, toks:list[Tok]):
    self.toks, self.i = toks, 0

  # ** token helpers **

  def peek(self, k:int=0) -> Tok: return self.toks[min(self.i+k, len(self.toks)-1)]
  def at(self, kind:str, val=None, k:int=0) -> bool:
    t = self.peek(k)
    return t.kind == kind and (val is None or t.val == val)
  def accept(self, kind:str, val=None) -> Tok|None:
    if not self.at(kind, val): return None
    self.i += 1
    return self.toks[self.i-1]
  def expect(self, kind:str, val=None, what:str|None=None) -> Tok:
    if (t:=self.accept(kind, val)) is not None: return t
    t = self.peek()
    got = {"newline": "end of line", "indent": "indented block", "dedent": "end of block", "eof": "end of file"}.get(t.kind, repr(t.val))
    raise BoopError(f"expected {what or repr(val) if val else what or kind}, got {got}", t.line, t.col)
  def pos(self, t:Tok) -> dict: return {"line": t.line, "col": t.col}

  # ** program / statements **

  def program(self) -> Program:
    fns: dict[str, Fn] = {}
    while not self.at("eof"):
      t = self.peek()
      if not self.at("kw", "fn"): raise BoopError("statements must be inside a fn", t.line, t.col)
      fn = self.fn()
      if fn.name in fns: raise BoopError(f"fn {fn.name} is defined twice", fn.line, fn.col)
      if fn.name == "main" and len(fn.params) > 2: raise BoopError("main takes at most 2 args (argc, argv)", fn.line, fn.col)
      fns[fn.name] = fn
    return Program(fns)

  def fn(self) -> Fn:
    t = self.expect("kw", "fn")
    name = self.expect("name", what="fn name").val
    self.expect("op", "(")
    params: list[str] = []
    while not self.at("op", ")"):
      p = self.expect("name", what="parameter name")
      if p.val in params: raise BoopError(f"duplicate parameter {p.val}", p.line, p.col)
      params.append(p.val)
      if not self.accept("op", ","): break
    self.expect("op", ")")
    return Fn(name, params, self.block(), **self.pos(t))

  def block(self) -> list[Node]:
    self.expect("newline")
    self.expect("indent", what="indented block")
    body = []
    while not self.accept("dedent"): body.append(self.stmt())
    return body

  def stmt(self) -> Node:
    t = self.peek()
    if self.accept("kw", "if"): return self.if_rest(t)
    if self.accept("kw", "while"): return While(self.expr(), self.block(), **self.pos(t))
    if self.accept("kw", "for"):
      var = self.expect("name", what="loop variable").val
      self.expect("kw", "in")
      return For(var, self.expr(), self.block(), **self.pos(t))
    if self.at("kw", "fn"): raise BoopError("fns can't be nested", t.line, t.col)
    if self.at("kw", "else"): raise BoopError("else without if", t.line, t.col)

    if self.accept("kw", "return"): s = Return(None if self.at("newline") else self.expr(), **self.pos(t))
    elif self.accept("kw", "print"): s = Print(self.print_args(), **self.pos(t))
    elif self.at("name") and self.at("op", "=", k=1):
      self.i += 2
      s = Assign(t.val, self.expr(), **self.pos(t))
    else:
      e = self.expr()
      if self.accept("op", "="):
        if not isinstance(e, Index): raise BoopError("can only assign to a name or a[i]", t.line, t.col)
        s = Store(e.arr, e.idx, self.expr(), **self.pos(t))
      elif isinstance(e, Call): s = ExprStmt(e, **self.pos(t))
      else: raise BoopError("expression result is unused", t.line, t.col)
    self.expect("newline", what="end of line")
    return s

  def if_rest(self, t:Tok) -> If:
    cond, body, orelse = self.expr(), self.block(), []
    if (e:=self.accept("kw", "else")):
      orelse = [self.if_rest(e)] if self.accept("kw", "if") else self.block()
    return If(cond, body, orelse, **self.pos(t))

  def print_args(self) -> list[Node]:
    self.expect("op", "(")
    args = []
    while not self.at("op", ")"):
      t = self.peek()
      args.append(Str(t.val, **self.pos(t)) if self.accept("str") else self.expr())
      if not self.accept("op", ","): break
    self.expect("op", ")")
    return args

  # ** expressions, lowest precedence first **

  def expr(self) -> Node: return self.or_()

  def or_(self) -> Node:
    a = self.and_()
    while (t:=self.accept("kw", "or")): a = Binary("or", a, self.and_(), **self.pos(t))
    return a

  def and_(self) -> Node:
    a = self.not_()
    while (t:=self.accept("kw", "and")): a = Binary("and", a, self.not_(), **self.pos(t))
    return a

  def not_(self) -> Node:
    if (t:=self.accept("kw", "not")): return Unary("not", self.not_(), **self.pos(t))
    return self.cmp()

  def cmp(self) -> Node:
    a = self.add()
    if self.at("op") and self.peek().val in CMP_OPS:
      t = self.accept("op")
      a = Binary(t.val, a, self.add(), **self.pos(t))
      if self.at("op") and self.peek().val in CMP_OPS:
        t = self.peek()
        raise BoopError("comparisons can't be chained, use and", t.line, t.col)
    return a

  def add(self) -> Node:
    a = self.mul()
    while self.at("op", "+") or self.at("op", "-"):
      t = self.accept("op")
      a = Binary(t.val, a, self.mul(), **self.pos(t))
    return a

  def mul(self) -> Node:
    a = self.unary()
    while self.at("op", "*") or self.at("op", "/") or self.at("op", "%"):
      t = self.accept("op")
      a = Binary(t.val, a, self.unary(), **self.pos(t))
    return a

  def unary(self) -> Node:
    if (t:=self.accept("op", "-")): return Unary("-", self.unary(), **self.pos(t))
    return self.postfix()

  def postfix(self) -> Node:
    a = self.atom()
    while (t:=self.accept("op", "[")):
      a = Index(a, self.expr(), **self.pos(t))
      self.expect("op", "]")
    return a

  def atom(self) -> Node:
    t = self.peek()
    if t.kind in ("int", "float", "half"):
      self.i += 1
      return Num(t.val, t.kind, **self.pos(t))
    if t.kind == "str": raise BoopError("strings can only appear in print", t.line, t.col)
    if self.accept("op", "("):
      e = self.expr()
      self.expect("op", ")")
      return e
    if self.accept("op", "["):
      elems = []
      while not self.at("op", "]"):
        elems.append(self.expr())
        if not self.accept("op", ","): break
      self.expect("op", "]")
      if not elems: raise BoopError("empty array literal, use T[n]", t.line, t.col)
      return ArrayLit(elems, **self.pos(t))
    if self.accept("type"):
      if self.accept("op", "["):
        n = self.expect("int", what="array size (an int literal)")
        if n.val <= 0: raise BoopError("array size must be positive", n.line, n.col)
        self.expect("op", "]")
        return ArrayAlloc(t.val, n.val, **self.pos(t))
      self.expect("op", "(", what=f"'(' or '[' after {t.val}")
      x = self.expr()
      self.expect("op", ")")
      return Cast(t.val, x, **self.pos(t))
    if self.accept("name"):
      if not self.accept("op", "("): return Name(t.val, **self.pos(t))
      args = []
      while not self.at("op", ")"):
        args.append(self.expr())
        if not self.accept("op", ","): break
      self.expect("op", ")")
      return Call(t.val, args, **self.pos(t))
    if t.kind == "kw" and t.val == "print": raise BoopError("print is a statement, it can't be used as a value", t.line, t.col)
    got = {"newline": "end of line", "eof": "end of file"}.get(t.kind, repr(t.val))
    raise BoopError(f"expected an expression, got {got}", t.line, t.col)

def parse(src:str) -> Program: return Parser(lex(src)).program()
