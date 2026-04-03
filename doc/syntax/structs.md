# Structs
- Structs are defined using the standard declaration syntax
- Struct definitions must be `const`
- Struct members are simply declarations
    - Members cannot be marked `extern` or `export`
    - Members can be marked `pub` to allow external access
    - Members can be marked `static`, denoting struct-level ownership (as opposed to instance)
        - This can be done for any declaration, though it is redundant for functions that do not have a self parameter
        - A member function with a self parameter cannot be marked `static`
        - `static` is only ever meaningful for non-member, non-function declarations. All other use cases are simply to convey meaning to the programmer
    - Member functions have a first argument which is an instance of the struct: `self`
        - A member function can provide this keyword in five different ways:
            1. `self` denotes a pass by value (copy)
            2. `&self` denotes a pass by const reference
            3. `*self` denotes a pass by const pointer
            4. `&mut self` denotes a pass by mutable reference
            5. `*mut self` denotes a pass by mutable pointer
        - This parameter _must_ be the first parameter of the function's parameter list
        - This parameter is conventionally named `self` but is allowed to assume any non-reserved keyword
        - This parameter has the underlying type of the directly enclosing struct
    - Functions are considered to be top-level within the struct and must be `const`
    - Members can be declared in any order, the compiler is order independent and is free to reorder to optimize
- Struct type are internal to the compiler and should seldom be written by hand
    - Struct definitions must use the walrus operator `:=` or `using` assignment
    - The type of a struct can be retrieving by using the `@typeOf` builtin
- Static members are resolved using the `::` operator
- Instance members are resolved using the `.` operator

```porpoise
const Foo := struct {           // Standard declaration with type inference
    var bar: int;               // Mutable member variable
    const baz: []byte = "baz";  // Constant member variable, must be initialized in-line

    pub var boo := 4u;          // Members are private by default
    static const foo := 3.4;    // Static variables are struct globals and are not instance specific

    const worker_one := fn(&self): void {           // Functions have an explicit 'self' parameter
        // ...
    };

    const worker_two := fn(&mut self): void {       // The self parameter can be marked '&mut' to mutate state
        // ...
    };

    pub const worker_three = fn(): void {        // Functions without 'self' parameter are static
        // ...
    };

    // Compile Error - top-level functions cannot be marked 'var'
    // var worker_four := fn(a: int, b: uint): ulong {
    //
    // };

    pub const fee := "Hello, World!";           // Members can be placed anywhere in the struct definition
};

Foo::foo;                                           // Static members are resolved using the '::' operator
Foo::worker_three();                                // The same goes for functions
```

- Structs can be marked `packed` to prevent the compiler from reordering members or from adding additional padding

```porpoise
const Bar := packed struct {
    // ...
};
```

- There is no inheritance
- Interfaces are not natively supported
