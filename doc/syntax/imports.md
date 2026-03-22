# Imports
- There are two kinds of imports in porpoise
    - Library imports: These are imports denoted by identifiers and are declared in dedicated `.p` 'module' files
    - File imports: These are imports denoted by a string that is a file path, followed by an identifier alias
- Both file and library imports allow an identifier alias using the `as` keyword, with this being required for user imports
- An imports publicly available symbols are namespaced behind its name or alias (if applicable), and can be accessed using the `::` operator
```porpoise
import std; // Library import
import std as stud; // Library import with alias
import "node.p"; // Illegal, file imports always require alias
import "node.p" as node; // File import with alias
```
- The filepath given to the import should be relative to the asking file, _not_ to a local project 'root'
- Imports are evaluated lazily
    - An unreferenced import will not be evaluated
    - Unreferenced symbols from an import will not be evaluated
    - This prevents the issue of circular imports without needing to implement a macro system or `pragma`

## The Module Keyword
- The module (`module`) keyword adjusts the visibility of all `using` and `import` statements in the file
    - Imports in non-module files are local to that file
    - Using statements in non-module files are local to that file
    - Imports in module files are automatically exported 
    - Using statements in module files are automatically exported
    - All other declarations require a public modifier for external access
- This keyword must be the first statement of a file if it is used, and there can not be more than one occurrence of it
- This keyword is valid in any file

## Library Import Declaration
- Libraries are declared by indicating that a file is a root of an import tree
- A library's root file path must be provided to the compiler through the command line
    - The root file should have the `module;` keyword to ensure forwarded imports and type aliases
    - Without this, you would have to use a file import for the root file's relative path
    - TODO
- These imports can be aliased to avoid name collisions, though this is not required

## File Import Declaration
- A file in conch is importable through its relative path to the importing file
- These imports must be aliased to an identifier as the string path cannot be used implicitly as a namespace
- The file may have the `module;` keyword to forward imports and type aliases

### Example
- `std.p` declares
```porpoise
module; // Use module keyword at head of file to declare the root of a module

import "io/io.p" as io;
import "containers/array_list.p" as array_list;
pub const ArrayList := array_list::ArrayList;
...
```

- `io/io.p` declares a public function `println` which can be accessed via:
```porpoise
std::io::println(...);
```

- `containers/array_list.p` declares a public function `ArrayList` that returns a generically-typed struct
```porpoise
pub const ArrayList := fn(T: type): type {...};
```
- This can be used just like any other function through the scope resolution operator:
```porpoise
var a: std::array_list::ArrayList(int);
var a: std::ArrayList(int); // Allowed due to function declaration alias
```
