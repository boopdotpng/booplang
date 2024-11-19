## booplang
as close to english as you can get, without sacrificing performance. this will compile to aarch64, specifically for m1 and above macs. more arm devices might work in the future. 


## building
This project uses CMake. 
1. `mkdir build`
2. `cd build` 
3. `cmake ..`
4. `cmake --build .`

Optionally, you can disable formatting (even if you have astyle installed) by running `cmake -DAUTO_FORMAT=OFF ..`. 

## todo list
- [x] stream file input instead of reading it all into memory
- [x] finalize a syntax for my language
- [ ] write the lexer/tokenizer
- [ ] write an abstract syntax tree
- [ ] type checking / minor optimizations
- [ ] error handling / nice error messages
- [ ] come up with my own IR
- [ ] ast -> ir
- [ ] ir -> arm assembly
- [ ] assemble and link the final executable
- [ ] print/stdout (link to macos)
- [ ] standard library
