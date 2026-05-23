// ============================================================================
//  MULTI-DOMAIN MATH ENGINE - Extended Edition
//  Features: Differentiation, Integration, Laplace, Inverse Laplace,
//            Fourier Transform, Taylor Series, nth Derivative, Numerical Eval,
//            LaTeX Output, Step-by-Step Workings, Implicit Multiplication,
//            Expression History, Initial/Final Value Theorems
//  Functions: sin, cos, tan, asin, acos, atan, ln, sqrt, abs, exp, sinh, cosh, tanh, asinh, acosh, atanh
//  Variables: t (integration var), e (Euler's number), s (Laplace domain), pi
// ============================================================================
 
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif
#include <cctype>
#include <stdexcept>
#include <limits>
#include <memory>
#include <algorithm>
 
#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
//  SECTION 1 — STEP LOGGER
//  Every node method calls gLog.log("explanation") to record what rule it
//  applied. printResults() prints these numbered after each operation.
//  Type 'steps off' at the prompt to silence them.
// ============================================================================
 
struct StepLogger {
    std::vector<std::string> steps;
    bool enabled;

    StepLogger() : steps(), enabled(true) {}
 
    void log(const std::string& msg) { if (enabled) steps.push_back(msg); }
    void clear()                     { steps.clear(); }
    bool empty()               const { return steps.empty(); }
 
    void print() const {
        for (size_t i = 0; i < steps.size(); ++i)
            std::cout << "      [" << (i + 1) << "] " << steps[i] << "\n";
    }
};
 
StepLogger gLog; // One shared global logger used by all node operations
 
// ============================================================================
//  SECTION 2 — UTILITY HELPERS
// ============================================================================
 
// Operator precedence — used to decide when parentheses are needed
int getPrecedence(const std::string& op) {
    if (op == "+" || op == "-") return 1;
    if (op == "*" || op == "/") return 2;
    if (op == "^")              return 3;
    return 4;
}
 
// Integer factorial — used for L{t^n} = n! / s^(n+1)
int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}
 
// Format a double cleanly: show "3" not "3.000000"
std::string fmtNum(double v) {
    if (v == static_cast<int>(v)) return std::to_string(static_cast<int>(v));
    std::ostringstream ss;
    ss << std::setprecision(6) << v;
    return ss.str();
}
 
// Forward declarations
class Node;
std::string solveRoots(Node* node, double start);
void extractSPoly(Node* n, double& a, double& b, double& c);

// Print a horizontal separator line
void printLine(char ch = '-', size_t w = 65) {
    std::cout << std::string(static_cast<std::string::size_type>(w), ch) << "\n";
}
 
// ============================================================================
//  SECTION 3 — FORWARD DECLARATIONS
//  All node types are declared here so each implementation can reference
//  the others (e.g. ConstantNode::integrate() returns a BinaryOpNode).
// ============================================================================
 
class SymbolicNode;
class ConstantNode;
class VariableNode;
class FunctionNode;
class BinaryOpNode;

 
// ============================================================================
//  SECTION 4 — NODE BASE CLASS
//  Every expression in the tree inherits from Node and implements:
//    toString()   — algebraic string  (e.g. "t^2 + 1")
//    toLatex()    — LaTeX string       (e.g. "t^{2} + 1")
//    eval(t)      — numerical value at a given t
//    diff()       — derivative w.r.t. t
//    integrate()  — indefinite integral w.r.t. t
//    laplace()    — Laplace transform  (t-domain → s-domain)
//    invLaplace() — Inverse Laplace    (s-domain → t-domain)
//    simplify()   — basic algebraic simplification
//    clone()      — deep copy
// ============================================================================
 
class Node {
public:
    virtual ~Node() = default;

    virtual std::string toString()     const = 0;
    virtual std::string toLatex()      const = 0;
    virtual double      eval(double t) const = 0;
    virtual std::unique_ptr<Node> diff()       const   = 0;
    virtual std::unique_ptr<Node> integrate()  const   = 0;
    virtual std::unique_ptr<Node> laplace()    const   = 0;
    virtual std::unique_ptr<Node> invLaplace() const   = 0;
    virtual std::unique_ptr<Node> simplify()   const   = 0;
    virtual std::unique_ptr<Node> clone()        const = 0;
 
    // Type-query helpers used for pattern matching inside operations
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
 
// ============================================================================
//  SECTION 5 — SYMBOLIC NODE
//  A plain-text placeholder returned when no rule applies.
//  Wraps the expression in descriptive notation for display.
// ============================================================================
 
class SymbolicNode : public Node {
    std::string expression;
public:
    explicit SymbolicNode(std::string expr) : expression(std::move(expr)) {}
 
    std::string toString()      const override { return expression; }
    std::string toLatex()       const override { return expression; }
    double      eval(double)    const override { return std::numeric_limits<double>::quiet_NaN(); }
    std::unique_ptr<Node> clone()         const override { return std::make_unique<SymbolicNode>(expression); }
    std::unique_ptr<Node> simplify()      const override { return clone(); }
    std::unique_ptr<Node> diff()          const override { return std::make_unique<SymbolicNode>("d/dt(" + expression + ")"); }
    std::unique_ptr<Node> integrate()     const override { return std::make_unique<SymbolicNode>("Integral(" + expression + ") dt"); }
    std::unique_ptr<Node> laplace()       const override { return std::make_unique<SymbolicNode>("L{" + expression + "}"); }
    std::unique_ptr<Node> invLaplace()    const override { return std::make_unique<SymbolicNode>("L^-1{" + expression + "}"); }
};
 
// ============================================================================
//  SECTION 6 — SUBCLASS DECLARATIONS
//  Method bodies are in Section 7 (after all classes exist) to avoid
//  forward-reference errors when one node type returns another.
// ============================================================================
 
// --- Constant: a fixed numeric value ---
class ConstantNode : public Node {
    double value;
public:
    explicit ConstantNode(double val) : value(val) {}
 
    bool        isConstant() const override { return true;  }
    double      getValue()   const override { return value; }
 
    std::string toString()   const override;
    std::string toLatex()    const override;
    double      eval(double) const override;
    std::unique_ptr<Node> clone()      const override;
    std::unique_ptr<Node> simplify()   const override;
    std::unique_ptr<Node> diff()       const override;
    std::unique_ptr<Node> integrate()  const override;
    std::unique_ptr<Node> laplace()    const override;
    std::unique_ptr<Node> invLaplace() const override;
};
 
// --- Variable: a named symbol such as t, e, s ---
class VariableNode : public Node {
    std::string name;
public:
    explicit VariableNode(std::string nm) : name(std::move(nm)) {}
 
    bool        isVariable()  const override { return true; }
    std::string getVarName()  const override { return name; }
 
    std::string toString()   const override;
    std::string toLatex()    const override;
    double      eval(double) const override;
    std::unique_ptr<Node> clone()      const override;
    std::unique_ptr<Node> simplify()   const override;
    std::unique_ptr<Node> diff()       const override;
    std::unique_ptr<Node> integrate()  const override;
    std::unique_ptr<Node> laplace()    const override;
    std::unique_ptr<Node> invLaplace() const override;
};
 
// --- Function: a named function applied to one argument, e.g. sin(t) ---
class FunctionNode : public Node {
    std::string name;
    std::unique_ptr<Node> arg;
public:
    FunctionNode(std::string nm, std::unique_ptr<Node> a) : name(std::move(nm)), arg(std::move(a)) {}
    FunctionNode(const FunctionNode&)            = delete; // No copy (owns pointer)
    FunctionNode& operator=(const FunctionNode&) = delete;
 
    bool        isFunction()  const override { return true; }
    std::string getFuncName() const override { return name; }
    Node*       getArg()      const override { return arg.get();  }
 
    std::string toString()   const override;
    std::string toLatex()    const override;
    double      eval(double) const override;
    std::unique_ptr<Node> clone()      const override;
    std::unique_ptr<Node> simplify()   const override;
    std::unique_ptr<Node> diff()       const override;
    std::unique_ptr<Node> integrate()  const override;
    std::unique_ptr<Node> laplace()    const override;
    std::unique_ptr<Node> invLaplace() const override;
};
 
// --- Binary Operation: left op right (e.g. t^2 + sin(t)) ---
class BinaryOpNode : public Node {
    std::string op;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
public:
    BinaryOpNode(std::string operation, std::unique_ptr<Node> l, std::unique_ptr<Node> r)
        : op(std::move(operation)), left(std::move(l)), right(std::move(r)) {}
    BinaryOpNode(const BinaryOpNode&)            = delete; // No copy (owns pointers)
    BinaryOpNode& operator=(const BinaryOpNode&) = delete;
 
    bool        isBinaryOp() const override { return true;  }
    std::string getOp()      const override { return op;    }
    Node*       getLeft()    const override { return left.get();  }
    Node*       getRight()   const override { return right.get(); }
 
    std::string toString()   const override;
    std::string toLatex()    const override;
    double      eval(double) const override;
    std::unique_ptr<Node> clone()      const override;
    std::unique_ptr<Node> simplify()   const override;
    std::unique_ptr<Node> diff()       const override;
    std::unique_ptr<Node> integrate()  const override;
    std::unique_ptr<Node> laplace()    const override;
    std::unique_ptr<Node> invLaplace() const override;
};
 
// ============================================================================
//  SECTION 7A — CONSTANT NODE IMPLEMENTATIONS
// ============================================================================
 
std::string ConstantNode::toString() const { return fmtNum(value); }
 
std::string ConstantNode::toLatex() const {
    return (value < 0) ? "{" + fmtNum(value) + "}" : fmtNum(value);
}

double ConstantNode::eval(double /*t*/) const { return value; }
std::unique_ptr<Node> ConstantNode::clone()    const         { return std::make_unique<ConstantNode>(value); }
std::unique_ptr<Node> ConstantNode::simplify() const         { return clone(); }
 
std::unique_ptr<Node> ConstantNode::diff() const {
    // Rule: d/dt[c] = 0
    gLog.log("Constant rule: d/dt[" + fmtNum(value) + "] = 0");
    return std::make_unique<ConstantNode>(0);
}
 
std::unique_ptr<Node> ConstantNode::integrate() const {
    // Rule: ∫c dt = c·t
    gLog.log("Constant integral: ∫" + fmtNum(value) + " dt = " + fmtNum(value) + "·t");
    return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(value), std::make_unique<VariableNode>("t"));
}
 
std::unique_ptr<Node> ConstantNode::laplace() const {
    // Rule: L{c} = c/s
    gLog.log("Laplace of constant: L{" + fmtNum(value) + "} = " + fmtNum(value) + "/s");
    return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(value), std::make_unique<VariableNode>("s"));
}
 
std::unique_ptr<Node> ConstantNode::invLaplace() const {
    // A bare constant in s-domain corresponds to a scaled impulse
    gLog.log("L^-1{c} = c*delta(t)  (scaled impulse)");
    return std::make_unique<SymbolicNode>(fmtNum(value) + "·δ(t)");
}
 
