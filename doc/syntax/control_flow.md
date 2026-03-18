# Control Flow
## Entry Point
- The entry point of a program must be a public `main` function that returns an integer
    - This is a special function that does not need an explicit return statement (implicitly returns 0)
- This function must take in a single parameter of type `[][:0]byte` representing the passed command line args
    - This is conventionally named `args` but can be named to your choosing
    - Note that these args are null terminated to respect C-interop with minimal friction
```porpoise
import std;

pub const main := fn(args: [][:0]byte): int {
    const message := "Hello, world!";
    std::io::println(message);
};
```

## Jumps
- Jump statements are implemented using the `return`, `break` and `continue` keywords
- The `return` keyword is the only jump statement that allows an associated value
    - This value can be any expression, including a type alias or declaration (new union, struct, or enum)
- `continue` and `break` can only be used in loop bodies, while `return` can only be used inside of functions
- `continue` and `break` will always apply to the immediately enclosing loop

## Conditional Expressions
- If-else expressions are a crucial part of control flow
- They are composed of a required conditional and consequence with an optional alternate
- The alternate clause is a restricted else statement, similar to `match`, `for`, and `while` loops. Similarly, it can only be a:
    - Jump statement
    - Expression statement
    - Block statement
- If the consequence and alternate statements are both expression statements, then the resulting value is returned
```porpoise
a = if (a) b; else c; // Assigns a conditionally
```

## Defer
- Defer statements defer execution to the end of their scope
- They are executed in reverse order with respect to their declaration
- A defer statement can be an expression, block, or discard statement
    - All other inner statement variants are disallowed
```porpoise
import std;

const main := fn(): void {
    defer std::io::println("World!");
    defer std::io::print("Hello, ");
};
```
- This example prints "Hello, World!"
    - Every defer statement pushes execution onto a LIFO stack that is executed at the end of scope
