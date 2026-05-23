#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cmath>
#include <stdexcept>

// ============================================================================
// 1. FORWARD DECLARATIONS & SYSTEM HELPERS
// ============================================================================

class Node;
class SymbolicNode;
class ConstantNode;
class VariableNode;
class FunctionNode;
class BinaryOpNode;

int getPrecedence(const std::string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*" || op == "/") return 2;
    if (op == "^") return 3;
    return 4; 
}

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// ============================================================================
// 2. CLASS INTERFACE DEFINITIONS (WITH -Weffc++ FIXES)
// ============================================================================

class Node {
public:
    virtual ~Node() = default;
    virtual std::string toString() = 0;
    virtual Node* diff() = 0;
    virtual Node* integrate() = 0;
    virtual Node* laplace() = 0;
    virtual Node* simplify() = 0;
    virtual Node* clone() const = 0;

    virtual bool isConstant() const { return false; }
    virtual bool isVariable() const { return false; }
    virtual bool isBinaryOp() const { return false; }
    virtual double getValue() const { return 0.0; }
    virtual std::string getOp() const { return ""; }
    virtual std::string getFuncName() const { return ""; }
    virtual Node* getArg() const { return nullptr; }
    virtual Node* getLeft() const { return nullptr; }
    virtual Node* getRight() const { return nullptr; }
};

class SymbolicNode : public Node {
private:
    std::string expression;
public:
    SymbolicNode(std::string expr) : expression(expr) {}
    std::string toString() override { return expression; }
    Node* clone() const override { return new SymbolicNode(expression); }
    Node* simplify() override { return clone(); }
    Node* diff() override { return new SymbolicNode("d/dt(" + expression + ")"); }
    Node* integrate() override { return new SymbolicNode("Integral(" + expression + " dt)"); }
    Node* laplace() override { return new SymbolicNode("Laplace(" + expression + ")"); }
};

class ConstantNode : public Node {
private:
    double value;
public:
    ConstantNode(double val) : value(val) {}
    bool isConstant() const override { return true; }
    double getValue() const override { return value; }
    
    std::string toString() override;
    Node* clone() const override;
    Node* simplify() override;
    Node* diff() override;
    Node* integrate() override;
    Node* laplace() override;
};

class VariableNode : public Node {
private:
    std::string name;
public:
    VariableNode(std::string nm) : name(nm) {}
    bool isVariable() const override { return true; }
    
    std::string toString() override;
    Node* clone() const override;
    Node* simplify() override;
    Node* diff() override;
    Node* integrate() override;
    Node* laplace() override;
};

class FunctionNode : public Node {
private:
    std::string name;
    Node* arg;
public:
    FunctionNode(std::string nm, Node* a) : name(nm), arg(a) {}
    FunctionNode(const FunctionNode&) = delete;
    FunctionNode& operator=(const FunctionNode&) = delete;
    ~FunctionNode() { delete arg; }
    
    // Explicitly disable copy semantics to satisfy -Weffc++ for pointer-holding classes


    std::string getFuncName() const override { return name; }
    Node* getArg() const override { return arg; }
    
    std::string toString() override;
    Node* clone() const override;
    Node* simplify() override;
    Node* diff() override;
    Node* integrate() override;
    Node* laplace() override;
};

class BinaryOpNode : public Node {
private:
    std::string op;
    Node* left;
    Node* right;
public:
    BinaryOpNode(std::string operation, Node* l, Node* r) : op(operation), left(l), right(r) {}
    BinaryOpNode(const BinaryOpNode&) = delete;
    BinaryOpNode& operator=(const BinaryOpNode&) = delete;
    ~BinaryOpNode() { delete left; delete right; }

    bool isBinaryOp() const override { return true; }
    std::string getOp() const override { return op; }
    Node* getLeft() const override { return left; }
    Node* getRight() const override { return right; }
    
    std::string toString() override;
    Node* clone() const override;
    Node* simplify() override;
    Node* diff() override;
    Node* integrate() override;
    Node* laplace() override;
};

// ============================================================================
// 3. DEFERRED METHOD IMPLEMENTATIONS (SAFE CIRCULAR REFS)
// ============================================================================