// ============================================================================
//  SECTION 7B — VARIABLE NODE IMPLEMENTATIONS
// ============================================================================
 
std::string VariableNode::toString() const { return name; }
std::string VariableNode::toLatex()  const { return name; }
 
double VariableNode::eval(double t) const {
    if (name == "t")  return t;
    if (name == "e")  return M_E;
    if (name == "pi") return M_PI;
    return std::numeric_limits<double>::quiet_NaN(); // s is not evaluable in t-domain
}
 
std::unique_ptr<Node> VariableNode::clone()   const { return std::make_unique<VariableNode>(name); }
std::unique_ptr<Node> VariableNode::simplify() const { return clone(); }
 
std::unique_ptr<Node> VariableNode::diff() const {
    if (name == "t") {
        gLog.log("Power rule: d/dt[t] = 1");
        return std::make_unique<ConstantNode>(1);
    }
    // All other variables are constants with respect to t
    gLog.log("d/dt[" + name + "] = 0  (" + name + " is not a function of t)");
    return std::make_unique<ConstantNode>(0);
}
 
std::unique_ptr<Node> VariableNode::integrate() const {
    if (name == "t") {
        // ∫t dt = t²/2
        gLog.log("Power rule: ∫t dt = t²/2");
        return std::make_unique<BinaryOpNode>("/",
            std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(2)),
            std::make_unique<ConstantNode>(2));
    }
    // ∫k dt = k·t  (k constant w.r.t. t)
    gLog.log("∫" + name + " dt = " + name + "·t  (treated as constant w.r.t. t)");
    return std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>(name), std::make_unique<VariableNode>("t"));
}
 
std::unique_ptr<Node> VariableNode::laplace() const {
    if (name == "t") {
        // L{t} = 1/s²
        gLog.log("Standard pair: L{t} = 1/s²");
        return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(1),
            std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(2)));
    }
    return std::make_unique<BinaryOpNode>("/", clone(), std::make_unique<VariableNode>("s"));
}
 
std::unique_ptr<Node> VariableNode::invLaplace() const {
    if (name == "s") {
        gLog.log("L^-1{s} = delta'(t)  (derivative of impulse)");
        return std::make_unique<SymbolicNode>("δ'(t)");
    }
    return std::make_unique<SymbolicNode>("L^-1{" + name + "}");
}
 
// ============================================================================
//  SECTION 7C — SHARED HELPER: extractLinearCoeff
//  Given a node representing the argument of a function, checks whether
//  it is of the form (b*t) or just (t), and extracts coefficient b.
//  Returns true if the pattern matched, false otherwise.
// ============================================================================
 
bool extractLinearCoeff(Node* arg, double& b) {
    // Pattern: just "t"
    if (arg->isVariable() && arg->getVarName() == "t") {
        b = 1.0; return true;
    }
    // Pattern: "b*t" or "t*b"
    if (arg->isBinaryOp() && arg->getOp() == "*") {
        Node* L = arg->getLeft();
        Node* R = arg->getRight();
        if (L->isConstant() && R->isVariable() && R->getVarName() == "t") { b = L->getValue(); return true; }
        if (R->isConstant() && L->isVariable() && L->getVarName() == "t") { b = R->getValue(); return true; }
    }
    return false;
}
 
// ============================================================================
//  SECTION 7D — FUNCTION NODE IMPLEMENTATIONS
//  Supported: sin, cos, tan, asin, acos, atan, ln, sqrt, abs, exp
// ============================================================================
 
std::string FunctionNode::toString() const {
    return name + "(" + arg->toString() + ")";
}
 
std::string FunctionNode::toLatex() const {
    if (name == "sqrt") return "\\sqrt{" + arg->toLatex() + "}";
    if (name == "ln")   return "\\ln\\!(" + arg->toLatex() + ")";
    if (name == "abs")  return "\\left|" + arg->toLatex() + "\\right|";
    if (name == "exp")  return "e^{" + arg->toLatex() + "}";
    if (name == "sinh") return "\\sinh(" + arg->toLatex() + ")";
    if (name == "cosh") return "\\cosh(" + arg->toLatex() + ")";
    if (name == "tanh") return "\\tanh(" + arg->toLatex() + ")";
    if (name == "asinh") return "\\text{asinh}(" + arg->toLatex() + ")";
    if (name == "acosh") return "\\text{acosh}(" + arg->toLatex() + ")";
    if (name == "atanh") return "\\text{atanh}(" + arg->toLatex() + ")";
    if (name == "u")     return "u(" + arg->toLatex() + ")";
    return "\\" + name + "(" + arg->toLatex() + ")";
}
 
double FunctionNode::eval(double t) const {
    double a = arg->eval(t);
    if (name == "sin")  return std::sin(a);
    if (name == "cos")  return std::cos(a);
    if (name == "tan")  return std::tan(a);
    if (name == "asin") return std::asin(a);
    if (name == "acos") return std::acos(a);
    if (name == "atan") return std::atan(a);
    if (name == "ln")   return std::log(a);
    if (name == "sqrt") return std::sqrt(a);
    if (name == "abs")  return std::fabs(a);
    if (name == "exp")  return std::exp(a);
    if (name == "sinh") return std::sinh(a);
    if (name == "cosh") return std::cosh(a);
    if (name == "tanh") return std::tanh(a);
    if (name == "asinh") return std::asinh(a);
    if (name == "acosh") return std::acosh(a);
    if (name == "atanh") return std::atanh(a);
    if (name == "u")     return (a >= 0.0) ? 1.0 : 0.0;
    return std::numeric_limits<double>::quiet_NaN();
}
 
std::unique_ptr<Node> FunctionNode::clone()   const { return std::make_unique<FunctionNode>(name, arg->clone()); }
std::unique_ptr<Node> FunctionNode::simplify() const { return std::make_unique<FunctionNode>(name, arg->simplify()); }
 
// --- Chain Rule Derivatives ---
std::unique_ptr<Node> FunctionNode::diff() const {
    gLog.log("Chain rule on " + toString() + "  →  outer' · inner'");
 
    auto uPrime = arg->diff();   // Inner derivative u'
 
    if (name == "sin") {
        // d/dt[sin(u)] = cos(u) · u'
        gLog.log("  sin rule: d/dt[sin(u)] = cos(u) · u'");
        return std::make_unique<BinaryOpNode>("*", std::make_unique<FunctionNode>("cos", arg->clone()), std::move(uPrime));
    }
    if (name == "cos") {
        // d/dt[cos(u)] = -sin(u) · u'
        gLog.log("  cos rule: d/dt[cos(u)] = -sin(u) · u'");
        return std::make_unique<BinaryOpNode>("*",
            std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(-1), std::make_unique<FunctionNode>("sin", arg->clone())),
            std::move(uPrime));
    }
    if (name == "tan") {
        // d/dt[tan(u)] = u' / cos²(u)
        gLog.log("  tan rule: d/dt[tan(u)] = u' / cos²(u)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<BinaryOpNode>("^", std::make_unique<FunctionNode>("cos", arg->clone()), std::make_unique<ConstantNode>(2)));
    }
    if (name == "ln") {
        // d/dt[ln(u)] = u' / u
        gLog.log("  ln rule: d/dt[ln(u)] = u'/u");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime), arg->clone());
    }
    if (name == "sqrt") {
        // d/dt[√u] = u' / (2√u)
        gLog.log("  sqrt rule: d/dt[√u] = u' / (2√u)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(2), std::make_unique<FunctionNode>("sqrt", arg->clone())));
    }
    if (name == "exp") {
        // d/dt[exp(u)] = exp(u) · u'
        gLog.log("  exp rule: d/dt[exp(u)] = exp(u) · u'");
        return std::make_unique<BinaryOpNode>("*", std::make_unique<FunctionNode>("exp", arg->clone()), std::move(uPrime));
    }
    if (name == "abs") {
        // d/dt[|u|] = (u/|u|) · u'  — equivalent to sign(u) · u'
        gLog.log("  abs rule: d/dt[|u|] = sign(u) · u'");
        return std::make_unique<BinaryOpNode>("*",
            std::make_unique<BinaryOpNode>("/", arg->clone(), std::make_unique<FunctionNode>("abs", arg->clone())),
            std::move(uPrime));
    }
    if (name == "asin") {
        // d/dt[asin(u)] = u' / √(1 - u²)
        gLog.log("  asin rule: d/dt[asin(u)] = u' / √(1-u²)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<FunctionNode>("sqrt",
                std::make_unique<BinaryOpNode>("-", std::make_unique<ConstantNode>(1),
                    std::make_unique<BinaryOpNode>("^", arg->clone(), std::make_unique<ConstantNode>(2)))));
    }
    if (name == "acos") {
        // d/dt[acos(u)] = -u' / √(1 - u²)
        gLog.log("  acos rule: d/dt[acos(u)] = -u' / √(1-u²)");
        return std::make_unique<BinaryOpNode>("/",
            std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(-1), std::move(uPrime)),
            std::make_unique<FunctionNode>("sqrt",
                std::make_unique<BinaryOpNode>("-", std::make_unique<ConstantNode>(1),
                    std::make_unique<BinaryOpNode>("^", arg->clone(), std::make_unique<ConstantNode>(2)))));
    }
    if (name == "atan") {
        // d/dt[atan(u)] = u' / (1 + u²)
        gLog.log("  atan rule: d/dt[atan(u)] = u' / (1+u²)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<BinaryOpNode>("+", std::make_unique<ConstantNode>(1),
                std::make_unique<BinaryOpNode>("^", arg->clone(), std::make_unique<ConstantNode>(2))));
    }
    if (name == "sinh") {
        // d/dt[sinh(u)] = cosh(u) * u'
        gLog.log("  sinh rule: d/dt[sinh(u)] = cosh(u) · u'");
        return std::make_unique<BinaryOpNode>("*", std::make_unique<FunctionNode>("cosh", arg->clone()), std::move(uPrime));
    }
    if (name == "cosh") {
        // d/dt[cosh(u)] = sinh(u) * u'
        gLog.log("  cosh rule: d/dt[cosh(u)] = sinh(u) · u'");
        return std::make_unique<BinaryOpNode>("*", std::make_unique<FunctionNode>("sinh", arg->clone()), std::move(uPrime));
    }
    if (name == "tanh") {
        // d/dt[tanh(u)] = u' / cosh^2(u)
        gLog.log("  tanh rule: d/dt[tanh(u)] = u' / cosh²(u)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<BinaryOpNode>("^", std::make_unique<FunctionNode>("cosh", arg->clone()), std::make_unique<ConstantNode>(2)));
    }
    if (name == "asinh") {
        // d/dt[asinh(u)] = u' / sqrt(u^2 + 1)
        gLog.log("  asinh rule: d/dt[asinh(u)] = u' / √(u²+1)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<FunctionNode>("sqrt",
                std::make_unique<BinaryOpNode>("+",
                    std::make_unique<BinaryOpNode>("^", arg->clone(), std::make_unique<ConstantNode>(2)),
                    std::make_unique<ConstantNode>(1))));
    }
    if (name == "acosh") {
        // d/dt[acosh(u)] = u' / sqrt(u^2 - 1)
        gLog.log("  acosh rule: d/dt[acosh(u)] = u' / √(u²-1)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<FunctionNode>("sqrt",
                std::make_unique<BinaryOpNode>("-",
                    std::make_unique<BinaryOpNode>("^", arg->clone(), std::make_unique<ConstantNode>(2)),
                    std::make_unique<ConstantNode>(1))));
    }
    if (name == "atanh") {
        // d/dt[atanh(u)] = u' / (1 - u^2)
        gLog.log("  atanh rule: d/dt[atanh(u)] = u' / (1-u²)");
        return std::make_unique<BinaryOpNode>("/", std::move(uPrime),
            std::make_unique<BinaryOpNode>("-",
                std::make_unique<ConstantNode>(1),
                std::make_unique<BinaryOpNode>("^", arg->clone(), std::make_unique<ConstantNode>(2))));
    }
    if (name == "u") {
        // d/dt[u(t)] = delta(t)
        gLog.log("  step rule: d/dt[u(u)] = δ(u) · u'");
        return std::make_unique<BinaryOpNode>("*", std::make_unique<SymbolicNode>("δ(" + arg->toString() + ")"), std::move(uPrime));
    }
 
    gLog.log("  No rule for d/dt[" + name + "(...)] — returning symbolic");
    return std::make_unique<SymbolicNode>("d/dt(" + toString() + ")");
}
 
