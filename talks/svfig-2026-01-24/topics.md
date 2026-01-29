More topics, shelved until later.

### Design of programming languages: fundamental problems and solutions we need.

- From my perspective, most languages and compilers are designed the wrong way around.
- The right way is self-bootstrap, freedom of scripting the compiler the inside, and freedom of JIT / AOT execution.
- Forth is an interesting foundation for building a _higher-level_ language on top.

### What Forth really is

- It's not really about the stacks.
- Comparison with Lisp.
- The power of lower-level foundations.
- Using Forth as a foundation of a higher-level language.

The key thing other languages don't offer is that every token in code can be executable. Words can run during compilation, and can control both the parser and the interpreter / compiler. So even if you're building a higher level language which builds a regular syntax tree, there's still interesting power in the idea that _tokens_ can run _code_ which was just previously defined, to mess around with the token stream and do interesting stuff, instead of everything related to the AST being inside the compiler blackbox.
