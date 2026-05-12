#include <iostream>
#include <limits>

int main() {
    double a = 0.0;
    double b = 0.0;
    char op = '\0';

    std::cout << "Simple calculator\n";
    std::cout << "Enter an expression like 3 + 4 and press Enter:\n";
    if (!(std::cin >> a >> op >> b)) {
        std::cerr << "Invalid input. Please enter a number, an operator, and another number.\n";
        return 1;
    }

    double result = 0.0;
    switch (op) {
        case '+':
            result = a + b;
            break;
        case '-':
            result = a - b;
            break;
        case '*':
            result = a * b;
            break;
        case '/':
            if (b == 0.0) {
                std::cerr << "Error: division by zero is not allowed.\n";
                return 1;
            }
            result = a / b;
            break;
        default:
            std::cerr << "Unsupported operator. Use +, -, *, or /.\n";
            return 1;
    }

    std::cout << "Result: " << result << "\n";
    return 0;
}