// --- Function Integrals ---
std::unique_ptr<Node> FunctionNode::integrate() const {
    double b = 1.0;
    bool linear = extractLinearCoeff(arg.get(), b); // Checks if arg = b*t
 
    if (linear) {
        if (name == "sin") {
            // ∫sin(bt) dt = -cos(bt)/b
            gLog.log("∫sin(bt)dt = -cos(bt)/b   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(-1.0 / b),
                std::make_unique<FunctionNode>("cos", arg->clone()));
        }
        if (name == "cos") {
            // ∫cos(bt) dt = sin(bt)/b
            gLog.log("∫cos(bt)dt = sin(bt)/b   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / b),
                std::make_unique<FunctionNode>("sin", arg->clone()));
        }
        if (name == "tan") {
            // ∫tan(bt) dt = -ln|cos(bt)|/b
            gLog.log("∫tan(bt)dt = -ln|cos(bt)|/b   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(-1.0 / b),
                std::make_unique<FunctionNode>("ln",
                    std::make_unique<FunctionNode>("abs", std::make_unique<FunctionNode>("cos", arg->clone()))));
        }
        if (name == "exp") {
            // ∫exp(bt) dt = exp(bt)/b
            gLog.log("∫exp(bt)dt = exp(bt)/b   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / b),
                std::make_unique<FunctionNode>("exp", arg->clone()));
        }
        if (name == "ln" && b == 1.0) {
            // ∫ln(t) dt = t·ln(t) - t   (integration by parts)
            gLog.log("∫ln(t)dt = t·ln(t) - t   [by parts: u=ln(t), dv=dt]");
            return std::make_unique<BinaryOpNode>("-",
                std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>("t"),
                    std::make_unique<FunctionNode>("ln", std::make_unique<VariableNode>("t"))),
                std::make_unique<VariableNode>("t"));
        }
        if (name == "sqrt" && b == 1.0) {
            // ∫√t dt = (2/3)·t^(3/2)
            gLog.log("∫√t dt = (2/3)·t^(3/2)");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(2.0 / 3.0),
                std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(1.5)));
        }
        if (name == "sinh") {
            // ∫sinh(bt) dt = cosh(bt)/b
            gLog.log("∫sinh(bt)dt = cosh(bt)/b   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / b),
                std::make_unique<FunctionNode>("cosh", arg->clone()));
        }
        if (name == "cosh") {
            // ∫cosh(bt) dt = sinh(bt)/b
            gLog.log("∫cosh(bt)dt = sinh(bt)/b   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / b),
                std::make_unique<FunctionNode>("sinh", arg->clone()));
        }
        if (name == "tanh") {
            // ∫tanh(bt) dt = ln(cosh(bt))/b
            gLog.log("∫tanh(bt)dt = ln(cosh(bt))/b   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / b),
                std::make_unique<FunctionNode>("ln", std::make_unique<FunctionNode>("cosh", arg->clone())));
        }
        if (name == "asinh" && b == 1.0) {
            // ∫asinh(t) dt = t·asinh(t) - sqrt(t^2 + 1)
            gLog.log("∫asinh(t)dt = t·asinh(t) - √(t²+1)");
            return std::make_unique<BinaryOpNode>("-",
                std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>("t"), std::make_unique<FunctionNode>("asinh", arg->clone())),
                std::make_unique<FunctionNode>("sqrt", std::make_unique<BinaryOpNode>("+", 
                    std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(2)), 
                    std::make_unique<ConstantNode>(1))));
        }
        if (name == "acosh" && b == 1.0) {
            // ∫acosh(t) dt = t·acosh(t) - sqrt(t^2 - 1)
            gLog.log("∫acosh(t)dt = t·acosh(t) - √(t²-1)");
            return std::make_unique<BinaryOpNode>("-",
                std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>("t"), std::make_unique<FunctionNode>("acosh", arg->clone())),
                std::make_unique<FunctionNode>("sqrt", std::make_unique<BinaryOpNode>("-", 
                    std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(2)), 
                    std::make_unique<ConstantNode>(1))));
        }
        if (name == "atanh" && b == 1.0) {
            // ∫atanh(t) dt = t·atanh(t) + 0.5·ln(1 - t^2)
            gLog.log("∫atanh(t)dt = t·atanh(t) + 0.5·ln|1-t²|");
            return std::make_unique<BinaryOpNode>("+",
                std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>("t"), std::make_unique<FunctionNode>("atanh", arg->clone())),
                std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(0.5), std::make_unique<FunctionNode>("ln", std::make_unique<FunctionNode>("abs", std::make_unique<BinaryOpNode>("-", std::make_unique<ConstantNode>(1), std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(2)))))));
        }
    }
 
    gLog.log("No closed-form integral for " + toString() + " — returning symbolic");
    return std::make_unique<SymbolicNode>("∫(" + toString() + ") dt");
}
 
// --- Function Laplace Transforms ---
std::unique_ptr<Node> FunctionNode::laplace() const {
    double b = 1.0;
    bool linear = extractLinearCoeff(arg.get(), b);
 
    if (linear) {
        if (name == "sin") {
            // L{sin(bt)} = b / (s² + b²)
            gLog.log("Pair: L{sin(bt)} = b/(s²+b²)   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(b),
                std::make_unique<BinaryOpNode>("+",
                    std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(2)),
                    std::make_unique<ConstantNode>(b * b)));
        }
        if (name == "cos") {
            // L{cos(bt)} = s / (s² + b²)
            gLog.log("Pair: L{cos(bt)} = s/(s²+b²)   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("/", std::make_unique<VariableNode>("s"),
                std::make_unique<BinaryOpNode>("+",
                    std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(2)),
                    std::make_unique<ConstantNode>(b * b)));
        }
        if (name == "exp") {
            // L{exp(at)} = 1/(s - a)
            gLog.log("Pair: L{exp(at)} = 1/(s-a)   [a = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(1),
                std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(b)));
        }
        if (name == "u" && b == 1.0) {
            // L{u(t)} = 1/s
            gLog.log("Pair: L{u(t)} = 1/s");
            return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(1),
                std::make_unique<VariableNode>("s"));
        }
        if (name == "sinh") {
            // L{sinh(bt)} = b / (s² - b²)
            gLog.log("Pair: L{sinh(bt)} = b/(s²-b²)   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(b),
                std::make_unique<BinaryOpNode>("-",
                    std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(2)),
                    std::make_unique<ConstantNode>(b * b)));
        }
        if (name == "cosh") {
            // L{cosh(bt)} = s / (s² - b²)
            gLog.log("Pair: L{cosh(bt)} = s/(s²-b²)   [b = " + fmtNum(b) + "]");
            return std::make_unique<BinaryOpNode>("/", std::make_unique<VariableNode>("s"),
                std::make_unique<BinaryOpNode>("-",
                    std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(2)),
                    std::make_unique<ConstantNode>(b * b)));
        }
    }
 
    gLog.log("No Laplace pair for " + toString() + " — returning symbolic");
    return std::make_unique<SymbolicNode>("L{" + toString() + "}");
}
 
std::unique_ptr<Node> FunctionNode::invLaplace() const {
    gLog.log("No inverse Laplace rule for function " + name + " — returning symbolic");
    return std::make_unique<SymbolicNode>("L⁻¹{" + toString() + "}");
}
 
// ============================================================================
//  SECTION 7E — BINARY OP NODE IMPLEMENTATIONS
// ============================================================================
 
std::string BinaryOpNode::toString() const {
    std::string L = left->toString(), R = right->toString();
    // Wrap sub-expressions in parens when their precedence is lower than ours
    if (left->isBinaryOp()  && getPrecedence(left->getOp())  < getPrecedence(op)) L = "(" + L + ")";
    if (right->isBinaryOp() && (getPrecedence(right->getOp()) < getPrecedence(op) || 
       (getPrecedence(right->getOp()) == getPrecedence(op) && (op == "-" || op == "/")))) 
        R = "(" + R + ")";
    return L + " " + op + " " + R;
}
 
std::string BinaryOpNode::toLatex() const {
    if (op == "/") return "\\frac{" + left->toLatex() + "}{" + right->toLatex() + "}";
    if (op == "^") return left->toLatex() + "^{" + right->toLatex() + "}";
    if (op == "*") return left->toLatex() + " \\cdot " + right->toLatex();
    return left->toLatex() + " " + op + " " + right->toLatex();
}
 
double BinaryOpNode::eval(double t) const {
    double L = left->eval(t), R = right->eval(t);
    if (op == "+") return L + R;
    if (op == "-") return L - R;
    if (op == "*") return L * R;
    if (op == "/") return (R != 0.0) ? L / R : std::numeric_limits<double>::quiet_NaN();
    if (op == "^") return std::pow(L, R);
    return std::numeric_limits<double>::quiet_NaN();
}
 
std::unique_ptr<Node> BinaryOpNode::clone() const {
    return std::make_unique<BinaryOpNode>(op, left->clone(), right->clone());
}
 
