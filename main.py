import os, sys
from boop.lexer import BoopError, lex
from boop.parser import parse
from boop.nodes import dump

def getenv(key:str) -> int: return int(os.getenv(key, "0"))

if __name__ == "__main__":
  if len(sys.argv) != 2: sys.exit("usage: [PRINT_AST=1] python main.py file.boop")
  src = open(sys.argv[1]).read()
  try:
    prog = parse(src)
    if getenv("PRINT_AST"): print(dump(prog))
  except BoopError as e: sys.exit(e.render(src, sys.argv[1]))
