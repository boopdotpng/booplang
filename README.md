## booplang
I wanted to write a compiler-simple as that. But more than that, i wanted to see how many features from interpreted languages could be adapted into a compiled one. The goal is to create something as intuitive and ergonomic as python, but fully compiled. if i can pull that off without unnecessary bloat (*cough* rust), that would be a win.

You can take a look at the currently supported syntax at [examples/basic](https://github.com/boopdotpng/booplang/blob/master/examples/basic.boop). It aims to be really straightforward to use and learn.

## building
This project uses Meson/ninja. 
1. `meson setup build`
2. `meson compile -C build` 
3. Executable is `./build/boop`.

## todo list
- [x] write an open-addressed hashmap with double hashing from scratch
- [x] finalize a syntax for my language
- [x] implement string interning
- [x] write the lexer/tokenizer 
- [x] remove unnecessary failed malloc checks. never gonna happen
- [x] error handling / nice error messages
- [ ] write an abstract syntax tree generator
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

## contributing
Any contributions are appreciated! It's going to take me a while to do this on my own. Make sure you install the pre-commit hook using `./install-hooks.sh` before creating a PR so that your changes are formatted the same way as the rest of the repository. 