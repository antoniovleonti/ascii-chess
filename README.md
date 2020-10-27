# Simple Chess
Simple, portable, & flexible implementation of Chess written in C & displayed using
simple ASCII characters.

I made this over the span of two days with pretty much no outside influence.
This is meant to be a base from which I can build chess variants. Thus, instead of
implementing aggressive optimizations, I instead have aimed to make it
very easy to implement new rules, new pieces, etc.

Good:
- Uses a cool way of checking for checks.
- Minimal hardcoding.
- Each piece has its own function which describes its movement and captures,
  making it easy to add new pieces or rules.

Bad:
- Underpromotion is not implemented.
- Does not use bitboards.
- No AI or playing partner. :/
