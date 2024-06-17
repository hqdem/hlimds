[//]: <> (SPDX-License-Identifier: Apache-2.0)

# Utopia EDA Coding Style

We use [LLVM Coding Conventions](https://llvm.org/docs/CodingStandards.html)
with some project-specific modifications.

Some of them are listed below.

* Use LF-ended source files (*.cpp, *.h, *.hpp, etc.).
* Use ASCII symbols only, no Cyrillic symbols are allowed.
* Basic indent is 2 spaces, continuation indent 4 spaces, no tabs are allowed.
* Maximum line length is 80, no trailing whitespaces.
* Do not use multiple subsequent blank lines.
* Use lowercase_underscore_separated style for names of source files.
* Use UpperCamelCase style for names of classes/enums/structures/types/unions.
* Use lowerCamelCase style for names of functions/methods/objects/variables.
* "{" symbol should be at the same line as the related operator has.
* Source files should have header comments (set the right `<yearnum>` here):

```cpp
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright <yearnum> ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
```

* All the header files should have Doxygen-formatted comments for classes:

```cpp
/**
 * \brief Implements a very useful thing.
 */
```

* All the includes should be listed in the following order:

1) project includes;
2) third-party library includes;
3) system includes.

Includes should be sorted in alphabetical order at every category.

* "_" symbol is allowed for file names only; it is forbidden for program code.