// --- Simplification ---
// Applies constant folding and common algebraic identities.
std::unique_ptr<Node> BinaryOpNode::simplify() const {
    auto L = left->simplify();
    auto R = right->simplify();
 
    // Fold two constants immediately: 3 * 4 → 12
    if (L->isConstant() && R->isConstant()) {
        double lv = L->getValue(), rv = R->getValue();
        if (op == "+") return std::make_unique<ConstantNode>(lv + rv);
        if (op == "-") return std::make_unique<ConstantNode>(lv - rv);
        if (op == "*") return std::make_unique<ConstantNode>(lv * rv);
        if (op == "/" && rv != 0.0) return std::make_unique<ConstantNode>(lv / rv);
        if (op == "^") return std::make_unique<ConstantNode>(std::pow(lv, rv));
    }
 
    // x * 1 = x  |  1 * x = x
    if (op == "*" && R->isConstant() && R->getValue() == 1.0) { return L; }
    if (op == "*" && L->isConstant() && L->getValue() == 1.0) { return R; }
 
    // x * 0 = 0  |  0 * x = 0
    if (op == "*" && ((L->isConstant() && L->getValue() == 0.0) ||
                      (R->isConstant() && R->getValue() == 0.0))) {
        return std::make_unique<ConstantNode>(0);
    }
 
    // x + 0 = x  |  0 + x = x
    if (op == "+" && R->isConstant() && R->getValue() == 0.0) { return L; }
    if (op == "+" && L->isConstant() && L->getValue() == 0.0) { return R; }
 
    // x - 0 = x
    if (op == "-" && R->isConstant() && R->getValue() == 0.0) { return L; }
 
    // x^1 = x  |  x^0 = 1
    if (op == "^" && R->isConstant() && R->getValue() == 1.0) { return L; }
    if (op == "^" && R->isConstant() && R->getValue() == 0.0) {
        return std::make_unique<ConstantNode>(1);
    }
 
    // --- Distribution Rule: a*(b + c) -> a*b + a*c ---
    if (op == "*") {
        // Left distribution: a * (b ± c)
        if (R->isBinaryOp() && (R->getOp() == "+" || R->getOp() == "-")) {
            auto dist = std::make_unique<BinaryOpNode>(R->getOp(),
                std::make_unique<BinaryOpNode>("*", L->clone(), R->getLeft()->clone()),
                std::make_unique<BinaryOpNode>("*", L->clone(), R->getRight()->clone()));
            return dist->simplify();
        }
        // Right distribution: (b ± c) * a
        if (L->isBinaryOp() && (L->getOp() == "+" || L->getOp() == "-")) {
            auto dist = std::make_unique<BinaryOpNode>(L->getOp(),
                std::make_unique<BinaryOpNode>("*", L->getLeft()->clone(), R->clone()),
                std::make_unique<BinaryOpNode>("*", L->getRight()->clone(), R->clone()));
            return dist->simplify();
        }
    }

    // --- Combine Like Terms Rule: 2t + 3t -> 5t ---
    if (op == "+" || op == "-") {
        auto getBaseAndCoeff = [](const Node* n, double& c, const Node*& b) {
            if (n->isBinaryOp() && n->getOp() == "*" && n->getLeft()->isConstant()) {
                c = n->getLeft()->getValue(); b = n->getRight();
            } else if (n->isBinaryOp() && n->getOp() == "*" && n->getRight()->isConstant()) {
                c = n->getRight()->getValue(); b = n->getLeft();
            } else if (!n->isConstant()) {
                c = 1.0; b = n;
            } else {
                b = nullptr; // Constants are handled by the folding rule above
            }
        };

        double cL = 0, cR = 0;
        const Node *bL = nullptr, *bR = nullptr;
        getBaseAndCoeff(L.get(), cL, bL);
        getBaseAndCoeff(R.get(), cR, bR);

        if (bL && bR && bL->toString() == bR->toString()) {
            double finalCoeff = (op == "+") ? (cL + cR) : (cL - cR);
            auto combined = std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(finalCoeff), bL->clone());
            return combined->simplify();
        }
    }

    return std::make_unique<BinaryOpNode>(op, std::move(L), std::move(R));
}
 
// --- Differentiation ---
std::unique_ptr<Node> BinaryOpNode::diff() const {
    if (op == "+" || op == "-") {
        // Sum/Difference Rule: (f ± g)' = f' ± g'
        gLog.log("Sum/difference rule: (f " + op + " g)' = f' " + op + " g'");
        return std::make_unique<BinaryOpNode>(op, left->diff(), right->diff());
    }
 
    if (op == "*") {
        // Product Rule: (f·g)' = f'·g + f·g'
        gLog.log("Product rule: (f·g)' = f'·g + f·g'"
                 "   where f = " + left->toString() + ",  g = " + right->toString());
        return std::make_unique<BinaryOpNode>("+",
            std::make_unique<BinaryOpNode>("*", left->diff(),  right->clone()),
            std::make_unique<BinaryOpNode>("*", left->clone(), right->diff()));
    }
 
    if (op == "/") {
        // Quotient Rule: (f/g)' = (f'g - fg') / g²
        gLog.log("Quotient rule: (f/g)' = (f'g - fg') / g²"
                 "   where f = " + left->toString() + ",  g = " + right->toString());
        return std::make_unique<BinaryOpNode>("/",
            std::make_unique<BinaryOpNode>("-",
                std::make_unique<BinaryOpNode>("*", left->diff(),  right->clone()),
                std::make_unique<BinaryOpNode>("*", left->clone(), right->diff())),
            std::make_unique<BinaryOpNode>("^", right->clone(), std::make_unique<ConstantNode>(2)));
    }
 
    if (op == "^") {
        // Power rule: d/dt[t^n] = n · t^(n-1)
        if (left->isVariable() && left->getVarName() == "t" && right->isConstant()) {
            double n = right->getValue();
            gLog.log("Power rule: d/dt[t^" + fmtNum(n) + "] = "
                     + fmtNum(n) + "·t^" + fmtNum(n - 1));
            return std::make_unique<BinaryOpNode>("*",
                std::make_unique<ConstantNode>(n),
                std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(n - 1)));
        }
        // Exponential rule: d/dt[e^f] = e^f · f'
        if (left->isVariable() && left->getVarName() == "e") {
            gLog.log("Exponential rule: d/dt[e^f] = e^f · f'"
                     "   where f = " + right->toString());
            return std::make_unique<BinaryOpNode>("*", clone(), right->diff());
        }
        // General base: d/dt[a^u] = a^u · ln(a) · u'
        if (left->isConstant()) {
            double a = left->getValue();
            gLog.log("General power: d/dt[" + fmtNum(a) + "^u] = "
                     + fmtNum(a) + "^u · ln(" + fmtNum(a) + ") · u'");
            return std::make_unique<BinaryOpNode>("*",
                std::make_unique<BinaryOpNode>("*", clone(), std::make_unique<ConstantNode>(std::log(a))),
                right->diff());
        }
    }
 
    gLog.log("No differentiation rule for (" + toString() + ") — returning symbolic");
    return std::make_unique<SymbolicNode>("d/dt(" + toString() + ")");
}
 
// --- Integration ---
std::unique_ptr<Node> BinaryOpNode::integrate() const {
    if (op == "+" || op == "-") {
        // Linearity: ∫(f ± g) dt = ∫f dt ± ∫g dt
        gLog.log("Linearity: ∫(f " + op + " g) dt = ∫f dt " + op + " ∫g dt");
        return std::make_unique<BinaryOpNode>(op, left->integrate(), right->integrate());
    }
 
    // Constant multiple: ∫c·f dt = c·∫f dt
    if (op == "*" && left->isConstant()) {
        gLog.log("Constant factor: ∫" + left->toString() + "·f dt = "
                 + left->toString() + "·∫f dt");
        return std::make_unique<BinaryOpNode>("*", left->clone(), right->integrate());
    }
    if (op == "*" && right->isConstant()) {
        gLog.log("Constant factor: ∫f·" + right->toString() + " dt = "
                 + right->toString() + "·∫f dt");
        return std::make_unique<BinaryOpNode>("*", left->integrate(), right->clone());
    }
 
    // --- Integration by Parts patterns ---
    if (op == "*") {
        bool leftIsT  = left->isVariable()  && left->getVarName() == "t";
        bool rightIsT = right->isVariable() && right->getVarName() == "t";
 
        // ∫t·sin(bt) dt = -t·cos(bt)/b + sin(bt)/b²
        if (leftIsT && right->isFunction() && right->getFuncName() == "sin") {
            double b;
            if (extractLinearCoeff(right->getArg(), b)) {
                gLog.log("Integration by parts: ∫t·sin(bt)dt = -t·cos(bt)/b + sin(bt)/b²   [b = " + fmtNum(b) + "]");
                return std::make_unique<BinaryOpNode>("+",
                    std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(-1.0 / b),
                        std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>("t"),
                            std::make_unique<FunctionNode>("cos", right->getArg()->clone()))),
                    std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / (b * b)),
                        std::make_unique<FunctionNode>("sin", right->getArg()->clone())));
            }
        }
        // ∫t·cos(bt) dt = t·sin(bt)/b + cos(bt)/b²
        if (leftIsT && right->isFunction() && right->getFuncName() == "cos") {
            double b;
            if (extractLinearCoeff(right->getArg(), b)) {
                gLog.log("Integration by parts: ∫t·cos(bt)dt = t·sin(bt)/b + cos(bt)/b²   [b = " + fmtNum(b) + "]");
                return std::make_unique<BinaryOpNode>("+",
                    std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / b),
                        std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>("t"),
                            std::make_unique<FunctionNode>("sin", right->getArg()->clone()))),
                    std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(1.0 / (b * b)),
                        std::make_unique<FunctionNode>("cos", right->getArg()->clone())));
            }
        }
        // ∫t·e^(at) dt = e^(at)·(t/a - 1/a²)
        bool rightIsExp = right->isBinaryOp() && right->getOp() == "^" &&
                          right->getLeft()->isVariable() && right->getLeft()->getVarName() == "e";
        if (leftIsT && rightIsExp) {
            double a;
            if (extractLinearCoeff(right->getRight(), a) && a != 0.0) {
                gLog.log("Integration by parts: ∫t·e^(at)dt = e^(at)·(t/a - 1/a²)   [a = " + fmtNum(a) + "]");
                return std::make_unique<BinaryOpNode>("*", right->clone(),
                    std::make_unique<BinaryOpNode>("-",
                        std::make_unique<BinaryOpNode>("/", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(a)),
                        std::make_unique<ConstantNode>(1.0 / (a * a))));
            }
        }
 
        // Mirror: if trig/exp is on the left, swap and retry
        if (rightIsT && (left->isFunction() || (left->isBinaryOp() && left->getOp() == "^"))) {
            return std::make_unique<BinaryOpNode>("*", right->clone(), left->clone())->integrate();
        }
    }
 
    // Power rule: ∫t^n dt = t^(n+1) / (n+1)
    if (op == "^" && left->isVariable() && left->getVarName() == "t" && right->isConstant()) {
        double n = right->getValue();
        gLog.log("Power rule: ∫t^" + fmtNum(n) + " dt = t^" + fmtNum(n + 1)
                 + "/" + fmtNum(n + 1));
        return std::make_unique<BinaryOpNode>("/",
            std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), std::make_unique<ConstantNode>(n + 1)),
            std::make_unique<ConstantNode>(n + 1));
    }
 
    // ∫e^(at) dt = e^(at)/a
    if (op == "^" && left->isVariable() && left->getVarName() == "e") {
        double a;
        if (extractLinearCoeff(right.get(), a) && a != 0.0) {
            gLog.log("Exponential integral: ∫e^(at)dt = e^(at)/a   [a = " + fmtNum(a) + "]");
            return std::make_unique<BinaryOpNode>("/", clone(), std::make_unique<ConstantNode>(a));
        }
    }
 
    gLog.log("No closed-form integral for (" + toString() + ") — returning symbolic");
    return std::make_unique<SymbolicNode>("∫(" + toString() + ") dt");
}
 