// --- ConstantNode ---
std::string ConstantNode::toString() {
    if (value == static_cast<int>(value)) return std::to_string(static_cast<int>(value));
    return std::to_string(value);
}
Node* ConstantNode::clone() const { return new ConstantNode(value); }
Node* ConstantNode::simplify() { return clone(); }
Node* ConstantNode::diff() { return new ConstantNode(0); }
Node* ConstantNode::integrate() { return new BinaryOpNode("*", new ConstantNode(value), new VariableNode("t")); }
Node* ConstantNode::laplace() { return new BinaryOpNode("/", new ConstantNode(value), new VariableNode("s")); }

// --- VariableNode ---
std::string VariableNode::toString() { return name; }
Node* VariableNode::clone() const { return new VariableNode(name); }
Node* VariableNode::simplify() { return clone(); }
Node* VariableNode::diff() { return new ConstantNode(name == "t" ? 1 : 0); }
Node* VariableNode::integrate() { 
    if (name == "t") {
        return new BinaryOpNode("/", new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(2)), new ConstantNode(2));
    }
    return new BinaryOpNode("*", new VariableNode(name), new VariableNode("t"));
}
Node* VariableNode::laplace() { 
    if (name == "t") {
        return new BinaryOpNode("/", new ConstantNode(1), new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(2)));
    }
    return new BinaryOpNode("/", new VariableNode(name), new VariableNode("s"));
}

// --- FunctionNode ---
std::string FunctionNode::toString() { return name + "(" + arg->toString() + ")"; }
Node* FunctionNode::clone() const { return new FunctionNode(name, arg->clone()); }
Node* FunctionNode::simplify() { return new FunctionNode(name, arg->simplify()); }
Node* FunctionNode::diff() {
    if (name == "sin") return new BinaryOpNode("*", new FunctionNode("cos", arg->clone()), arg->diff());
    if (name == "cos") return new BinaryOpNode("*", new BinaryOpNode("*", new ConstantNode(-1), new FunctionNode("sin", arg->clone())), arg->diff());
    if (name == "tan") return new BinaryOpNode("*", new BinaryOpNode("/", new ConstantNode(1), new BinaryOpNode("^", new FunctionNode("cos", arg->clone()), new ConstantNode(2))), arg->diff());
    if (name == "atan") return new BinaryOpNode("/", arg->diff(), new BinaryOpNode("+", new ConstantNode(1), new BinaryOpNode("^", arg->clone(), new ConstantNode(2))));
    if (name == "asin") return new BinaryOpNode("/", arg->diff(), new FunctionNode("sqrt", new BinaryOpNode("-", new ConstantNode(1), new BinaryOpNode("^", arg->clone(), new ConstantNode(2)))));
    if (name == "acos") return new BinaryOpNode("/", new BinaryOpNode("*", new ConstantNode(-1), arg->diff()), new FunctionNode("sqrt", new BinaryOpNode("-", new ConstantNode(1), new BinaryOpNode("^", arg->clone(), new ConstantNode(2)))));
    return new SymbolicNode("d/dt(" + this->toString() + ")");
}
Node* FunctionNode::integrate() {
    double b = 1.0; bool valid = false;
    if (arg->isVariable() && arg->toString() == "t") { b = 1.0; valid = true; }
    else if (arg->isBinaryOp() && arg->getOp() == "*") {
        if (arg->getLeft()->isConstant() && arg->getRight()->isVariable() && arg->getRight()->toString() == "t") { b = arg->getLeft()->getValue(); valid = true; }
        else if (arg->getRight()->isConstant() && arg->getLeft()->isVariable() && arg->getLeft()->toString() == "t") { b = arg->getRight()->getValue(); valid = true; }
    }
    if (valid) {
        if (name == "sin") return new BinaryOpNode("*", new ConstantNode(-1.0 / b), new FunctionNode("cos", arg->clone()));
        if (name == "cos") return new BinaryOpNode("*", new ConstantNode(1.0 / b), new FunctionNode("sin", arg->clone()));
        if (name == "tan") return new BinaryOpNode("*", new ConstantNode(-1.0 / b), new FunctionNode("ln", new FunctionNode("cos", arg->clone())));
    }
    return new SymbolicNode("Integral(" + this->toString() + " dt)");
}
Node* FunctionNode::laplace() {
    double b = 1.0; bool valid = false;
    if (arg->isVariable() && arg->toString() == "t") { b = 1.0; valid = true; }
    else if (arg->isBinaryOp() && arg->getOp() == "*") {
        if (arg->getLeft()->isConstant() && arg->getRight()->isVariable() && arg->getRight()->toString() == "t") { b = arg->getLeft()->getValue(); valid = true; }
        else if (arg->getRight()->isConstant() && arg->getLeft()->isVariable() && arg->getLeft()->toString() == "t") { b = arg->getRight()->getValue(); valid = true; }
    }
    if (valid) {
        if (name == "sin") return new BinaryOpNode("/", new ConstantNode(b), new BinaryOpNode("+", new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(2)), new ConstantNode(b * b)));
        if (name == "cos") return new BinaryOpNode("/", new VariableNode("s"), new BinaryOpNode("+", new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(2)), new ConstantNode(b * b)));
    }
    return new SymbolicNode("Laplace(" + this->toString() + ")");
}

