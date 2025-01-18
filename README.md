## booplang
as close to english as you can get, without sacrificing performance. this will compile to arm64, specifically for m1 and above macs. x64 support after i can solve a leetcode problem using booplang. 


## building
This project uses CMake. 
1. `mkdir build`
2. `cd build` 
3. `cmake ..`
4. `cmake --build .`

Optionally, you can disable formatting (even if you have astyle installed) by running `cmake -DAUTO_FORMAT=OFF ..`. 

## todo list
- [x] write an open-addressed hashmap with double hashing from scratch
- [x] finalize a syntax for my language
- [x] implement string interning
- [ ] remove unnecessary failed malloc checks. never gonna happen
- [ ] write the lexer/tokenizer (halfway done)
- [ ] write an abstract syntax tree
- [ ] type checking / minor optimizations
- [ ] error handling / nice error messages
- [ ] come up with my own IR
- [ ] ast -> ir
- [ ] ir -> arm assembly
- [ ] assemble and link the final executable
- [ ] print/stdout (link to macos)
- [ ] standard library

## notes 

- I won't optimize for performance until the language is almost complete. I want to have a working language quicker, as opposed to working on it an extra month just for a 20% speedup. 
- My goal for the next year is to get into FPGA and CPU design, and I think a project like this is a really good way to transition into that.  
- The estimated timeline is a few months (I have a 9-5). 