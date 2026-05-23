#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <stdexcept>
#include <limits>
#include <algorithm>

// ============================================================================
//  SECTION 1 — STEP LOGGER
// ============================================================================
struct StepLogger {
    std::vector<std::string> steps;
    bool enabled = true;

    void log(const std::string& msg) { if (enabled) steps.push_back(msg); }
    void clear()                     { steps.clear(); }
    bool empty()               const { return steps.empty(); }

    void print() const {
        for (size_t i = 0; i < steps.size(); ++i)
            std::cout << "      [" << (i + 1) << "] " << steps[i] << "\n";
    }
};

StepLogger gLog; 

// ============================================================================
//  SECTION 2 — UTILITY HELPERS
// ============================================================================
int getPrecedence(const std::string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*" || op == "/") return 2;
    if (op == "^")              return 3;
    return 4;
}

int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

std::string fmtNum(double v) {
    if (v == static_cast<int>(v)) return std::to_string(static_cast<int>(v));
    std::ostringstream ss;
    ss << std::setprecision(6) << v;
    return ss.str();
}

// ============================================================================
//  SECTION 3-6 — CLASS STRUCTURE
// ============================================================================
class Node {
public:
    virtual ~Node() = default;
    virtual std::string toString()           = 0;
    virtual std::string toLatex()            = 0;
    virtual double      eval(double t) const = 0;
    virtual Node*       diff()               = 0;
    virtual Node*       integrate()          = 0;
    virtual Node*       laplace()            = 0;
    virtual Node*       invLaplace()         = 0;
    virtual Node*       simplify()           = 0;
    virtual Node*       clone()        const = 0;
    virtual bool        isConstant()  const { return false; }
    virtual bool        isVariable()  const { return false; }
    virtual bool        isBinaryOp()  const { return false; }
    virtual bool        isFunction()  const { return false; }
    virtual double      getValue()    const { return 0.0;   }
    virtual std::string getOp()       const { return "";    }
    virtual std::string getFuncName() const { return "";    }
    virtual std::string getVarName()  const { return "";    }
    virtual Node*       getArg()      const { return nullptr; }
    virtual Node*       getLeft()     const { return nullptr; }
    virtual Node*       getRight()    const { return nullptr; }
};

class SymbolicNode : public Node {
    std::string expression;
public:
    explicit SymbolicNode(std::string expr) : expression(std::move(expr)) {}
    std::string toString()            override { return expression; }
    std::string toLatex()             override { return expression; }
    double      eval(double)    const override { return std::numeric_limits<double>::quiet_NaN(); }
    Node*       clone()         const override { return new SymbolicNode(expression); }
    Node*       simplify()            override { return clone(); }
    Node*       diff()                override { return new SymbolicNode("d/dt(" + expression + ")"); }
    Node*       integrate()           override { return new SymbolicNode("∫(" + expression + ") dt"); }
    Node*       laplace()             override { return new SymbolicNode("L{" + expression + "}"); }
    Node*       invLaplace()          override { return new SymbolicNode("L⁻¹{" + expression + "}"); }
};

class ConstantNode : public Node {
    double value;
public:
    explicit ConstantNode(double val) : value(val) {}
    bool        isConstant() const override { return true;  }
    double      getValue()   const override { return value; }
    std::string toString()         override;
    std::string toLatex()          override;
    double      eval(double) const override;
    Node* clone()      const override;
    Node* simplify()         override;
    Node* diff()             override;
    Node* integrate()        override;
    Node* laplace()          override;
    Node* invLaplace()       override;
};

class VariableNode : public Node {
    std::string name;
public:
    explicit VariableNode(std::string nm) : name(std::move(nm)) {}
    bool        isVariable()  const override { return true; }
    std::string getVarName()  const override { return name; }
    std::string toString()         override;
    std::string toLatex()          override;
    double      eval(double) const override;
    Node* clone()      const override;
    Node* simplify()         override;
    Node* diff()             override;
    Node* integrate()        override;
    Node* laplace()          override;
    Node* invLaplace()       override;
};

class FunctionNode : public Node {
    std::string name;
    Node*       arg;
public:
    FunctionNode(std::string nm, Node* a) : name(std::move(nm)), arg(a) {}
    ~FunctionNode() { delete arg; }
    bool        isFunction()  const override { return true; }
    std::string getFuncName() const override { return name; }
    Node*       getArg()      const override { return arg;  }
    std::string toString()         override;
    std::string toLatex()          override;
    double      eval(double) const override;
    Node* clone()      const override;
    Node* simplify()         override;
    Node* diff()             override;
    Node* integrate()        override;
    Node* laplace()          override;
    Node* invLaplace()       override;
};

