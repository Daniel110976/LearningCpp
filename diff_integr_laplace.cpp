#include <iostream>
#include <string>
#include <memory>
#include <cctype>
#include <vector>
#include <stdexcept>

// Helper utility for Laplace factorial calculations
long long factorial(int n) {
    long long res = 1;
    for (int i = 2; i <= n; ++i) res *= i;
    return res;
}

// ==========================================================
// 1. ABSTRACT BASE NODE WITH CALCULA BLUEPRINTS
// ==========================================================
class Node {
public:
    virtual ~Node() {}
    virtual std::string str() const = 0;       
    virtual std::string diff() const = 0;      
    virtual std::string integrate() const = 0; 
    virtual std::string laplace() const = 0;   
    
    virtual bool isZero() const { return false; }
    virtual bool isOne() const { return false; }
    virtual bool isConstantNode() const { return false; }
};

// ==========================================================
// 2. CONCRETE MATH NODES (EFFC++ COMPLIANT)
// ==========================================================

class Constant : public Node {
    double value;
public:
    explicit Constant(double v) : value(v) {} // Initialized via list
    
    std::string str() const override {
        std::string s = std::to_string(value);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    std::string diff() const override { return "0"; }
    std::string integrate() const override { return isZero() ? "0" : "(" + str() + "*t)"; }
    std::string laplace() const override { return isZero() ? "0" : "(" + str() + " / s)"; }
    
    bool isZero() const override { return value == 0.0; }
    bool isOne() const override { return value == 1.0; }
    bool isConstantNode() const override { return true; }
};

class Variable : public Node {
public:
    Variable() {} 
    std::string str() const override { return "t"; }
    std::string diff() const override { return "1"; }
    std::string integrate() const override { return "((t^2) / 2)"; }
    std::string laplace() const override { return "(1 / (s^2))"; }
};

class Add : public Node {
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
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
    std::string integrate() const override {
        return "(" + left->integrate() + " + " + right->integrate() + ")";
    }
    std::string laplace() const override {
        return "(" + left->laplace() + " + " + right->laplace() + ")";
    }
};

class Sub : public Node {
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
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
    std::string integrate() const override {
        return "(" + left->integrate() + " - " + right->integrate() + ")";
    }
    std::string laplace() const override {
        return "(" + left->laplace() + " - " + right->laplace() + ")";
    }
};

class Multiply : public Node {
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
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
        return "(" + term1 + " + " + term2 + ")";
    }
    std::string integrate() const override {
        if (left->isConstantNode()) return left->str() + " * (" + right->integrate() + ")";
        if (right->isConstantNode()) return right->str() + " * (" + left->integrate() + ")";
        return "Integral(" + str() + " dt)";
    }
    std::string laplace() const override {
        if (left->isConstantNode()) return left->str() + " * (" + right->laplace() + ")";
        if (right->isConstantNode()) return right->str() + " * (" + left->laplace() + ")";
        return "Laplace(" + str() + ")";
    }
};

class Divide : public Node {
    std::shared_ptr<Node> num;
    std::shared_ptr<Node> denom;
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
    std::string integrate() const override {
        if (denom->isConstantNode()) return "(" + num->integrate() + " / " + denom->str() + ")";
        return "Integral(" + str() + " dt)";
    }
    std::string laplace() const override {
        if (denom->isConstantNode()) return "(" + num->laplace() + " / " + denom->str() + ")";
        return "Laplace(" + str() + ")";
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
        if (b_diff == "1") return coeff + "*" + new_power;
        return coeff + "*" + new_power + "*" + b_diff;
    }
    std::string integrate() const override {
        if (base->str() == "t") {
            int new_exp = exponent + 1;
            return "((t^" + std::to_string(new_exp) + ") / " + std::to_string(new_exp) + ")";
        }
        return "Integral(" + str() + " dt)";
    }
    std::string laplace() const override {
        if (base->str() == "t") {
            long long fact_val = factorial(exponent);
            int new_exp = exponent + 1;
            return "(" + std::to_string(fact_val) + " / (s^" + std::to_string(new_exp) + "))";
        }
        return "Laplace(" + str() + ")";
    }
};

class Asin : public Node {
    std::shared_ptr<Node> arg;
public:
    explicit Asin(std::shared_ptr<Node> a) : arg(a) {}
    std::string str() const override { return "asin(" + arg->str() + ")"; }
    std::string diff() const override {
        std::string a_diff = arg->diff();
        if (a_diff == "0") return "0";
        return "(" + a_diff + " / sqrt(1 - (" + arg->str() + ")^2))";
    }
    std::string integrate() const override { return "Integral(" + str() + " dt)"; }
    std::string laplace() const override { return "Laplace(" + str() + ")"; }
};

class Acos : public Node {
    std::shared_ptr<Node> arg;
public:
    explicit Acos(std::shared_ptr<Node> a) : arg(a) {}
    std::string str() const override { return "acos(" + arg->str() + ")"; }
    std::string diff() const override {
        std::string a_diff = arg->diff();
        if (a_diff == "0") return "0";
        return "(-" + a_diff + " / sqrt(1 - (" + arg->str() + ")^2))";
    }
    std::string integrate() const override { return "Integral(" + str() + " dt)"; }
    std::string laplace() const override { return "Laplace(" + str() + ")"; }
};

