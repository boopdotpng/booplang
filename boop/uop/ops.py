from enum import Enum, IntEnum
import weakref

# an IntEnum whose values keep counting across every subclass, so Ops and Arm64Ops never share a value
class FastEnum(IntEnum):
  def __str__(self): return Enum.__str__(self)
  def __repr__(x): return str(x)
  @staticmethod
  def _generate_next_value_(_, __, ___, last_values): return 1 + max([0, *last_values, *[max(c) for c in FastEnum.__subclasses__()]])

# uop metaclass for caching
class UopMetaClass(type):
  ucache:dict[tuple, weakref.ReferenceType[UOp]] = {}
  def __call__(cls, op:Ops, src:tuple[Uop,...]=tuple(), arg:Any=None, tag:Any=None, metadata:tuple[Metadata,...]|None=None):
    pass

# actual uops
class Uop(metaclass=UopMetaClass):
  op:Ops
  src:tuple[Uop, ...] = tuple()
  arg:Any = None
  tag:Any = None