// --- Laplace Transform ---
std::unique_ptr<Node> BinaryOpNode::laplace() const {
    if (op == "+" || op == "-") {
        // Linearity: L{f ± g} = L{f} ± L{g}
        gLog.log("Linearity: L{f " + op + " g} = L{f} " + op + " L{g}");
        return std::make_unique<BinaryOpNode>(op, left->laplace(), right->laplace());
    }
 
    // Scaling: L{c·f} = c·L{f}
    if (op == "*" && left->isConstant()) {
        gLog.log("Scaling: L{" + left->toString() + "·f} = " + left->toString() + "·L{f}");
        return std::make_unique<BinaryOpNode>("*", left->clone(), right->laplace());
    }
    if (op == "*" && right->isConstant()) {
        gLog.log("Scaling: L{f·" + right->toString() + "} = " + right->toString() + "·L{f}");
        return std::make_unique<BinaryOpNode>("*", left->laplace(), right->clone());
    }
 
    // Product patterns: detect (exp) × (sin | cos | t) from either side
    if (op == "*") {
        // Locate the exponential and the trig/t factor
        auto getExpCoeff = [](const Node* n, double& a) -> bool {
            if (n->isBinaryOp() && n->getOp() == "^" &&
                n->getLeft()->isVariable() && n->getLeft()->getVarName() == "e")
                return extractLinearCoeff(n->getRight(), a);
            return false;
        };
 
        double a = 0, b = 0;
        bool foundExp   = false;
        const Node* otherSide = nullptr;
 
        // Try left=exp, right=other
        if (getExpCoeff(left.get(), a)) {
            foundExp = true; otherSide = right.get();
        } else if (getExpCoeff(right.get(), a)) {
            foundExp = true; otherSide = left.get();
        }
 
        if (foundExp && otherSide) {
            if (otherSide->isFunction() && otherSide->getFuncName() == "sin" &&
                extractLinearCoeff(otherSide->getArg(), b)) {
                // First Shift: L{e^(at)·sin(bt)} = b / ((s-a)² + b²)
                gLog.log("First shift theorem: L{e^(at)·sin(bt)} = b/((s-a)²+b²)"
                         "   [a=" + fmtNum(a) + " b=" + fmtNum(b) + "]");
                return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(b),
                    std::make_unique<BinaryOpNode>("+",
                        std::make_unique<BinaryOpNode>("^",
                            std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(a)),
                            std::make_unique<ConstantNode>(2)),
                        std::make_unique<ConstantNode>(b * b)));
            }
            if (otherSide->isFunction() && otherSide->getFuncName() == "cos" &&
                extractLinearCoeff(otherSide->getArg(), b)) {
                // First Shift: L{e^(at)·cos(bt)} = (s-a) / ((s-a)² + b²)
                gLog.log("First shift theorem: L{e^(at)·cos(bt)} = (s-a)/((s-a)²+b²)"
                         "   [a=" + fmtNum(a) + " b=" + fmtNum(b) + "]");
                return std::make_unique<BinaryOpNode>("/",
                    std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(a)),
                    std::make_unique<BinaryOpNode>("+",
                        std::make_unique<BinaryOpNode>("^",
                            std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(a)),
                            std::make_unique<ConstantNode>(2)),
                        std::make_unique<ConstantNode>(b * b)));
            }
            if (otherSide->isVariable() && otherSide->getVarName() == "t") {
                // L{t·e^(at)} = 1/(s-a)²
                gLog.log("Pair: L{t·e^(at)} = 1/(s-a)²   [a=" + fmtNum(a) + "]");
                return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(1),
                    std::make_unique<BinaryOpNode>("^",
                        std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(a)),
                        std::make_unique<ConstantNode>(2)));
            }
        }

        // --- Second Shift Theorem: L{u(t-a)f(t-a)} = e^(-as)F(s) ---
        auto getShift = [](const Node* n, double& a) -> bool {
            if (n->isFunction() && n->getFuncName() == "u") {
                Node* uArg = n->getArg();
                if (uArg->isBinaryOp() && uArg->getOp() == "-" &&
                    uArg->getLeft()->isVariable() && uArg->getLeft()->getVarName() == "t" &&
                    uArg->getRight()->isConstant()) {
                    a = uArg->getRight()->getValue(); return true;
                }
            }
            return false;
        };

        double aS = 0;
        const Node *uP = nullptr, *fP = nullptr;
        if (getShift(left.get(), aS)) { uP = left.get(); fP = right.get(); }
        else if (getShift(right.get(), aS)) { uP = right.get(); fP = left.get(); }

        if (uP && aS > 0) {
            auto isTMinusA = [aS](Node* n) {
                return n->isBinaryOp() && n->getOp() == "-" &&
                       n->getLeft()->isVariable() && n->getLeft()->getVarName() == "t" &&
                       n->getRight()->isConstant() && std::abs(n->getRight()->getValue() - aS) < 1e-9;
            };
            std::unique_ptr<Node> unshifted = nullptr;
            // f(t-a) = (t-a)^n
            if (fP->isBinaryOp() && fP->getOp() == "^" && isTMinusA(fP->getLeft()))
                unshifted = std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("t"), fP->getRight()->clone());
            // f(t-a) = sin(b(t-a)) or similar functions
            else if (fP->isFunction()) {
                Node* fa = fP->getArg();
                if (isTMinusA(fa)) unshifted = std::make_unique<FunctionNode>(fP->getFuncName(), std::make_unique<VariableNode>("t"));
                else if (fa->isBinaryOp() && fa->getOp() == "*" && fa->getLeft()->isConstant() && isTMinusA(fa->getRight()))
                    unshifted = std::make_unique<FunctionNode>(fP->getFuncName(), std::make_unique<BinaryOpNode>("*", fa->getLeft()->clone(), std::make_unique<VariableNode>("t")));
            }
            // f(t-a) = e^(b(t-a))
            else if (fP->isBinaryOp() && fP->getOp() == "^" && fP->getLeft()->isVariable() && fP->getLeft()->getVarName() == "e") {
                Node* ea = fP->getRight();
                if (isTMinusA(ea)) unshifted = std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("e"), std::make_unique<VariableNode>("t"));
                else if (ea->isBinaryOp() && ea->getOp() == "*" && ea->getLeft()->isConstant() && isTMinusA(ea->getRight()))
                    unshifted = std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("e"), std::make_unique<BinaryOpNode>("*", ea->getLeft()->clone(), std::make_unique<VariableNode>("t")));
            }

            if (unshifted) {
                gLog.log("Second shift theorem: L{u(t-a)f(t-a)} = e^(-as)F(s)   [a=" + fmtNum(aS) + "]");
                return std::make_unique<BinaryOpNode>("*",
                    std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("e"),
                        std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(-aS), std::make_unique<VariableNode>("s"))),
                    unshifted->laplace());
            }
        }
    }
 
    // L{t^n} = n! / s^(n+1)
    if (op == "^" && left->isVariable() && left->getVarName() == "t" && right->isConstant()) {
        int n = static_cast<int>(right->getValue());
        gLog.log("Pair: L{t^n} = n!/s^(n+1)   [n=" + std::to_string(n)
                 + ", n!=" + std::to_string(factorial(n)) + "]");
        return std::make_unique<BinaryOpNode>("/",
            std::make_unique<ConstantNode>(factorial(n)),
            std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(n + 1)));
    }
 
    // L{e^(at)} = 1/(s-a)
    if (op == "^" && left->isVariable() && left->getVarName() == "e") {
        double a;
        if (extractLinearCoeff(right.get(), a)) {
            gLog.log("Pair: L{e^(at)} = 1/(s-a)   [a=" + fmtNum(a) + "]");
            return std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(1),
                std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(a)));
        }
    }
 
    gLog.log("No Laplace rule for (" + toString() + ") — returning symbolic");
    return std::make_unique<SymbolicNode>("L{" + toString() + "}");
}
 
