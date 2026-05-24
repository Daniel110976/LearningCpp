CppSymbolicEngine
A C++ symbolic math engine designed for transparency and educational clarity. This engine computes derivatives and performs algebraic operations while exposing the recursive steps taken to reach the solution.
## Features
 * **Step-by-Step Trace:** Unlike standard calculators, this engine logs every recursive step of the differentiation process, allowing you to see exactly how rules (like the Product, Quotient, and Chain rules) are applied.
 * **Recursive Descent Parser:** Built from the ground up to handle nested mathematical expressions and operator precedence.
 * **Extensible Architecture:** Designed to be modular, making it easy to add new mathematical functions or integration rules.
## How It Works
The engine constructs an **Abstract Syntax Tree (AST)** from your input string. By traversing this tree, it applies calculus identities recursively.
## Example Output
When you input an expression, the engine outputs the path it took to solve it:
```text
Input: acosh(3 - 2^t)
Trace:
  - Identifying outer function: acosh(u)
  - Applying chain rule: d/du(acosh(u)) * du/dt
  - ... (recursive steps)
Result: -(2^t * ln(2)) / sqrt((3 - 2^t)^2 - 1)

```
## Getting Started
### Prerequisites
 * A C++ compiler (g++, clang, or MSVC).
 * No external dependencies—the engine is built entirely with standard C++ libraries.
### Building
```bash
g++ -o engine main.cpp
./engine

```
## Project Status
This engine is currently in active development. My focus is on refining the algebraic simplification pass to make final results cleaner and expanding the library of integrable functions.
## Contributing
Contributions are welcome! If you spot an identity that isn't simplifying correctly or have an idea for a new feature, feel free to open an **Issue** or submit a **Pull Request**.
## License
Distributed under the MIT License. See LICENSE for more information.
    