class BinaryOpNode : public Node {
    std::string op;
    Node*       left;
    Node*       right;
public:
    BinaryOpNode(std::string operation, Node* l, Node* r)
        : op(std::move(operation)), left(l), right(r) {}
    ~BinaryOpNode() { delete left; delete right; }
    bool        isBinaryOp() const override { return true;  }
    std::string getOp()      const override { return op;    }
    Node*       getLeft()    const override { return left;  }
    Node*       getRight()   const override { return right; }
    std::string toString()         override;
    std::string toLatex()          override;
    double      eval(double) const override;
    Node* clone()      const override;
    Node* simplify()         override;
    Node* diff()             override;
    Node* integrate()        override;
    Node* laplace()          override;
    Node* invLaplace()       override;
};

// ============================================================================
//  SECTION 7 — METHOD IMPLEMENTATIONS (Simplified snippet for brevity)
// ============================================================================
// Note: In your full file, ensure you include the function bodies 
// like: std::string ConstantNode::toString() { return fmtNum(value); } 
// and the logic for the node operations (diff, integrate, etc.) 
// exactly as you wrote them in your previous snippet.
// ============================================================================

bool extractLinearCoeff(Node* arg, double& b) {
    if (arg->isVariable() && arg->getVarName() == "t") { b = 1.0; return true; }
    if (arg->isBinaryOp() && arg->getOp() == "*") {
        Node* L = arg->getLeft(); Node* R = arg->getRight();
        if (L->isConstant() && R->isVariable() && R->getVarName() == "t") { b = L->getValue(); return true; }
        if (R->isConstant() && L->isVariable() && L->getVarName() == "t") { b = R->getValue(); return true; }
    }
    return false;
}

// ... (Fill in the implementation bodies for ConstantNode, VariableNode, FunctionNode, BinaryOpNode) ...

Node* BinaryOpNode::integrate() {
    // Basic rules
    if (op == "+" || op == "-") return new BinaryOpNode(op, left->integrate(), right->integrate());
    if (op == "*" && left->isConstant()) return new BinaryOpNode("*", left->clone(), right->integrate());
    if (op == "*" && right->isConstant()) return new BinaryOpNode("*", left->integrate(), right->clone());

    // Integration by Parts
    if (op == "*") {
        bool leftIsT = left->isVariable() && left->getVarName() == "t";
        // t * exp(at)
        bool rightIsExp = right->isBinaryOp() && right->getOp() == "^" && right->getLeft()->isVariable() && right->getLeft()->getVarName() == "e";
        
        if (leftIsT && rightIsExp) {
            double a;
            if (extractLinearCoeff(right->getRight(), a) && a != 0.0) {
                gLog.log("Integration by parts: ∫t·e^(at)dt = e^(at)·(t/a - 1/a²)   [a = " + fmtNum(a) + "]");
                return new BinaryOpNode("*",
                    new FunctionNode("exp", right->getRight()->clone()),
                    new BinaryOpNode("-",
                        new BinaryOpNode("/", new VariableNode("t"), new ConstantNode(a)),
                        new BinaryOpNode("/", new ConstantNode(1), new ConstantNode(a * a))));
            }
        }
        // t^n
        if (op == "^" && left->isVariable() && left->getVarName() == "t" && right->isConstant()) {
            double n = right->getValue();
            return new BinaryOpNode("/", new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(n + 1)), new ConstantNode(n + 1));
        }
    }
    return new SymbolicNode("∫(" + toString() + ") dt");
}

Node* BinaryOpNode::laplace() {
    if (op == "+" || op == "-") return new BinaryOpNode(op, left->laplace(), right->laplace());
    if (op == "*") {
        if (left->isConstant()) return new BinaryOpNode("*", left->clone(), right->laplace());
        if (right->isConstant()) return new BinaryOpNode("*", left->laplace(), right->clone());
    }
    return new SymbolicNode("L{" + toString() + "}");
}

Node* BinaryOpNode::invLaplace() {
    return new SymbolicNode("L⁻¹{" + toString() + "}");
}

int main() {
    std::cout << "Engine Initialized." << std::endl;
    return 0;
}