// --- BinaryOpNode ---
Node* BinaryOpNode::clone() const { return new BinaryOpNode(op, left->clone(), right->clone()); }
std::string BinaryOpNode::toString() {
    std::string leftStr = left->toString(); std::string rightStr = right->toString();
    if (left->isBinaryOp() && getPrecedence(left->getOp()) < getPrecedence(op)) leftStr = "(" + leftStr + ")";
    if (right->isBinaryOp() && getPrecedence(right->getOp()) <= getPrecedence(op)) rightStr = "(" + rightStr + ")";
    return leftStr + " " + op + " " + rightStr;
}
Node* BinaryOpNode::simplify() {
    Node* simpLeft = left->simplify(); Node* simpRight = right->simplify();
    if (simpLeft->isConstant() && simpRight->isConstant()) {
        double valL = simpLeft->getValue(); double valR = simpRight->getValue();
        delete simpLeft; delete simpRight;
        if (op == "+") return new ConstantNode(valL + valR);
        if (op == "-") return new ConstantNode(valL - valR);
        if (op == "*") return new ConstantNode(valL * valR);
        if (op == "/") { if (valR != 0) return new ConstantNode(valL / valR); }
        if (op == "^") return new ConstantNode(std::pow(valL, valR));
    }
    if (op == "*" && simpRight->isConstant() && simpRight->getValue() == 1) { delete simpRight; return simpLeft; }
    if (op == "*" && simpLeft->isConstant() && simpLeft->getValue() == 1) { delete simpLeft; return simpRight; }
    if (op == "*" && ((simpRight->isConstant() && simpRight->getValue() == 0) || (simpLeft->isConstant() && simpLeft->getValue() == 0))) {
        delete simpLeft; delete simpRight; return new ConstantNode(0);
    }
    if (op == "+" && simpRight->isConstant() && simpRight->getValue() == 0) { delete simpRight; return simpLeft; }
    if (op == "+" && simpLeft->isConstant() && simpLeft->getValue() == 0) { delete simpLeft; return simpRight; }
    if (op == "-" && simpRight->isConstant() && simpRight->getValue() == 0) { delete simpRight; return simpLeft; }
    return new BinaryOpNode(op, simpLeft, simpRight);
}
Node* BinaryOpNode::diff() {
    if (op == "+" || op == "-") return new BinaryOpNode(op, left->diff(), right->diff());
    if (op == "*") return new BinaryOpNode("+", new BinaryOpNode("*", left->diff(), right->clone()), new BinaryOpNode("*", left->clone(), right->diff()));
    if (op == "/") return new BinaryOpNode("/", new BinaryOpNode("-", new BinaryOpNode("*", left->diff(), right->clone()), new BinaryOpNode("*", left->clone(), right->diff())), new BinaryOpNode("^", right->clone(), new ConstantNode(2)));
    if (op == "^") {
        if (left->isVariable() && left->toString() == "e") return new BinaryOpNode("*", this->clone(), right->diff());
        if (left->isVariable() && left->toString() == "t" && right->isConstant()) {
            return new BinaryOpNode("*", new ConstantNode(right->getValue()), new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(right->getValue() - 1)));
        }
    }
    return new SymbolicNode("d/dt(" + this->toString() + ")");
}
Node* BinaryOpNode::integrate() {
    if (op == "+" || op == "-") return new BinaryOpNode(op, left->integrate(), right->integrate());
    if (op == "*" && left->isConstant()) return new BinaryOpNode("*", left->clone(), right->integrate());
    if (op == "*" && right->isConstant()) return new BinaryOpNode("*", left->integrate(), right->clone());
    if (op == "/") return new BinaryOpNode("/", left->integrate(), right->clone());
    if (op == "^") {
        if (left->isVariable() && left->toString() == "t" && right->isConstant()) {
            return new BinaryOpNode("/", new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(right->getValue() + 1)), new ConstantNode(right->getValue() + 1));
        }
        if (left->isVariable() && left->toString() == "e") {
            double a = 1.0; bool valid = false;
            if (right->isVariable() && right->toString() == "t") { a = 1.0; valid = true; }
            else if (right->isBinaryOp() && right->getOp() == "*") {
                if (right->getLeft()->isConstant() && right->getRight()->isVariable() && right->getRight()->toString() == "t") { a = right->getLeft()->getValue(); valid = true; }
                if (right->getRight()->isConstant() && right->getLeft()->isVariable() && right->getLeft()->toString() == "t") { a = right->getRight()->getValue(); valid = true; }
            }
            if (valid) return new BinaryOpNode("/", this->clone(), new ConstantNode(a));
        }
    }
    return new SymbolicNode("Integral(" + this->toString() + " dt)");
}
Node* BinaryOpNode::laplace() {
    if (op == "+" || op == "-") return new BinaryOpNode(op, left->laplace(), right->laplace());
    if (op == "*" && left->isConstant()) return new BinaryOpNode("*", left->clone(), right->laplace());
    if (op == "*" && right->isConstant()) return new BinaryOpNode("*", left->laplace(), right->clone());
    if (op == "/") return new BinaryOpNode("/", left->laplace(), right->clone());
    if (op == "*") {
        double a = 0.0, b = 0.0;
        bool leftExp = false, rightExp = false, leftSin = false, rightSin = false, leftCos = false, rightCos = false;
        bool leftT = (left->isVariable() && left->toString() == "t");
        bool rightT = (right->isVariable() && right->toString() == "t");
        if (left->isBinaryOp() && left->getOp() == "^" && left->getLeft()->isVariable() && left->getLeft()->toString() == "e") {
            Node* r = left->getRight();
            if (r->isVariable() && r->toString() == "t") { a = 1.0; leftExp = true; }
            else if (r->isBinaryOp() && r->getOp() == "*") {
                if (r->getLeft()->isConstant() && r->getRight()->isVariable() && r->getRight()->toString() == "t") { a = r->getLeft()->getValue(); leftExp = true; }
                if (r->getRight()->isConstant() && r->getLeft()->isVariable() && r->getLeft()->toString() == "t") { a = r->getRight()->getValue(); leftExp = true; }
            }
        }
        if (left->getFuncName() == "sin") {
            Node* arg = left->getArg();
            if (arg->isVariable() && arg->toString() == "t") { b = 1.0; leftSin = true; }
            else if (arg->isBinaryOp() && arg->getOp() == "*") {
                if (arg->getLeft()->isConstant() && arg->getRight()->isVariable() && arg->getRight()->toString() == "t") { b = arg->getLeft()->getValue(); leftSin = true; }
                if (arg->getRight()->isConstant() && arg->getLeft()->isVariable() && arg->getLeft()->toString() == "t") { b = arg->getRight()->getValue(); leftSin = true; }
            }
        }
        if (left->getFuncName() == "cos") {
            Node* arg = left->getArg();
            if (arg->isVariable() && arg->toString() == "t") { b = 1.0; leftCos = true; }
            else if (arg->isBinaryOp() && arg->getOp() == "*") {
                if (arg->getLeft()->isConstant() && arg->getRight()->isVariable() && arg->getRight()->toString() == "t") { b = arg->getLeft()->getValue(); leftCos = true; }
                if (arg->getRight()->isConstant() && arg->getLeft()->isVariable() && arg->getLeft()->toString() == "t") { b = arg->getRight()->getValue(); leftCos = true; }
            }
        }
        if (right->isBinaryOp() && right->getOp() == "^" && right->getLeft()->isVariable() && right->getLeft()->toString() == "e") {
            Node* r = right->getRight();
            if (r->isVariable() && r->toString() == "t") { a = 1.0; rightExp = true; }
            else if (r->isBinaryOp() && r->getOp() == "*") {
                if (r->getLeft()->isConstant() && r->getRight()->isVariable() && r->getRight()->toString() == "t") { a = r->getLeft()->getValue(); rightExp = true; }
                if (r->getRight()->isConstant() && r->getLeft()->isVariable() && r->getLeft()->toString() == "t") { a = r->getRight()->getValue(); rightExp = true; }
            }
        }
        if (right->getFuncName() == "sin") {
            Node* arg = right->getArg();
            if (arg->isVariable() && arg->toString() == "t") { b = 1.0; rightSin = true; }
            else if (arg->isBinaryOp() && arg->getOp() == "*") {
                if (arg->getLeft()->isConstant() && arg->getRight()->isVariable() && arg->getRight()->toString() == "t") { b = arg->getLeft()->getValue(); rightSin = true; }
                if (arg->getRight()->isConstant() && arg->getLeft()->isVariable() && arg->getLeft()->toString() == "t") { b = arg->getRight()->getValue(); rightSin = true; }
            }
        }
        if (right->getFuncName() == "cos") {
            Node* arg = right->getArg();
            if (arg->isVariable() && arg->toString() == "t") { b = 1.0; rightCos = true; }
            else if (arg->isBinaryOp() && arg->getOp() == "*") {
                if (arg->getLeft()->isConstant() && arg->getRight()->isVariable() && arg->getRight()->toString() == "t") { b = arg->getLeft()->getValue(); rightCos = true; }
                if (arg->getRight()->isConstant() && arg->getLeft()->isVariable() && arg->getLeft()->toString() == "t") { b = arg->getRight()->getValue(); rightCos = true; }
            }
        }
        if ((leftExp && rightSin) || (rightExp && leftSin)) {
            return new BinaryOpNode("/", new ConstantNode(b), new BinaryOpNode("+", new BinaryOpNode("^", new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)), new ConstantNode(2)), new ConstantNode(b * b)));
        }
        if ((leftExp && rightCos) || (rightExp && leftCos)) {
            return new BinaryOpNode("/", new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)), new BinaryOpNode("+", new BinaryOpNode("^", new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)), new ConstantNode(2)), new ConstantNode(b * b)));
        }
        if ((leftT && rightExp) || (rightT && leftExp)) {
            return new BinaryOpNode("/", new ConstantNode(1), new BinaryOpNode("^", new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)), new ConstantNode(2)));
        }
    }
    if (op == "^") {
        if (left->isVariable() && left->toString() == "t" && right->isConstant()) {
            int power = static_cast<int>(right->getValue());
            return new BinaryOpNode("/", new ConstantNode(factorial(power)), new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(power + 1)));
        }
        if (left->isVariable() && left->toString() == "e") {
            double a = 1.0; Node* r = right;
            if (r->isVariable() && r->toString() == "t") a = 1.0;
            else if (r->isBinaryOp() && r->getOp() == "*") {
                if (r->getLeft()->isConstant() && r->getRight()->isVariable() && r->getRight()->toString() == "t") a = r->getLeft()->getValue();
                if (r->getRight()->isConstant() && r->getLeft()->isVariable() && r->getLeft()->toString() == "t") a = r->getRight()->getValue();
            }
            return new BinaryOpNode("/", new ConstantNode(1), new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)));
        }
    }
    return new SymbolicNode("Laplace(" + this->toString() + ")");
}

