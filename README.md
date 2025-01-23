## booplang
As close to english as you can get, without sacrificing performance. This will compile to arm64, specifically for m1 and above macs. x64 support will be implemented after I can solve a leetcode problem using booplang on arm64. Okay, it's not actually English. But the main goal was to write a language that eliminates reaching for weird symbols on your keyboard like () {} [] ` and others. The actual language syntax doesn't really matter, so I wanted to do something unique. 

You can take a look at the current syntax (most of it) at [examples/basic.boop](https://github.com/boopdotpng/booplang/blob/master/examples/basic.boop)

I might consider implementing a system where you can choose the langauge's syntax based on what you prefer. 

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
- [x] write the lexer/tokenizer 
- [ ] write an abstract syntax tree generator
- [ ] remove unnecessary failed malloc checks. never gonna happen
- [ ] error handling / nice error messages
- [ ] type checking / minor optimizations
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

## documentation 

The documentation will be updated when I get further into the project, but the aim is to make compilers and assembly more understandable for newer cs students. I'm looking forward to the massive write-up after I'm done! 