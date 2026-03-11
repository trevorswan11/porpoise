# Arrays
## Declarations
- Arrays are defined with 3 key components
    - A size: This must be an underscore (signifying inferred size), a compile time constant (denoted with `comptime`), or a usize integer literal (suffix `uz`)
    - A type: This can be any valid type, but it must be present and cannot be inferred
    - Items: These items must be of the type specified, and must match the provided size if provided
- Arrays cannot empty, either explicitly or implicitly
- Arrays are immutably sized, though internal elements can be mutable based on the provided type
- A trailing comma is allowed, but not required
```conch
[_]*N{a, b, c, d, e, }; // Inferred Size
[2uz]int{A, B, }; // Explicit size
[1uz]int{2, 3}; // Illegal - size mismatch
[0uz]int{}; // Illegal - empty array
[_]int{}; // Illegal - empty array
```

- Arrays do not decay to pointers implicitly as they do in C
- To call a C function that takes in a pointer that is actually an array, you can use the builtin `@ptrFromArray`
    - This erases the underlying size of the array and is not reversible. It is only recommended for use with C interop

## Types
- There are two types of Array types 
    - Array types are extremely similar to their declaration counterpart, requiring two key components
        - A size: This must be a compile time constant or a usize integer literal
        - A type: This can be any valid type, but it must be present and cannot be inferred. It can be as deeply nested as you'd like
    - Slice types differ from array types in that they lack a size. They are composed of:
        - An open-close bracket pair with nothing in between
        - A type: exactly the same as an array
    - Slices do not have to be of fixed size as they are technically a fat pointer. They store both a pointer and size
    - Values of type 'array' have an implicit `size` field that can be accessed used the dot operator
    - Values of type 'slice' also have this `size` field, but are equipped with a second field called `ptr` to access the pointer directly
    - Values of either type can be used with array-related builtins
- An array type's size cannot be 0
- Array types can have modifiers
```conch
var a: &[S]&*mut T; // Normal array type
var a: &[]&*mut T; // Analogous slice type
```
