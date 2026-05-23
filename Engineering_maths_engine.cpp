#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <cctype>
#include <stdexcept>

// ============================================================================
// 1. ABSTRACT BASE CLASS (The "Interface")
// ============================================================================
// Every math component (number, variable, operation) inherits from Node.
// unique_ptr automatically deletes the memory when the tree goes out of scope.
class Node {
public:
    virtual ~Node() = default;
    virtual std::string toString() const = 0;
    virtual double eval(double t) const = 0; // Calculates the value at 't'
    virtual std::unique_ptr<Node> clone() const = 0;
};

// ============================================================================
// 2. NODE IMPLEMENTATIONS
// ============================================================================

class ConstantNode : public Node {
    double value;
public:
    ConstantNode(double val) : value(val) {}
    std::string toString() const override { return std::to_string(value); }
    
    // Fix: Remove 't' or comment it out like this:
    double eval(double /*t*/) const override { return value; } 
    
    std::unique_ptr<Node> clone() const override { return std::make_unique<ConstantNode>(value); }
};

class VariableNode : public Node {
    std::string name;
public:
    VariableNode(std::string nm) : name(nm) {}
    std::string toString() const override { return name; }
    double eval(double t) const override { return (name == "t") ? t : 0.0; }
    std::unique_ptr<Node> clone() const override { return std::make_unique<VariableNode>(name); }
};

class BinaryOpNode : public Node {
    std::string op;
    std::unique_ptr<Node> left, right;
public:
    BinaryOpNode(std::string o, std::unique_ptr<Node> l, std::unique_ptr<Node> r) 
        : op(o), left(std::move(l)), right(std::move(r)) {}

    std::string toString() const override { return "(" + left->toString() + " " + op + " " + right->toString() + ")"; }
    
    double eval(double t) const override {
        double l = left->eval(t), r = right->eval(t);
        if (op == "+") return l + r;
        if (op == "-") return l - r;
        if (op == "*") return l * r;
        if (op == "/") return l / r;
        return 0;
    }
    std::unique_ptr<Node> clone() const override { return std::make_unique<BinaryOpNode>(op, left->clone(), right->clone()); }
};

class FunctionNode : public Node {
    std::string name;
    std::unique_ptr<Node> arg;
public:
    FunctionNode(std::string nm, std::unique_ptr<Node> a) : name(nm), arg(std::move(a)) {}
    std::string toString() const override { return name + "(" + arg->toString() + ")"; }
    
    double eval(double t) const override {
        double val = arg->eval(t);
        if (name == "exp") return std::exp(val);
        if (name == "ln")  return std::log(val);
        if (name == "sqrt") return std::sqrt(val);
        return 0;
    }
    std::unique_ptr<Node> clone() const override { return std::make_unique<FunctionNode>(name, arg->clone()); }
};

// ============================================================================
// 3. LEXER (Implicit Multiplication Logic)
// ============================================================================
std::vector<std::string> tokenize(std::string src) {
    std::vector<std::string> tokens;
    for (size_t i = 0; i < src.size(); ++i) {
        if (isspace(src[i])) continue;

        // IMPLICIT MULTIPLICATION: If we have "2t" or "5(t)", insert a "*"
        if (!tokens.empty()) {
            std::string last = tokens.back();
            bool lastIsVal = (isdigit(last[0]) || last == "t" || last == ")");
            bool currIsVal = (isdigit(src[i]) || src[i] == 't' || src[i] == '(' || isalpha(src[i]));
            
            if (lastIsVal && currIsVal) {
                tokens.push_back("*"); // Injects multiplication
            }
        }

        if (isdigit(src[i]) || src[i] == '.') {
            std::string n; while(i < src.size() && (isdigit(src[i]) || src[i] == '.')) n += src[i++]; i--;
            tokens.push_back(n);
        } else if (isalpha(src[i])) {
            std::string s; while(i < src.size() && isalpha(src[i])) s += src[i++]; i--;
            tokens.push_back(s);
        } else {
            tokens.push_back(std::string(1, src[i]));
        }
    }
    return tokens;
}

// ============================================================================
// 4. PARSER (Recursive Descent)
// ============================================================================
class Parser {
    std::vector<std::string> tokens;
    size_t pos = 0;

    std::unique_ptr<Node> parsePrimary() {
        std::string t = tokens[pos++];
        if (t == "exp" || t == "sqrt" || t == "ln") {
            pos++; // Skip '('
            auto node = std::make_unique<FunctionNode>(t, parseExpression());
            pos++; // Skip ')'
            return node;
        }
        if (t == "(") {
            auto node = parseExpression();
            pos++; // Skip ')'
            return node;
        }
        if (t == "t") return std::make_unique<VariableNode>("t");
        return std::make_unique<ConstantNode>(std::stod(t));
    }

public:
    Parser(std::vector<std::string> t) : tokens(t) {}
    
    std::unique_ptr<Node> parseExpression() {
        auto left = parsePrimary();
        while (pos < tokens.size() && tokens[pos] != ")") {
            std::string op = tokens[pos++];
            auto right = parsePrimary();
            left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right));
        }
        return left;
    }
};

// ============================================================================
// 5. RUNTIME LOOP
// ============================================================================
int main() {
    std::string input;
    std::cout << "--- Engineering Math Verifier ---\n";
    std::cout << "Enter expression (e.g., 2t + exp(t)) or 'exit':\n";

    while (true) {
        std::cout << "\nInput > ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        if (input.empty()) continue;

        try {
            // 1. Tokenize (Lexing)
            auto tokens = tokenize(input);
            
            // 2. Parse (Building the AST)
            Parser parser(tokens);
            auto root = parser.parseExpression();

            // 3. Results
            std::cout << "Parsed: " << root->toString() << "\n";
            std::cout << "Eval at t=2.0: " << root->eval(2.0) << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "Syntax Error: " << e.what() << " (Check parens/formatting)\n";
        }
    }
    return 0;
}
