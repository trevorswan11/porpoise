# Imports
- There are two kinds of imports in porpoise
    - Module imports: These are imports denoted by identifiers and are declared in dedicated `.p` 'module' files
    - User imports: These are imports denoted by a string that is a file path, followed by an identifier alias
- Both user and module imports allow an identifier alias using the `as` keyword, with this being required for user imports
- An imports publicly available symbols are namespaced behind the alias, and can be accessed using the `::` operator
```porpoise
import std; // Module import
import std as stud; // Module import with alias
import "node.p"; // Illegal, user imports always require alias
import "node.p" as node; // User import with alias
```
- The filepath given to the import should be relative to the asking file, _not_ to a local project 'root'
- Imports are evaluated lazily
    - An unreferenced import will not be evaluated
    - Unreferenced symbols from an import will not be evaluated
    - This prevents the issue of circular imports without needing to implement a macro system or `pragma`

## Module Import Declaration
- Modules are declared by indicating that a file is a root of an import tree
- Modules are required to have their root file declared with the first statement being `module;`
    - The presence of this keyword prevents the import of the file through its path
- Visibility
    - Imports in non-module files are local to that file
    - Using statements in non-module files are local to that file
    - Imports in module files are automatically exported 
    - Using statements in module files are automatically exported
    - All other declarations require a public modifier for external access

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
