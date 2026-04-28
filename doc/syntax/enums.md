# Enums
- Enums are extremely simple in porpoise, consisting of:
    - An optional backing integral type
    - Variants
    - Optional default values for variants
- Enums are strongly typed, and are declared with the standard declaration syntax or with a `using` statement
    - In the event that the standard declaration syntax is used, it must be `const`
- In the event that the standard declaration syntax is used, it must be `const`
- The backing type of an enum is defaulted to the smallest available type, but can be set to any integer (signed, unsigned, or byte)
- Enums can be casted to and from their underlying type by using the `@cast` builtin
```porpoise
const Colors := enum { red, blue = 3, green };
const Shapes := enum : u64 { circle, square };
```
- Enum variants are namespaced, meaning they do not leak into their outer scope as they would in C
- To access an enum's variant, the `.` operator is used
```porpoise
const a := Colors.red;
const a: Colors = .red; // Equivalent
```
- You can put declarations inside of an enum as you would with a struct
    - These can be member functions, static functions, or static variables only
        - Member functions have a first argument which is an instance of the enum as explained in the struct documentation
    - Attempting to emplace an instance variable in an enum is not supported
        - This means that all non-function declarations must be explicitly marked `static`
    - These members must appear after all enumeration values