// ============================================================================
// 4. LEXER & PARSER TOKENS
// ============================================================================

enum TokenType { TOKEN_NUM, TOKEN_VAR, TOKEN_FUNC, TOKEN_OP, TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_EOF };
struct Token { TokenType type; std::string value; };

class Lexer {
private:
    std::string src; size_t pos = 0;
public:
    Lexer(std::string source) : src(source) {}
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < src.size()) {
            char ch = src[pos];
            if (std::isspace(ch)) { pos++; continue; }
            if (std::isdigit(ch) || ch == '.') {
                std::string num;
                while (pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.')) num += src[pos++];
                tokens.push_back({TOKEN_NUM, num}); continue;
            }
            if (std::isalpha(ch)) {
                std::string ident;
                while (pos < src.size() && std::isalpha(src[pos])) ident += src[pos++];
                if (ident == "t" || ident == "e" || ident == "s") tokens.push_back({TOKEN_VAR, ident});
                else if (ident == "sin" || ident == "cos" || ident == "tan" || ident == "asin" || ident == "acos" || ident == "atan") {
                    tokens.push_back({TOKEN_FUNC, ident});
                } else throw std::runtime_error("Unknown identifier: " + ident);
                continue;
            }
            if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
                tokens.push_back({TOKEN_OP, std::string(1, ch)}); pos++; continue;
            }
            if (ch == '(') { tokens.push_back({TOKEN_LPAREN, "("}); pos++; continue; }
            if (ch == ')') { tokens.push_back({TOKEN_RPAREN, ")"}); pos++; continue; }
            throw std::runtime_error("Illegal entry symbol: " + std::string(1, ch));
        }
        tokens.push_back({TOKEN_EOF, ""});
        return tokens;
    }
};