class Atan : public Node {
    std::shared_ptr<Node> arg;
public:
    explicit Atan(std::shared_ptr<Node> a) : arg(a) {}
    std::string str() const override { return "atan(" + arg->str() + ")"; }
    std::string diff() const override {
        std::string a_diff = arg->diff();
        if (a_diff == "0") return "0";
        return "(" + a_diff + " / (1 + (" + arg->str() + ")^2))";
    }
    std::string integrate() const override { return "Integral(" + str() + " dt)"; }
    std::string laplace() const override { return "Laplace(" + str() + ")"; }
};

// ==========================================================
// 3. LEXER AND PARSER ENGINE
// ==========================================================
enum TokenType { NUM, VAR, PLUS, MINUS, MUL, DIV, POW, OPEN_P, CLOSE_P, FUNC_ASIN, FUNC_ACOS, FUNC_ATAN, END };

struct Token {
    TokenType type;
    std::string value;

    // Fixed: Member initialization list for struct fields
    Token() : type(END), value("") {}
    Token(TokenType t, const std::string& v) : type(t), value(v) {}
};

class Parser {
    std::string src;
    size_t idx;
    Token currentToken;

    void advance() {
        while (idx < src.length() && std::isspace(src[idx])) idx++;
        if (idx >= src.length()) { currentToken = Token(END, ""); return; }

        char c = src[idx];
        if (std::isdigit(c) || c == '.') {
            std::string val;
            while (idx < src.length() && (std::isdigit(src[idx]) || src[idx] == '.')) val += src[idx++];
            currentToken = Token(NUM, val);
        } else if (std::isalpha(c)) {
            std::string name;
            while (idx < src.length() && std::isalpha(src[idx])) name += src[idx++];
            if (name == "t") currentToken = Token(VAR, "t");
            else if (name == "asin") currentToken = Token(FUNC_ASIN, "asin");
            else if (name == "acos") currentToken = Token(FUNC_ACOS, "acos");
            else if (name == "atan") currentToken = Token(FUNC_ATAN, "atan");
            else throw std::runtime_error("Unknown identifier: " + name);
        } else {
            idx++;
            if (c == '+') currentToken = Token(PLUS, "+");
            else if (c == '-') currentToken = Token(MINUS, "-");
            else if (c == '*') currentToken = Token(MUL, "*");
            else if (c == '/') currentToken = Token(DIV, "/");
            else if (c == '^') currentToken = Token(POW, "^");
            else if (c == '(') currentToken = Token(OPEN_P, "(");
            else if (c == ')') currentToken = Token(CLOSE_P, ")");
            else throw std::runtime_error(std::string("Invalid character: ") + c);
        }
    }

    std::shared_ptr<Node> parseBase() {
        Token t = currentToken;
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
            if (currentToken.type != CLOSE_P) throw std::runtime_error("Missing closing parenthesis");
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
            if (currentToken.type != NUM) throw std::runtime_error("Exponents must be explicit constants");
            int exp = std::stoi(currentToken.value);
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
    // Fixed: All members explicitly mapped out to clear the effc++ flag
    Parser() : src(""), idx(0), currentToken() {}

    std::shared_ptr<Node> parse(const std::string& inputStr) {
        src = inputStr;
        idx = 0;
        advance();
        auto result = parseExpression();
        if (currentToken.type != END) throw std::runtime_error("Unexpected trailing text");
        return result;
    }
};

// ==========================================================
// 4. MAIN INTERACTIVE EXECUTION LOOP
// ==========================================================
int main() {
    Parser calculusEngine;
    std::string userInput;

    std::cout << "=========================================================\n";
    std::cout << "   MULTI-DOMAIN CALCULUS ENGINE (DIFF, INT, LAPLACE)     \n";
    std::cout << "=========================================================\n";
    std::cout << " Rules:\n";
    std::cout << "  - Use 't' as the independent variable.\n";
    std::cout << "  - Type explicit operators: *, /, +, -, ^\n";
    std::cout << "  - Functions available: asin(), acos(), atan()\n";
    std::cout << "  - Example: (5*t^3) + atan(t/3)\n";
    std::cout << "=========================================================\n\n";

    while (true) {
        std::cout << "Enter function f(t) = (or type 'quit' to exit): ";
        std::getline(std::cin, userInput);
        if (userInput == "quit" || userInput == "exit") break;
        if (userInput.empty()) continue;

        try {
            auto expressionTree = calculusEngine.parse(userInput);
            
            std::cout << "\n---------------------------------------------------------\n";
            std::cout << "Interpreted Form  : f(t) = " << expressionTree->str() << "\n\n";
            std::cout << "1. Derivative     : d/dt = " << expressionTree->diff() << "\n";
            std::cout << "2. Integral       : S dt = " << expressionTree->integrate() << " + C\n";
            std::cout << "3. Laplace Trans. : L{}  = " << expressionTree->laplace() << "\n";
            std::cout << "---------------------------------------------------------\n\n";
        } 
        catch (const std::exception& error) {
            std::cerr << "\n>>> Syntax Error: " << error.what() << "\n\n";
        }
    }
    return 0;
}
