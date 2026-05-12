#include <iostream>
#include <string>
#include <memory>
#include <cctype>
#include <cmath>
#include <vector>
#include <stdexcept>

// ==========================================
// 1. EXPRESSION TREE NODES WITH SIMPLIFICATION
// ==========================================

class Node {
public:
    virtual ~Node() {}
    virtual std::string str() const = 0;
    virtual std::string diff() const = 0;
    virtual bool isZero() const { return false; }
    virtual bool isOne() const { return false; }
};

class Constant : public Node {
    double value;
public:
    Constant(double v) : value(v) {}
    std::string str() const override {
        std::string s = std::to_string(value);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    std::string diff() const override { return "0"; }
    bool isZero() const override { return value == 0.0; }
    bool isOne() const override { return value == 1.0; }
};

class Variable : public Node {
public:
    std::string str() const override { return "x"; }
    std::string diff() const override { return "1"; }
};

class Add : public Node {
    std::shared_ptr<Node> left, right;
public:
    Add(std::shared_ptr<Node> l, std::shared_ptr<Node> r) : left(l), right(r) {}
    std::string str() const override { return "(" + left->str() + " + " + right->str() + ")"; }
    
    std::string diff() const override {
        std::string l_diff = left->diff();
        std::string r_diff = right->diff();
        if (l_diff == "0") return r_diff;
        if (r_diff == "0") return l_diff;
        return "(" + l_diff + " + " + r_diff + ")";
    }
};

class Sub : public Node {
    std::shared_ptr<Node> left, right;
public:
    Sub(std::shared_ptr<Node> l, std::shared_ptr<Node> r) : left(l), right(r) {}
    std::string str() const override { return "(" + left->str() + " - " + right->str() + ")"; }
    
    std::string diff() const override {
        std::string l_diff = left->diff();
        std::string r_diff = right->diff();
        if (r_diff == "0") return l_diff;
        if (l_diff == "0") return "(-" + r_diff + ")";
        return "(" + l_diff + " - " + r_diff + ")";
    }
};

class Multiply : public Node {
    std::shared_ptr<Node> left, right;
public:
    Multiply(std::shared_ptr<Node> l, std::shared_ptr<Node> r) : left(l), right(r) {}
    std::string str() const override { return "(" + left->str() + " * " + right->str() + ")"; }
    
    std::string diff() const override {
        std::string l_diff = left->diff();
        std::string r_diff = right->diff();
        
        bool term1_zero = (r_diff == "0" || left->isZero());
        bool term2_zero = (l_diff == "0" || right->isZero());
        
        std::string term1 = term1_zero ? "0" : (left->isOne() ? r_diff : left->str() + "*" + r_diff);
        std::string term2 = term2_zero ? "0" : (right->isOne() ? l_diff : right->str() + "*" + l_diff);
        
        if (term1 == "0" && term2 == "0") return "0";
        if (term1 == "0") return term2;
        if (term2 == "0") return term1;
        return term1 + " + " + term2;
    }
};

class Divide : public Node {
    std::shared_ptr<Node> num, denom;
public:
    Divide(std::shared_ptr<Node> n, std::shared_ptr<Node> d) : num(n), denom(d) {}
    std::string str() const override { return "(" + num->str() + " / " + denom->str() + ")"; }
    
    std::string diff() const override {
        std::string n_diff = num->diff();
        std::string d_diff = denom->diff();
        
        std::string u_prime_v = (n_diff == "0") ? "0" : (denom->str() + "*" + n_diff);
        std::string u_v_prime = (d_diff == "0") ? "0" : (num->str() + "*" + d_diff);
        
        std::string numerator;
        if (u_prime_v == "0" && u_v_prime == "0") return "0";
        if (u_v_prime == "0") numerator = u_prime_v;
        else if (u_prime_v == "0") numerator = "-" + u_v_prime;
        else numerator = "(" + u_prime_v + " - " + u_v_prime + ")";
        
        return "(" + numerator + " / (" + denom->str() + "^2))";
    }
};

class Power : public Node {
    std::shared_ptr<Node> base;
    int exponent;
public:
    Power(std::shared_ptr<Node> b, int e) : base(b), exponent(e) {}
    std::string str() const override { return "(" + base->str() + "^" + std::to_string(exponent) + ")"; }
    