class Parser {
private:
    std::vector<Token> tokens; size_t index = 0;
    Token peek() { return tokens[index]; }
    Token consume() { return tokens[index++]; }
public:
    Parser(std::vector<Token> t) : tokens(t) {}
    Node* parse() { return parseExpression(); }

    Node* parseExpression() {
        Node* node = parseTerm();
        while (peek().type == TOKEN_OP && (peek().value == "+" || peek().value == "-")) {
            std::string op = consume().value;
            node = new BinaryOpNode(op, node, parseTerm());
        }
        return node;
    }

    Node* parseTerm() {
        Node* node = parseFactor();
        while (peek().type == TOKEN_OP && (peek().value == "*" || peek().value == "/")) {
            std::string op = consume().value;
            node = new BinaryOpNode(op, node, parseFactor());
        }
        return node;
    }

    Node* parseFactor() {
        Token tok = peek();
        if (tok.type == TOKEN_OP && tok.value == "+") { consume(); return parseFactor(); }
        if (tok.type == TOKEN_OP && tok.value == "-") {
            consume(); Node* factor = parseFactor();
            if (factor->isConstant()) {
                double val = factor->getValue(); delete factor;
                return new ConstantNode(-val);
            }
            return new BinaryOpNode("*", new ConstantNode(-1), factor);
        }
        if (tok.type == TOKEN_NUM) {
            consume(); Node* base = new ConstantNode(std::stod(tok.value));
            if (peek().type == TOKEN_OP && peek().value == "^") { consume(); base = new BinaryOpNode("^", base, parseFactor()); }
            return base;
        }
        if (tok.type == TOKEN_VAR) {
            consume(); Node* base = new VariableNode(tok.value);
            if (peek().type == TOKEN_OP && peek().value == "^") { consume(); base = new BinaryOpNode("^", base, parseFactor()); }
            return base;
        }
        if (tok.type == TOKEN_FUNC) {
            std::string name = consume().value;
            if (consume().type != TOKEN_LPAREN) throw std::runtime_error("Missing left parenthesis.");
            Node* arg = parseExpression();
            if (consume().type != TOKEN_RPAREN) throw std::runtime_error("Missing right parenthesis.");
            return new FunctionNode(name, arg);
        }
        if (tok.type == TOKEN_LPAREN) {
            consume(); Node* node = parseExpression();
            if (consume().type != TOKEN_RPAREN) throw std::runtime_error("Mismatched syntax parenthesis grouping.");
            if (peek().type == TOKEN_OP && peek().value == "^") { consume(); node = new BinaryOpNode("^", node, parseFactor()); }
            return node;
        }
        throw std::runtime_error("Engine parsing failure near token: " + tok.value);
    }
};

