# Booplang Intermediate Representation (BoopIR) Instruction Set

BoopIR is a **Static Single Assignment (SSA)** intermediate representation designed for lowering into assembly. It features explicit **basic blocks**, **typed variables**, and **minimal control flow** to enable optimizations.

---

## **1. General Syntax**
Each instruction follows this format:

```
<result>: <type> = <operation> <operands>
```

- **SSA rule:** Each variable is assigned **only once**.
- **Basic blocks** (`L1, L2, ...`) define control flow.

Example:
```plaintext
r1: i32 = add r2, r3
```

---

## **2. Data Types**
| Type   | Description           |
|--------|-----------------------|
| `i32`  | 32-bit integer        |
| `f32`  | 32-bit float          |
| `ptr`  | Pointer to memory     |
| `bool` | Boolean (1-bit)       |

---

## **3. Arithmetic Instructions**
| Instruction  | Description                 | Example |
|-------------|-----------------------------|---------|
| `add`       | Addition                     | `r1: i32 = add r2, r3` |
| `sub`       | Subtraction                  | `r1: i32 = sub r2, r3` |
| `mul`       | Multiplication               | `r1: i32 = mul r2, r3` |
| `div`       | Integer division             | `r1: i32 = div r2, r3` |
| `mod`       | Modulus                      | `r1: i32 = mod r2, r3` |
| `neg`       | Negation                     | `r1: i32 = neg r2` |

---

## **4. Logical & Bitwise Instructions**
| Instruction  | Description                 | Example |
|-------------|-----------------------------|---------|
| `and`       | Bitwise AND                  | `r1: i32 = and r2, r3` |
| `or`        | Bitwise OR                   | `r1: i32 = or r2, r3` |
| `xor`       | Bitwise XOR                  | `r1: i32 = xor r2, r3` |
| `not`       | Bitwise NOT                  | `r1: i32 = not r2` |
| `shl`       | Shift left                   | `r1: i32 = shl r2, 3` |
| `shr`       | Shift right                  | `r1: i32 = shr r2, 2` |

---

## **5. Comparison Instructions**
| Instruction  | Description                 | Example |
|-------------|-----------------------------|---------|
| `cmp`       | Compare two values          | `r1: bool = cmp r2, r3` |
| `eq`        | Equal (`==`)                | `r1: bool = eq r2, r3` |
| `neq`       | Not equal (`!=`)            | `r1: bool = neq r2, r3` |
| `lt`        | Less than (`<`)             | `r1: bool = lt r2, r3` |
| `le`        | Less or equal (`<=`)        | `r1: bool = le r2, r3` |
| `gt`        | Greater than (`>`)          | `r1: bool = gt r2, r3` |
| `ge`        | Greater or equal (`>=`)     | `r1: bool = ge r2, r3` |

---

## **6. Control Flow Instructions**
| Instruction  | Description                 | Example |
|-------------|-----------------------------|---------|
| `jmp`       | Unconditional jump          | `jmp L2` |
| `br`        | Conditional branch          | `br r1, L2, L3` |
| `ret`       | Return value                 | `ret r1` |
| `phi`       | Phi function (SSA merge)    | `r1: i32 = phi(r2, r3)` |

---

## **7. Function Instructions**
| Instruction  | Description                 | Example |
|-------------|-----------------------------|---------|
| `call`      | Function call                | `call factorial, r1 -> r2` |
| `alloca`    | Stack allocation             | `r1: ptr = alloca 32` |
| `load`      | Load from memory             | `r1: i32 = load r2` |
| `store`     | Store to memory              | `store r2, r3` |

---

## **8. Memory & Heap Management**
| Instruction  | Description                 | Example |
|-------------|-----------------------------|---------|
| `alloc`     | Allocate heap memory        | `r1: ptr = alloc 64` |
| `free`      | Free heap memory            | `free r1` |

---

## **9. Example: Factorial Function**
```plaintext
function factorial(n: i32) -> i32
L1:
    cmp n, 0
    beq n, L2
    jmp L3

L2:
    r1_1: i32 = 1
    jmp L4

L3:
    r1_2: i32 = n - 1
    call factorial, r1_2 -> r2
    r3: i32 = n * r2
    jmp L4

L4:
    r_ret: i32 = phi(r1_1, r3)
    ret r_ret
end function
```

---

## **10. Example: Loop Constructs**
### **While Loop (`while i < 10`)**
```plaintext
L1:
    cmp i, 10
    bge i, L2
    print i
    i_1: i32 = i + 1
    jmp L1

L2:
```

### **For Loop (`for i from 1 to 6 by 0.5`)**
```plaintext
L1:
    cmp i, 6.0
    bge i, L2
    print i + 1.0
    i_1: f32 = i + 0.5
    jmp L1

L2:
```

---

## **11. Example: Memory Allocation**
```plaintext
function main()
L1:
    r1: ptr = alloc 14  ; Allocate "hello, world"
    store r1, "hello, world"
    print r1
    free r1
end function
```

---

## **12. Design Notes**
- **SSA-form** means all variables are immutable once assigned.
- **Explicit phi nodes** handle control flow merges.
- **Registers first, stack only if necessary**.
- **Heap (`alloc/free`) and stack (`alloca`) are separate**.

This instruction set provides a minimal yet flexible IR for lowering into assembly while keeping optimizations in mind.