// --- Inverse Laplace Transform (pattern matching on F(s)) ---
std::unique_ptr<Node> BinaryOpNode::invLaplace() const {
    // Linearity: L⁻¹{F ± G} = L⁻¹{F} ± L⁻¹{G}
    if (op == "+" || op == "-") {
        gLog.log("Linearity: L^-1{F " + op + " G} = L^-1{F} " + op + " L^-1{G}");
        return std::make_unique<BinaryOpNode>(op, left->invLaplace(), right->invLaplace());
    }
    // Scaling: L⁻¹{c·F} = c·L⁻¹{F}
    if (op == "*" && left->isConstant()) {
        gLog.log("Scaling: L^-1{" + left->toString() + "·F} = " + left->toString() + "·L^-1{F}");
        return std::make_unique<BinaryOpNode>("*", left->clone(), right->invLaplace());
    }
    if (op == "*" && right->isConstant()) {
        return std::make_unique<BinaryOpNode>("*", right->clone(), left->invLaplace());
    }
 
    if (op == "/") {
        // --- Partial Fraction Expansion Helper (Quadratic Case) ---
        double n2=0, n1=0, n0=0; // Numerator coeffs
        double d2=0, d1=0, d0=0; // Denominator coeffs
        extractSPoly(left.get(), n2, n1, n0);
        extractSPoly(right.get(), d2, d1, d0);

        // Handle N(s) / (as^2 + bs + c) where a != 0
        if (d2 != 0.0 && n2 == 0.0) {
            double disc = d1*d1 - 4*d2*d0;
            if (disc > 0) { // Distinct Real Roots
                double r1 = (-d1 + std::sqrt(disc)) / (2 * d2);
                double r2 = (-d1 - std::sqrt(disc)) / (2 * d2);
                
                // Residues: A = N(r1)/D'(r1), B = N(r2)/D'(r2)
                double A = (n1 * r1 + n0) / (2 * d2 * r1 + d1);
                double B = (n1 * r2 + n0) / (2 * d2 * r2 + d1);

                gLog.log("PFE: Found distinct poles at s=" + fmtNum(r1) + ", " + fmtNum(r2));
                
                // Reconstruct: A/(s-r1) + B/(s-r2)
                auto expanded = std::make_unique<BinaryOpNode>("+",
                    std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(A), std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(r1))),
                    std::make_unique<BinaryOpNode>("/", std::make_unique<ConstantNode>(B), std::make_unique<BinaryOpNode>("-", std::make_unique<VariableNode>("s"), std::make_unique<ConstantNode>(r2)))
                );
                
                return expanded->invLaplace();
            }
            else if (disc < 0) { // Complex Roots: s = alpha +/- j*beta
                // Denominator: d2*s^2 + d1*s + d0 = d2 * [(s - alpha)^2 + beta^2]
                double alpha = -d1 / (2.0 * d2);
                double beta = std::sqrt(-disc) / (2.0 * d2);

                // Numerator: n1*s + n0 = n1*(s - alpha) + (n1*alpha + n0)
                // Residues for shifted cos and sin
                double A = n1 / d2;
                double B = (n1 * alpha + n0) / (d2 * beta);

                gLog.log("PFE: Found complex poles at s = " + fmtNum(alpha) + " ± j" + fmtNum(beta));

                // Time domain: e^(alpha*t) * [A*cos(beta*t) + B*sin(beta*t)]
                auto cosPart = std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(A),
                    std::make_unique<FunctionNode>("cos", std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(beta), std::make_unique<VariableNode>("t"))));
                auto sinPart = std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(B),
                    std::make_unique<FunctionNode>("sin", std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(beta), std::make_unique<VariableNode>("t"))));
                auto innerSum = std::make_unique<BinaryOpNode>("+", std::move(cosPart), std::move(sinPart));
                
                auto expPart = std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("e"),
                    std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(alpha), std::make_unique<VariableNode>("t")));

                // The simplify() call in the main loop will clean up cases where alpha=0 or A/B=1
                return std::make_unique<BinaryOpNode>("*", std::move(expPart), std::move(innerSum));
            }
            else { // Repeated Real Roots (disc == 0)
                double r = -d1 / (2.0 * d2);
                // F(s) = [ (n1/d2) / (s-r) ] + [ ((n1*r + n0)/d2) / (s-r)^2 ]
                double A = n1 / d2;
                double B = (n1 * r + n0) / d2;
                
                gLog.log("PFE: Found repeated pole at s = " + fmtNum(r));
                
                // A*e^(rt) + B*t*e^(rt)
                auto expTerm = std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("e"), 
                    std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(r), std::make_unique<VariableNode>("t")));
                auto term1 = std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(A), expTerm->clone());
                auto term2 = std::make_unique<BinaryOpNode>("*", std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(B), std::make_unique<VariableNode>("t")), std::move(expTerm));
                
                return std::make_unique<BinaryOpNode>("+", std::move(term1), std::move(term2))->simplify();
            }
        }

        // L^-1{c/s} = c  (constant in t-domain)
        if (right->isVariable() && right->getVarName() == "s" && left->isConstant()) {
            gLog.log("L^-1{c/s} = c   [c = " + fmtNum(left->getValue()) + "]");
            return std::make_unique<ConstantNode>(left->getValue());
        }
        // L^-1{1/s^2} = t
        if (left->isConstant() && left->getValue() == 1.0 &&
            right->isBinaryOp() && right->getOp() == "^" &&
            right->getLeft()->isVariable() && right->getLeft()->getVarName() == "s" &&
            right->getRight()->isConstant() && right->getRight()->getValue() == 2.0) {
            gLog.log("L^-1{1/s^2} = t");
            return std::make_unique<VariableNode>("t");
        }
        // L^-1{1/(s-a)} = e^(at)
        if (left->isConstant() && left->getValue() == 1.0 &&
            right->isBinaryOp() && right->getOp() == "-" &&
            right->getLeft()->isVariable() && right->getLeft()->getVarName() == "s" &&
            right->getRight()->isConstant()) {
            double a = right->getRight()->getValue();
            gLog.log("L^-1{1/(s-a)} = e^(at)   [a = " + fmtNum(a) + "]");
            return std::make_unique<BinaryOpNode>("^", std::make_unique<VariableNode>("e"),
                std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(a), std::make_unique<VariableNode>("t")));
        }
        // L^-1{b/(s^2+b^2)} = sin(bt)
        if (left->isConstant()) {
            double bv = left->getValue();
            if (right->isBinaryOp() && right->getOp() == "+") {
                Node* rL = right->getLeft(); Node* rR = right->getRight();
                if (rL->isBinaryOp() && rL->getOp() == "^" &&
                    rL->getLeft()->isVariable() && rL->getLeft()->getVarName() == "s" &&
                    rL->getRight()->isConstant() && rL->getRight()->getValue() == 2.0 &&
                    rR->isConstant() && std::fabs(std::sqrt(rR->getValue()) - bv) < 1e-9) {
                    gLog.log("L^-1{b/(s^2+b^2)} = sin(bt)   [b = " + fmtNum(bv) + "]");
                    return std::make_unique<FunctionNode>("sin",
                        std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(bv), std::make_unique<VariableNode>("t")));
                }
                if (rL->isBinaryOp() && rL->getOp() == "^" &&
                    rL->getLeft()->isVariable() && rL->getLeft()->getVarName() == "s" &&
                    rL->getRight()->isConstant() && rL->getRight()->getValue() == 2.0 &&
                    rR->isConstant() && std::fabs(std::sqrt(rR->getValue()) - bv) < 1e-9) {
                    gLog.log("L^-1{b/(s^2-b^2)} = sinh(bt)   [b = " + fmtNum(bv) + "]");
                    return std::make_unique<FunctionNode>("sinh",
                        std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(bv), std::make_unique<VariableNode>("t")));
                }
            }
        }
        // L^-1{s/(s^2+b^2)} = cos(bt)
        if (left->isVariable() && left->getVarName() == "s") {
            if (right->isBinaryOp()) {
                Node* rL = right->getLeft(); Node* rR = right->getRight();
                if (rL->isBinaryOp() && rL->getOp() == "^" &&
                    rL->getLeft()->isVariable() && rL->getLeft()->getVarName() == "s" &&
                    rL->getRight()->isConstant() && rL->getRight()->getValue() == 2.0 && 
                    right->getOp() == "+" && rR->isConstant()) {
                    double b = std::sqrt(rR->getValue());
                    gLog.log("L^-1{s/(s^2+b^2)} = cos(bt)   [b = " + fmtNum(b) + "]");
                    return std::make_unique<FunctionNode>("cos",
                        std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(b), std::make_unique<VariableNode>("t")));
                }
                if (rL->isBinaryOp() && rL->getOp() == "^" &&
                    rL->getLeft()->isVariable() && rL->getLeft()->getVarName() == "s" &&
                    rL->getRight()->isConstant() && rL->getRight()->getValue() == 2.0 &&
                    right->getOp() == "-" && rR->isConstant()) {
                    double b = std::sqrt(rR->getValue());
                    gLog.log("L^-1{s/(s^2-b^2)} = cosh(bt)   [b = " + fmtNum(b) + "]");
                    return std::make_unique<FunctionNode>("cosh",
                        std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(b), std::make_unique<VariableNode>("t")));
                }
            }
        }
    }
 
    gLog.log("No inverse Laplace rule matched for (" + toString() + ") - returning symbolic");
    return std::make_unique<SymbolicNode>("L^-1{" + toString() + "}");
}
 
// ============================================================================
//  SECTION 8 — LEXER
//  Converts a raw string into a sequence of tokens.
//  Also inserts implicit '*' tokens so "2t" and "3sin(t)" work correctly.
// ============================================================================
 
enum TokenType { TOKEN_NUM, TOKEN_VAR, TOKEN_FUNC, TOKEN_OP,
                 TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_EOF };
 
struct Token { enum TokenType type; std::string value; };
 
class Lexer {
    std::string src;
    size_t pos = 0;
 
    // Returns true if the identifier is a known function name
    static bool isKnownFunction(const std::string& s) {
        static const std::vector<std::string> fns = {
            "sin","cos","tan","asin","acos","atan","ln","sqrt","abs","exp",
            "sinh","cosh","tanh","asinh","acosh","atanh","u"
        };
        return std::find(fns.begin(), fns.end(), s) != fns.end();
    }
 
public:
    explicit Lexer(std::string source) : src(std::move(source)) {}
 
    std::vector<Token> tokenize() {
        std::vector<Token> raw;
 
        while (pos < src.size()) {
            char ch = src[pos];
 
            if (std::isspace(ch)) { pos++; continue; }
 
            // Numeric literal (supports decimals like 3.14)
            if (std::isdigit(ch) || ch == '.') {
                std::string num;
                bool hasDot = false;
                while (pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.')) {
                    if (src[pos] == '.') {
                        if (hasDot) break;
                        hasDot = true;
                    }
                    num += src[pos++];
                }
                if (num == ".") throw std::runtime_error("Invalid numeric literal: '.'");
                raw.push_back(Token{TOKEN_NUM, num});
                continue;
            }
 
            // Identifier: variable name or function name
            if (std::isalpha(ch)) {
                std::string ident;
                while (pos < src.size() && std::isalpha(src[pos]))
                    ident += src[pos++];
 
                if (ident == "t" || ident == "e" || ident == "s" || ident == "pi")
                    raw.push_back({TOKEN_VAR, ident});
                else if (isKnownFunction(ident))
                    raw.push_back({TOKEN_FUNC, ident});
                else
                    throw std::runtime_error("Unknown identifier: '" + ident + "'");
                continue;
            }
 
            if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '^') {
                raw.push_back({TOKEN_OP, std::string(1, ch)}); pos++; continue;
            }
            if (ch == '(') { raw.push_back({TOKEN_LPAREN,  "("}); pos++; continue; }
            if (ch == ')') { raw.push_back({TOKEN_RPAREN,  ")"}); pos++; continue; }
 
            throw std::runtime_error("Illegal character: '" + std::string(1, ch) + "'");
        }
        raw.push_back({TOKEN_EOF, ""});
 
        // --- Implicit Multiplication ---
        // Insert a synthetic '*' token between tokens where multiplication is implied:
        //   "2t"    → "2 * t"
        //   "3sin(" → "3 * sin("
        //   "t("    → "t * ("
        //   ")(t"   → ") * ( t"
        std::vector<Token> result;
        for (size_t i = 0; i + 1 < raw.size(); ++i) {
            result.push_back(raw[i]);
            enum TokenType cur = raw[i].type;
            enum TokenType nxt = raw[i + 1].type;
            bool insertMul =
                (cur == TOKEN_NUM    && (nxt == TOKEN_VAR || nxt == TOKEN_FUNC || nxt == TOKEN_LPAREN)) ||
                (cur == TOKEN_VAR    && (nxt == TOKEN_VAR || nxt == TOKEN_FUNC || nxt == TOKEN_LPAREN)) ||
                (cur == TOKEN_RPAREN && (nxt == TOKEN_NUM || nxt == TOKEN_VAR  ||
                                         nxt == TOKEN_FUNC || nxt == TOKEN_LPAREN));
            if (insertMul)
                result.push_back({TOKEN_OP, "*"});
        }
        result.push_back(raw.back()); // Push EOF
        return result;
    }
};
 
// ============================================================================
//  SECTION 9 — PARSER
//  Recursive-descent parser that builds an Abstract Syntax Tree (AST).
//
//  Grammar (simplified):
//    expression ::= term (('+' | '-') term)*
//    term       ::= power (('*' | '/') power)*
//    power      ::= unary ('^' power)?          — right-associative
//    unary      ::= ('-' | '+') unary | primary
//    primary    ::= NUMBER | VARIABLE | FUNC'('expr')' | '('expr')'
// ============================================================================
 
