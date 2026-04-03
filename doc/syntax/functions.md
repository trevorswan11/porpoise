# Functions
- Functions are defined using the standard declaration syntax
- Function types can be aliased with a `using` statement
- Function parameters are immutable by default
```porpoise
const foo := fn(a: int, b: uint): ulong {
    // ...
};
```

## Parameters
- Parameters obey the same syntax rules as the rest of the type system
- Without any type modifiers, parameters are passed by value
    - All parameters are implicitly constant values
- A parameter may be passed by const reference by marking it as `&`
    - To call such a function, the call site must also indicate with the `&` operator
    - A const pointer is created with the same operator, but the definition denotes this with `*`
- A parameter may be passed by mutable reference by marking it as `&mut`
    - To call such a function, the call site must also indicate with the `&mut` operator
    - A mutable pointer is created with the same operator, but the definition denotes this with `*mut`
- There are no default parameters
```porpoise
const foo := fn(a: &int): ulong {
    // ...
};

const bar := fn(b: int): ulong {
    // ...
};

const baz := fn(c: &mut int): ulong {
    // ...
};

var a := 2;
_ = foo(&a);        // Allowed, passed by const reference
_ = bar(a);         // Allowed, passed by value
_ = baz(&mut a);    // Allowed, passed by mutable reference

const b := 1;
_ = foo(&b);        // Allowed, passed by const reference
_ = bar(b);         // Allowed, passed by value
_ = baz(&mut b);    // Illegal, cannot mutate const
```

- Variadic function parameters are supported to allow for c-interop
    - Currently, porpoise does not expose primitives for interacting with this parameter type
    - For example, you can declare an extern function like `printf` from libc as: `extern const printf: fn(*byte, ...): int;`

## Semantics
- There is no function overloading
    - This includes operators. There is no operator overloading
- Top-level function declarations cannot be marked variable and must be `const`
- Functions can have the `noreturn` return 'type' which signifies that the compiler should not expect a `return` construct in the function body
    - Violating this assumption is a compile time error
- The return types `noreturn` and `void` cannot have any type modifiers
- There are no closures
    - You are able to declare local functions (i.e. functions inside of functions) or anonymous functions (i.e. functions passed directly as a parameter to a function) but they can not access local variables in the enclosing scope

## Builtin Functions
- Builtin functions are prefixed with the `@` symbol and are always camelCase
- Builtin functions can run at compile time or runtime depending on the context, and will attempt to lower to the most efficient instructions when possible

## Calling
- Functions are called in three ways depending on the context
    - If calling a top-level, non-struct function, the syntax is simple (`func(args...)`)
    - If calling a member function, the syntax is similar to other languages (`instance.func(args...)`)
        - This requires the function to have a self parameter, as discussed in the struct documentation
        - The callee must have mutability that matches the explicit self parameter
    - If calling a static struct function, the syntax uses the scope resolution operator (`Struct::func(args...)`)

## Types
- Function's signatures are their types, including parameter modifiers and the return type
    - Note that these signatures do not contain parameter names (this is not supported, even optionally)
    - Signatures do include the variadic argument distinction
- A function can be declared verbosely by using the type before the declaration function
```porpoise
const f: fn(bool): int = fn(b: bool): int { ... };
```

- You can also use this to indicate that a function takes a function as an argument
```porpoise
const f := fn(g: fn(): int): int { ... };
```