// ============================================================================
// 5. RUNTIME ENVIRONMENT MAIN
// ============================================================================

int main() {
    std::string input;
    std::cout << "=== Multi-Domain Math Engine (Unified Clean Build) ===\n";
    std::cout << "Enter custom expressions (e.g., e^(-3*t)*cos(4*t), t^2 - 5*t). Type 'exit' to end.\n";

    while (true) {
        std::cout << "\nInput f(t) > ";
        std::getline(std::cin, input);
        if (input == "exit" || input == "quit") break;
        if (input.empty()) continue;

        try {
            Lexer lexer(input);
            std::vector<Token> tokens = lexer.tokenize();
            Parser parser(tokens);
            Node* root = parser.parse();

            std::cout << "Interpreted   : f(t) = " << root->toString() << "\n";
            std::cout << "---------------------------------------------------------\n";

            Node* rawDiff = root->diff(); Node* cleanDiff = rawDiff->simplify();
            std::cout << "1. Derivative  : d/dt = " << cleanDiff->toString() << "\n";
            delete rawDiff; delete cleanDiff;

            Node* rawInt = root->integrate(); Node* cleanInt = rawInt->simplify();
            std::cout << "2. Integral    : S dt = " << cleanInt->toString() << " + C\n";
            delete rawInt; delete cleanInt;

            Node* rawLap = root->laplace(); Node* cleanLap = rawLap->simplify();
            std::cout << "3. Laplace Tr. : L{}  = " << cleanLap->toString() << "\n";
            delete rawLap; delete cleanLap;

            delete root;
        } catch (const std::exception& e) {
            std::cout << ">>> Runtime Exception: " << e.what() << "\n";
        }
    }
    return 0;
}