class Parser {
    std::vector<Token> tokens;
    size_t index = 0;
 
    Token& peek()    { return tokens[index]; }
    Token  consume() { return tokens[index++]; }
 
    // Lowest precedence: addition and subtraction
    std::unique_ptr<Node> parseExpression() {
        auto node = parseTerm();
        while (peek().type == TOKEN_OP && (peek().value == "+" || peek().value == "-")) {
            std::string op = consume().value;
            node = std::make_unique<BinaryOpNode>(op, std::move(node), parseTerm());
        }
        return node;
    }
 
    // Multiplication and division
    std::unique_ptr<Node> parseTerm() {
        auto node = parsePower();
        while (peek().type == TOKEN_OP && (peek().value == "*" || peek().value == "/")) {
            std::string op = consume().value;
            node = std::make_unique<BinaryOpNode>(op, std::move(node), parsePower());
        }
        return node;
    }
 
    // Exponentiation — right-associative so 2^3^2 = 2^(3^2) = 512
    std::unique_ptr<Node> parsePower() {
        auto base = parseUnary();
        if (peek().type == TOKEN_OP && peek().value == "^") {
            consume();
            auto exp = parsePower(); // Right-recursive
            base = std::make_unique<BinaryOpNode>("^", std::move(base), std::move(exp));
        }
        return base;
    }
 
    // Unary minus or plus before an expression
    std::unique_ptr<Node> parseUnary() {
        if (peek().type == TOKEN_OP && peek().value == "+") { consume(); return parseUnary(); }
        if (peek().type == TOKEN_OP && peek().value == "-") {
            consume();
            auto f = parseUnary();
            // Fold -number immediately so we store -3 not (-1)*3
            if (f->isConstant()) { return std::make_unique<ConstantNode>(-f->getValue()); }
            return std::make_unique<BinaryOpNode>("*", std::make_unique<ConstantNode>(-1), std::move(f));
        }
        return parsePrimary();
    }
 
    // Atoms: numbers, variables, function calls, parenthesized expressions
    std::unique_ptr<Node> parsePrimary() {
        Token tok = peek();
 
        if (tok.type == TOKEN_NUM) {
            consume();
            return std::make_unique<ConstantNode>(std::stod(tok.value));
        }
        if (tok.type == TOKEN_VAR) {
            consume();
            if (tok.value == "pi") return std::make_unique<ConstantNode>(M_PI); // pi → 3.14159…
            return std::make_unique<VariableNode>(tok.value);
        }
        if (tok.type == TOKEN_FUNC) {
            std::string fname = consume().value;
            if (consume().type != TOKEN_LPAREN)
                throw std::runtime_error("Expected '(' after '" + fname + "'");
            auto arg = parseExpression();
            if (consume().type != TOKEN_RPAREN)
                throw std::runtime_error("Expected ')' to close '" + fname + "(...)'");
            return std::make_unique<FunctionNode>(fname, std::move(arg));
        }
        if (tok.type == TOKEN_LPAREN) {
            consume();
            auto node = parseExpression();
            if (consume().type != TOKEN_RPAREN)
                throw std::runtime_error("Mismatched parenthesis — missing ')'");
            return node;
        }
 
        throw std::runtime_error("Unexpected token: '" + tok.value + "'");
    }
 
public:
    explicit Parser(std::vector<Token> t) : tokens(std::move(t)) {}
 
    std::unique_ptr<Node> parse() {
        auto result = parseExpression();
        if (peek().type != TOKEN_EOF)
            throw std::runtime_error("Unexpected trailing token: '" + peek().value + "'");
        return result;
    }
};
 
// ============================================================================
//  SECTION 10 — HIGHER-LEVEL MATH OPERATIONS
//  These functions take a parsed Node* and perform extended analysis on it.
// ============================================================================
 
// Helper to extract coefficients of a quadratic s-polynomial: as^2 + bs + c
void extractSPoly(Node* n, double& a, double& b, double& c) {
    a = 0; b = 0; c = 0;
    if (n == nullptr) return;
    if (n->isConstant()) { c = n->getValue(); return; }
    if (n->isVariable() && n->getVarName() == "s") { b = 1.0; return; }
    if (n->isBinaryOp()) {
        std::string op = n->getOp();
        if (op == "+") {
            double a1, b1, c1, a2, b2, c2;
            extractSPoly(n->getLeft(), a1, b1, c1);
            extractSPoly(n->getRight(), a2, b2, c2);
            a = a1 + a2; b = b1 + b2; c = c1 + c2;
        } else if (op == "-") {
            double a1, b1, c1, a2, b2, c2;
            extractSPoly(n->getLeft(), a1, b1, c1);
            extractSPoly(n->getRight(), a2, b2, c2);
            a = a1 - a2; b = b1 - b2; c = c1 - c2;
        } else if (op == "*" && n->getLeft()->isConstant()) {
            extractSPoly(n->getRight(), a, b, c);
            double k = n->getLeft()->getValue(); a *= k; b *= k; c *= k;
        } else if (op == "^" && n->getLeft()->isVariable() && n->getLeft()->getVarName() == "s" && n->getRight()->isConstant()) {
            double p = n->getRight()->getValue();
            if (p == 2.0) a = 1.0; else if (p == 1.0) b = 1.0; else if (p == 0.0) c = 1.0;
        }
    }
}

// --- Nth Derivative ---
// Applies diff() and simplify() n times to obtain f^(n)(t).
std::unique_ptr<Node> nthDerivative(Node* node, int n) {
    std::unique_ptr<Node> current = node->clone();
    for (int i = 0; i < n; ++i) {
        gLog.log("--- Pass " + std::to_string(i + 1) + " of " + std::to_string(n) + " ---");
        current = current->diff()->simplify();
    }
    return current;
}
 
// --- Maclaurin / Taylor Series (expanded around t = a) ---
// T(t) = Σ [f^(n)(a) / n!] · (t-a)^n   for n = 0 … (terms-1)
std::string taylorSeries(const Node* node, double a, int terms) {
    std::ostringstream out;
    out << "T(t) = ";
 
    std::unique_ptr<Node> current = node->clone();
    bool firstTerm = true;
 
    for (int n = 0; n < terms; ++n) {
        double coeffFull = current->eval(a);              // f^(n)(a)
        double coeff     = coeffFull / factorial(n);      // Divide by n!
 
        if (std::fabs(coeff) > 1e-10) {
            if (!firstTerm)       out << (coeff >= 0 ? " + " : " - ");
            else if (coeff < 0)   out << "-";
            firstTerm = false;
            double absCoeff = std::fabs(coeff);
 
            if (n == 0) {
                out << std::setprecision(4) << absCoeff;
            } else if (n == 1) {
                if (absCoeff != 1.0) out << std::setprecision(4) << absCoeff << "·";
                out << (a == 0.0 ? "t" : "(t - " + fmtNum(a) + ")");
            } else {
                if (absCoeff != 1.0) out << std::setprecision(4) << absCoeff << "·";
                out << (a == 0.0 ? "t^" + std::to_string(n)
                                 : "(t-" + fmtNum(a) + ")^" + std::to_string(n));
            }
        }
 
        // Differentiate for the next pass (if more terms remain)
        if (n < terms - 1) {
            current = current->diff()->simplify();
        }
    }
 
    if (firstTerm) out << "0";
    out << " + ...";
    return out.str();
}
 
// --- Initial Value Theorem ---
// States: lim(t→0⁺) f(t) = lim(s→∞) s·F(s)
// We verify by evaluating f(t) directly at t=0.
std::string initialValueTheorem(Node* f_t) {
    double iv = f_t->eval(0.0);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "IVT: lim(t→0+) f(t) = f(0) = " << iv << "\n";
    ss << "     (Confirm: lim(s→∞) s·F(s) must also equal " << iv << ")";
    return ss.str();
}
 
// --- Final Value Theorem ---
// States: lim(t→∞) f(t) = lim(s→0) s·F(s)
// Valid ONLY when all poles of F(s) are in the left half of the complex plane.
std::string finalValueTheorem(Node* laplace_F) {
    // Build the expression s·F(s) and display it
    auto simp = std::make_unique<BinaryOpNode>("*", std::make_unique<VariableNode>("s"), laplace_F->clone())->simplify();
    std::ostringstream ss;
    ss << "FVT: lim(t→∞) f(t) = lim(s→0) s·F(s)\n";
    ss << "     s·F(s) = " << simp->toString() << "\n";
    ss << "     (Valid only when all poles of F(s) are in the left half-plane)";
    return ss.str();
}
 
// --- Fourier Transform (basic causal signals) ---
// For causal signals f(t) (zero for t<0), FT = Laplace at s = jω.
// For bilateral signals (sin, cos), uses known spectral pairs.
std::string fourierTransform(Node* node) {
    // Known spectral pairs for bilateral signals
    if (node->isFunction()) {
        double b;
        if (node->getFuncName() == "sin" && extractLinearCoeff(node->getArg(), b))
            return "F(ω) = jπ [ δ(ω+" + fmtNum(b) + ") - δ(ω-" + fmtNum(b) + ") ]";
        if (node->getFuncName() == "cos" && extractLinearCoeff(node->getArg(), b))
            return "F(ω) = π [ δ(ω+" + fmtNum(b) + ") + δ(ω-" + fmtNum(b) + ") ]";
    }
    if (node->isConstant())
        return "F(ω) = " + fmtNum(node->getValue()) + " · 2π·δ(ω)  (DC signal)";
 
    // For causal signals: substitute s = jω into the Laplace result
    std::string lapStr = node->laplace()->simplify()->toString();
    return "F(ω) = F(s)|_{s=jω}  where F(s) = " + lapStr
         + "\n       → substitute s = jω to obtain F(ω)";
}

// --- Numerical Root Finding (Newton's Method) ---
std::string solveRoots(const Node* node, double start) {
    double x = start;
    for (int i = 0; i < 10; ++i) {
        double val = node->eval(x);
        double dv = node->diff()->eval(x);
        if (std::abs(dv) < 1e-10) break;
        x = x - val / dv;
    }
    return "Root near t=" + fmtNum(start) + " is approx " + fmtNum(x);
}
 
// ============================================================================
//  SECTION 11 — RESULT DISPLAY
//  Formats all operation results with section headers, step workings,
//  and numerical tables for easy reading.
// ============================================================================
 
void showNumerical(const Node* root) {
    std::cout << "\n[1] NUMERICAL EVALUATION\n";
    printLine();
    std::cout << std::fixed << std::setprecision(6);
    for (double t : {0.0, 0.25, 0.5, 1.0, 2.0, M_PI})
        std::cout << "  f(" << std::setw(6) << t << ") = " << root->eval(t) << "\n";
}