    std::string diff() const override {
        if (exponent == 0) return "0";
        std::string b_diff = base->diff();
        if (b_diff == "0") return "0";
        
        std::string coeff = std::to_string(exponent);
        std::string new_power = (exponent - 1 == 1) ? base->str() : "(" + base->str() + "^" + std::to_string(exponent - 1) + ")";
        
        std::string logic = coeff + "*" + new_power;
        if (b_diff == "1") return logic;
        return logic + "*" + b_diff;
    }
};

class Asin : public Node {
    std::shared_ptr<Node> arg;
public:
    Asin(std::shared_ptr<Node> a) : arg(a) {}
    std::string str() const override { return "asin(" + arg->str() + ")"; }
    std::string diff() const override {
        std::string a_diff = arg->diff();
        if (a_diff == "0") return "0";
        return "(" + a_diff + " / std::sqrt(1 - (" + arg->str() + ")^2))";
    }
};

class Acos : public Node {
    std::shared_ptr<Node> arg;
public:
    Acos(std::shared_ptr<Node> a) : arg(a) {}
    std::string str() const override { return "acos(" + arg->str() + ")"; }
    std::string diff() const override {
        std::string a_diff = arg->diff();
        if (a_diff == "0") return "0";
        return "(-" + a_diff + " / std::sqrt(1 - (" + arg->str() + ")^2))";
    }
};

class Atan : public Node {
    std::shared_ptr<Node> arg;
public:
    Atan(std::shared_ptr<Node> a) : arg(a) {}
    std::string str() const override { return "atan(" + arg->str() + ")"; }
    std::string diff() const override {
        std::string a_diff = arg->diff();
        if (a_diff == "0") return "0";
        return "(" + a_diff + " / (1 + (" + arg->str() + ")^2))";
    }
};

// ==========================================
// 2. TEXT PARSER ENGINE (LEXER & PARSER)
// ==========================================

enum TokenType { NUM, VAR, PLUS, MINUS, MUL, DIV, POW, OPEN_P, CLOSE_P, FUNC_ASIN, FUNC_ACOS, FUNC_ATAN, END };

struct Token {
    TokenType type;
    std::string value;
    Token(TokenType type = END, const std::string& value = "")
        : type(type), value(value) {}
};

class Parser {
    std::string src = "";
    size_t idx = 0;
    Token currentToken{END, ""};

    void advance() {
        while (idx < src.length() && std::isspace(src[idx])) idx++;
        if (idx >= src.length()) { currentToken = { END, "" }; return; }

        char c = src[idx];
        if (std::isdigit(c) || c == '.') {
            std::string val;
            while (idx < src.length() && (std::isdigit(src[idx]) || src[idx] == '.')) val += src[idx++];
            currentToken = { NUM, val };
        } else if (std::isalpha(c)) {
            std::string name;
            while (idx < src.length() && std::isalpha(src[idx])) name += src[idx++];
            if (name == "x") currentToken = { VAR, "x" };
            else if (name == "asin") currentToken = { FUNC_ASIN, "asin" };
            else if (name == "acos") currentToken = { FUNC_ACOS, "acos" };
            else if (name == "atan") currentToken = { FUNC_ATAN, "atan" };
            else throw std::runtime_error("Unknown identifier: " + name);
        } else {
            idx++;
            if (c == '+') currentToken = { PLUS, "+" };
            else if (c == '-') currentToken = { MINUS, "-" };
            else if (c == '*') currentToken = { MUL, "*" };
            else if (c == '/') currentToken = { DIV, "/" };
            else if (c == '^') currentToken = { POW, "^" };
            else if (c == '(') currentToken = { OPEN_P, "(" };
            else if (c == ')') currentToken = { CLOSE_P, ")" };
            else throw std::runtime_error(std::string("Invalid character: ") + c);
        }
    }

