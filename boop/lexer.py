from dataclasses import dataclass

KEYWORDS = {"fn", "if", "else", "while", "for", "in", "return", "print", "and", "or", "not"}
TYPES = {"int", "float", "half", "bool"}
OPS = ["==", "!=", "<=", ">=", "<", ">", "=", "+", "-", "*", "/", "%", "(", ")", "[", "]", ","]

class BoopError(Exception):
  def __init__(self, msg:str, line:int=0, col:int=0):
    super().__init__(msg)
    self.msg, self.line, self.col = msg, line, col
  def render(self, src:str, fn:str="<src>") -> str:
    if not self.line: return f"{fn}: error: {self.msg}"
    text = src.split("\n")[self.line-1]
    return f"{fn}:{self.line}:{self.col}: error: {self.msg}\n  {text}\n  {' '*(self.col-1)}^"

# kind: name type kw int float half str op newline indent dedent eof
@dataclass(frozen=True)
class Tok:
  kind: str
  val: object
  line: int
  col: int
  def __repr__(self): return f"{self.kind}:{self.val!r}@{self.line}:{self.col}"

def lex(src:str) -> list[Tok]:
  toks: list[Tok] = []
  indents = [0]
  for ln, line in enumerate(src.split("\n"), 1):
    stripped = line.lstrip(" ")
    if not stripped or stripped.startswith(";"): continue  # blank and comment-only lines don't affect indentation
    if stripped[0] == "\t" or "\t" in line[:len(line)-len(stripped)]: raise BoopError("tabs aren't allowed for indentation", ln, 1)
    ind = len(line) - len(stripped)
    if ind > indents[-1]:
      indents.append(ind)
      toks.append(Tok("indent", ind, ln, 1))
    while ind < indents[-1]:
      indents.pop()
      toks.append(Tok("dedent", ind, ln, 1))
    if ind != indents[-1]: raise BoopError("dedent doesn't match any outer indentation level", ln, 1)

    i = ind
    while i < len(line):
      c, col = line[i], i+1
      if c == " ": i += 1
      elif c == ";": break
      elif c == '"':
        j = line.find('"', i+1)
        if j == -1: raise BoopError("unterminated string", ln, col)
        toks.append(Tok("str", line[i+1:j], ln, col))
        i = j+1
      elif c.isdigit():
        j = i
        while j < len(line) and line[j].isdigit(): j += 1
        is_float = j < len(line) and line[j] == "."
        if is_float:
          j += 1
          if j >= len(line) or not line[j].isdigit(): raise BoopError("expected digits after '.'", ln, j+1)
          while j < len(line) and line[j].isdigit(): j += 1
        text = line[i:j]
        if j < len(line) and line[j] == "h": toks.append(Tok("half", float(text), ln, col)); j += 1
        elif is_float: toks.append(Tok("float", float(text), ln, col))
        else: toks.append(Tok("int", int(text), ln, col))
        if j < len(line) and (line[j].isalnum() or line[j] == "_"): raise BoopError(f"bad number literal", ln, col)
        i = j
      elif c.isalpha() or c == "_":
        j = i
        while j < len(line) and (line[j].isalnum() or line[j] == "_"): j += 1
        word = line[i:j]
        toks.append(Tok("kw" if word in KEYWORDS else "type" if word in TYPES else "name", word, ln, col))
        i = j
      else:
        op = next((o for o in OPS if line.startswith(o, i)), None)
        if op is None: raise BoopError(f"unexpected character {c!r}", ln, col)
        toks.append(Tok("op", op, ln, col))
        i += len(op)
    toks.append(Tok("newline", None, ln, len(line)+1))
  last = toks[-1].line+1 if toks else 1
  toks += [Tok("dedent", 0, last, 1) for _ in indents[1:]]
  toks.append(Tok("eof", None, last, 1))
  return toks