void showDiff(const Node* root) {
    std::cout << "\n[2] DIFFERENTIATION - d/dt [ f(t) ]\n";
    printLine();
    gLog.clear();
    auto d1 = root->diff()->simplify();
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  f'(t) = " << d1->toString() << "\n";
    std::cout << "  LaTeX   :         " << d1->toLatex()  << "\n";
}

void showSecondDiff(const Node* root) {
    std::cout << "\n[3] SECOND DERIVATIVE - d^2/dt^2 [ f(t) ]\n";
    printLine();
    gLog.clear();
    auto d2 = nthDerivative(const_cast<Node*>(root), 2); // Helper uses Node*
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  f''(t) = " << d2->toString() << "\n";
}

void showIntegration(const Node* root) {
    std::cout << "\n[4] INTEGRATION - Integral f(t) dt\n";
    printLine();
    gLog.clear();
    auto intg = root->integrate()->simplify();
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  F(t) = " << intg->toString() << " + C\n";
    std::cout << "  LaTeX   :        " << intg->toLatex()  << "\n";
}

void showSeries(const Node* root) {
    std::cout << "\n[5] MACLAURIN SERIES (expanded around t=0, 6 terms)\n";
    printLine();
    gLog.clear();
    std::cout << "  " << taylorSeries(root, 0.0, 6) << "\n"; // Helper uses const Node*
}

void showLaplace(const Node* root) {
    std::cout << "\n[6] LAPLACE TRANSFORM - L{ f(t) } = F(s)\n";
    printLine();
    gLog.clear();
    auto Fs = root->laplace()->simplify();
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  F(s) = " << Fs->toString() << "\n";
    std::cout << "  LaTeX   :        " << Fs->toLatex()  << "\n";
}

void showValueTheorems(const Node* root) {
    std::cout << "\n[7] VALUE THEOREMS\n";
    printLine();
    auto Fs = root->laplace()->simplify();
    std::cout << "  " << initialValueTheorem(const_cast<Node*>(root)) << "\n";
    std::cout << "  " << finalValueTheorem(Fs.get())     << "\n";
}

void showInvLaplace(const Node* root) {
    std::cout << "\n[8] INVERSE LAPLACE - L^-1{ F(s) }\n";
    printLine();
    gLog.clear();
    auto Fs = root->laplace()->simplify();
    auto inv = Fs->invLaplace();
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  L^-1{F(s)} = " << inv->toString() << "\n";
}

void showFourier(const Node* root) {
    std::cout << "\n[9] FOURIER TRANSFORM - F{ f(t) } = F(w)\n";
    printLine();
    gLog.clear();
    std::cout << "  " << fourierTransform(const_cast<Node*>(root)) << "\n";
}

void showRoots(const Node* root) {
    std::cout << "\n[10] NUMERICAL ROOTS\n";
    printLine();
    std::cout << "  " << solveRoots(root, 1.0) << "\n";
}

void printResults(const std::unique_ptr<Node>& root, const std::string& input) {
    std::cout << "\n";
    printLine('=', 65);
    std::cout << "  INPUT    :  f(t) = " << input           << "\n";
    std::cout << "  PARSED   :  f(t) = " << root->toString() << "\n";
    std::cout << "  LaTeX    :  f(t) = " << root->toLatex()  << "\n";
    printLine('=', 65);
 
    showNumerical(root.get());
    showDiff(root.get());
    showSecondDiff(root.get());
    showIntegration(root.get());
    showSeries(root.get());
    showLaplace(root.get());
    showValueTheorems(root.get());
    showInvLaplace(root.get());
    showFourier(root.get());
    showRoots(root.get());

    printLine('=', 65);
    std::cout << "\n";
}

void printManual() {
    printLine('=', 65);
    std::cout << "  ENGINE CAPABILITIES & DOCUMENTATION\n";
    std::cout << "  ENGINE CAPABILITIES & USER MANUAL\n";
    printLine('-', 65);
    std::cout << "  1. DERIVATIVES : 1st and 2nd symbolic derivatives.\n"
              << "  2. INTEGRALS   : Indefinite integration with Parts & Substitution.\n"
              << "  3. LAPLACE     : L{f(t)} with Shift Theorems support.\n"
              << "  4. INV LAPLACE : L^-1{F(s)} via Quadratic Partial Fractions.\n"
              << "  5. SERIES      : 6-term Maclaurin expansion.\n"
              << "  6. FOURIER     : Spectral analysis for causal/bilateral signals.\n"
              << "  7. THEOREMS    : Initial (IVT) and Final (FVT) value checks.\n"
              << "  8. NUMERICAL   : Newton-Rhapson root finding & point eval.\n";
    std::cout << "  CORE FUNCTIONALITIES:\n"
              << "  - CALCULUS     : Symbolic 1st/2nd derivatives & indefinite integrals.\n"
              << "  - LAPLACE      : L{f(t)} with First/Second Shift Theorems.\n"
              << "  - INV LAPLACE  : L^-1{F(s)} via Partial Fraction Expansion (PFE).\n"
              << "  - SPECTRAL     : Fourier transforms for causal & bilateral signals.\n"
              << "  - ANALYSIS     : Maclaurin series & Initial/Final Value Theorems.\n"
              << "  - NUMERICAL    : Newton-Rhapson root solving & point evaluation.\n";
    printLine('-', 65);
    std::cout << "  SYNTAX RULES:\n";
    std::cout << "  INPUT SYNTAX:\n"
              << "  - Use 't' for time domain, 's' for Laplace domain.\n"
              << "  - Implicit Multiplication: '2t' is read as '2*t'.\n"
              << "  - Step Function: 'u(t)' or 'u(t-3)' for unit steps.\n"
              << "  - Constants: 'e' (Euler), 'pi' (3.14159).\n";
    printLine('-', 65);
    std::cout << "  LIMITATIONS:\n"
              << "  - Inverse Laplace PFE currently supports up to 2nd order poles.\n"
              << "  - FVT does not strictly verify system stability (LHP poles).\n"
              << "  - Integrals of non-standard non-linear arguments may stay symbolic.\n";
    std::cout << "  CURRENT LIMITATIONS:\n"
              << "  - Inverse Laplace PFE handles up to 2nd order (quadratic) poles.\n"
              << "  - Integration by Parts is optimized for t*sin(t), t*cos(t), t*exp(t).\n"
              << "  - Complex PFE residues are calculated for distinct/repeated poles.\n"
              << "  - Non-standard functions in Laplace domain may return symbolic tags.\n";
    printLine('=', 65);
}
 
// ============================================================================
//  SECTION 12 — MAIN REPL
//  Interactive loop with history, recall, and steps toggle.
//
//  Commands (type at the prompt):
//    help         — show detailed functionality list
//    history      — list all previous inputs
//    recall N     — re-run input number N from history
//    steps on     — show step-by-step workings  (default)
//    steps off    — hide step-by-step workings
//    exit / quit  — close the program
// ============================================================================
 
int main() {
#ifdef _WIN32
    // Set console code page to UTF-8 (65001) to support symbols like π, ∫, δ
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::vector<std::string> history;
    std::string input;
 
    std::cout << "\n";
    printLine('*', 65);
    std::cout << "  MULTI-DOMAIN MATH ENGINE — Extended Edition\n\n";
    std::cout << "  Operators : + - * / ^ (implicit * also works: 2t, 3sin)\n";
    std::cout << "  Functions : sin cos tan asin acos atan ln sqrt abs exp sinh cosh tanh asinh acosh atanh u\n";
    std::cout << "  Variables : t  (time)   e  (Euler)   pi  (3.14159...)\n\n";
    std::cout << "  Examples  : t^3 - 2t + 1\n";
    std::cout << "              e^(-2*t)*cos(3t)\n";
    std::cout << "              u(t) - u(t-2)\n\n";
    std::cout << "  Commands  : history | recall N | steps on/off | exit\n";
    printLine('*', 65);
 
    printManual();

    while (true) {
        std::cout << "\nf(t) > ";
        std::getline(std::cin, input);
 
        // Strip leading/trailing whitespace
        auto st = input.find_first_not_of(" \t");
        auto en = input.find_last_not_of(" \t");
        if (st == std::string::npos) continue;
        input = input.substr(st, en - st + 1);
 
        // --- Built-in Commands ---
        if (input == "exit" || input == "quit") {
            std::cout << "Goodbye.\n\n"; break;
        }
        if (input == "help") {
            printManual();
            continue;
        }
        if (input == "history") {
            if (history.empty()) { std::cout << "  No history yet.\n"; continue; }
            for (size_t i = 0; i < history.size(); ++i)
                std::cout << "  [" << (i + 1) << "] " << history[i] << "\n";
            continue;
        }
        if (input.rfind("recall ", 0) == 0) {
            try {
                long long n = std::stoll(input.substr(7));
                if (n < 1 || static_cast<size_t>(n) > history.size())
                    { std::cout << "  Index out of range.\n"; continue; }
                input = history[static_cast<size_t>(n) - 1];
                std::cout << "  Recalling: " << input << "\n";
            } catch (...) { std::cout << "  Usage: recall N\n"; continue; }
        }
        if (input == "steps off") { gLog.enabled = false; std::cout << "  Workings hidden.\n";  continue; }
        if (input == "steps on")  { gLog.enabled = true;  std::cout << "  Workings visible.\n"; continue; }
 
        // --- Parse and Evaluate ---
        try {
            Lexer  lexer(input);
            auto   tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto   root   = parser.parse();
 
            history.push_back(input);
            
            bool backToMain = false;
            while (!backToMain) {
                std::cout << "\n--- Operation Menu for: " << root->toString() << " ---\n"
                          << "  1. Full Analysis       6. Laplace Transform\n"
                          << "  2. Numerical Eval      7. Value Theorems (IVT/FVT)\n"
                          << "  3. 1st Derivative      8. Inverse Laplace\n"
                          << "  4. 2nd Derivative      9. Fourier Transform\n"
                          << "  5. Integration        10. Numerical Roots\n"
                          << "  0. New Expression\n"
                          << "Choice > ";
                
                std::string choiceStr;
                std::getline(std::cin, choiceStr);
                int choice = 0;
                try { choice = std::stoi(choiceStr); } catch(...) { continue; }

                switch(choice) {
                    case 1: printResults(root, input); break;
                    case 2: showNumerical(root.get()); break;
                    case 3: showDiff(root.get()); break;
                    case 4: showSecondDiff(root.get()); break;
                    case 5: showIntegration(root.get()); break;
                    case 6: showLaplace(root.get()); break;
                    case 7: showValueTheorems(root.get()); break;
                    case 8: showInvLaplace(root.get()); break;
                    case 9: showFourier(root.get()); break;
                    case 10: showRoots(root.get()); break;
                    case 0: backToMain = true; break;
                    default: std::cout << "Invalid choice.\n";
                }
                
                if (!backToMain && choice != 1) {
                    std::cout << "\nPress Enter to return to menu...";
                    std::getline(std::cin, choiceStr);
                }
            }
        }
        catch (const std::exception& ex) {
            std::cout << "  ! Error: " << ex.what() << "\n";
        }
    }
 
    return 0;
}