    std::shared_ptr<Node> parseBase() {
        Token t = currentToken;
        if (t.type == PLUS || t.type == MINUS) {
            bool negative = (t.type == MINUS);
            advance();
            auto node = parseBase();
            if (negative) {
                return std::make_shared<Multiply>(std::make_shared<Constant>(-1.0), node);
            }
            return node;
        }
        if (t.type == NUM) { advance(); return std::make_shared<Constant>(std::stod(t.value)); }
        if (t.type == VAR) { advance(); return std::make_shared<Variable>(); }
        if (t.type == OPEN_P) {
            advance();
            auto node = parseExpression();
            if (currentToken.type != CLOSE_P) throw std::runtime_error("Missing closing parenthesis");
            advance();
            return node;
        }
        if (t.type == FUNC_ASIN || t.type == FUNC_ACOS || t.type == FUNC_ATAN) {
            advance();
            if (currentToken.type != OPEN_P) throw std::runtime_error("Expected '(' after function name");
            advance();
            auto arg = parseExpression();
            if (currentToken.type != CLOSE_P) throw std::runtime_error("Missing closing parenthesis after argument");
            advance();
            if (t.type == FUNC_ASIN) return std::make_shared<Asin>(arg);
            if (t.type == FUNC_ACOS) return std::make_shared<Acos>(arg);
            return std::make_shared<Atan>(arg);
        }
        throw std::runtime_error("Unexpected token in expression");
    }

    std::shared_ptr<Node> parseFactor() {
        auto node = parseBase();
        if (currentToken.type == POW) {
            advance();
            bool negativeExp = false;
            if (currentToken.type == PLUS || currentToken.type == MINUS) {
                negativeExp = (currentToken.type == MINUS);
                advance();
            }
            if (currentToken.type != NUM) throw std::runtime_error("Exponents must be explicit constants");
            int exp = std::stoi(currentToken.value);
            if (negativeExp) exp = -exp;
            advance();
            node = std::make_shared<Power>(node, exp);
        }
        return node;
    }

    std::shared_ptr<Node> parseTerm() {
        auto node = parseFactor();
        while (currentToken.type == MUL || currentToken.type == DIV) {
            TokenType op = currentToken.type;
            advance();
            auto nextFactor = parseFactor();
            if (op == MUL) node = std::make_shared<Multiply>(node, nextFactor);
            else node = std::make_shared<Divide>(node, nextFactor);
        }
        return node;
    }

    std::shared_ptr<Node> parseExpression() {
        auto node = parseTerm();
        while (currentToken.type == PLUS || currentToken.type == MINUS) {
            TokenType op = currentToken.type;
            advance();
            auto nextTerm = parseTerm();
            if (op == PLUS) node = std::make_shared<Add>(node, nextTerm);
            else node = std::make_shared<Sub>(node, nextTerm);
        }
        return node;
    }

public:
    std::shared_ptr<Node> parse(const std::string& inputStr) {
        src = inputStr;
        idx = 0;
        advance();
        auto result = parseExpression();
        if (currentToken.type != END) throw std::runtime_error("Unexpected text trailing at end of function");
        return result;
    }
};

// ==========================================
// 3. MAIN INTERACTIVE EXECUTION LOOP
// ==========================================

int main() {
    Parser executionEngine;
    std::string userInput;

    std::cout << "==================================================\n";
    std::cout << "    UNIVERSAL SYMBOLIC DIFFERENTIATION ENGINE     \n";
    std::cout << "==================================================\n";
    std::cout << "Rules:\n";
    std::cout << " - Variable must be explicitly written as 'x'\n";
    std::cout << " - Operators must be written explicitly (*, /, +, -, ^)\n";
    std::cout << " - Supported: asin(), acos(), atan()\n";
    std::cout << " Example inputs: (x^2)*asin(x/2)  |  atan(x/3)  |  5*x\n";
    std::cout << "==================================================\n\n";

    while (true) {
        std::cout << "Enter function y = (or type 'quit' to exit): ";
        std::getline(std::cin, userInput);
        if (userInput == "quit" || userInput == "exit") break;
        if (userInput.empty()) continue;

        try {
            auto expressionTree = executionEngine.parse(userInput);
            
            std::cout << "\n--------------------------------------------------\n";
            std::cout << "Interpreted Function:  y = " << expressionTree->str() << "\n";
            std::cout << "Calculated Derivative: dy/dx = " << expressionTree->diff() << "\n";
            std::cout << "--------------------------------------------------\n\n";
        } 
        catch (const std::exception& error) {
            std::cerr << "\n>>> Parsing Error: " << error.what() << "\n";
            std::cerr << ">>> Check your syntax and try again.\n\n";
        }
    }

    return 0;
}
