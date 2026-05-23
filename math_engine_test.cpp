// ============================================================================
//  MULTI-DOMAIN MATH ENGINE — Extended Edition
//  Features: Differentiation, Integration, Laplace, Inverse Laplace,
//            Fourier Transform, Taylor Series, nth Derivative, Numerical Eval,
//            LaTeX Output, Step-by-Step Workings, Implicit Multiplication,
//            Expression History, Initial/Final Value Theorems
//  Functions: sin, cos, tan, asin, acos, atan, ln, sqrt, abs, exp
//  Variables: t (integration var), e (Euler's number), s (Laplace domain), pi
// ============================================================================
 
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
//  Every node method calls gLog.log("explanation") to record what rule it
//  applied. printResults() prints these numbered after each operation.
//  Type 'steps off' at the prompt to silence them.
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
 
// Print a horizontal separator line
void printLine(char ch = '-', int w = 65) {
    std::cout << std::string(w, ch) << "\n";
}
 
// ============================================================================
//  SECTION 3 — FORWARD DECLARATIONS
//  All node types are declared here so each implementation can reference
//  the others (e.g. ConstantNode::integrate() returns a BinaryOpNode).
// ============================================================================
 
class Node;
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
 
    virtual std::string toString()           = 0;
    virtual std::string toLatex()            = 0;
    virtual double      eval(double t) const = 0;
    virtual Node*       diff()               = 0;
    virtual Node*       integrate()          = 0;
    virtual Node*       laplace()            = 0;
    virtual Node*       invLaplace()         = 0;
    virtual Node*       simplify()           = 0;
    virtual Node*       clone()        const = 0;
 
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
 
// --- Variable: a named symbol such as t, e, s ---
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
 
// --- Function: a named function applied to one argument, e.g. sin(t) ---
class FunctionNode : public Node {
    std::string name;
    Node*       arg;           // This node owns its argument
public:
    FunctionNode(std::string nm, Node* a) : name(std::move(nm)), arg(a) {}
    FunctionNode(const FunctionNode&)            = delete; // No copy (owns pointer)
    FunctionNode& operator=(const FunctionNode&) = delete;
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
 
// --- Binary Operation: left op right (e.g. t^2 + sin(t)) ---
class BinaryOpNode : public Node {
    std::string op;
    Node*       left;          // This node owns left and right
    Node*       right;
public:
    BinaryOpNode(std::string operation, Node* l, Node* r)
        : op(std::move(operation)), left(l), right(r) {}
    BinaryOpNode(const BinaryOpNode&)            = delete; // No copy (owns pointers)
    BinaryOpNode& operator=(const BinaryOpNode&) = delete;
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
//  SECTION 7A — CONSTANT NODE IMPLEMENTATIONS
// ============================================================================
 
std::string ConstantNode::toString() { return fmtNum(value); }
 
// Negative exponents need braces in LaTeX: e^{-3}
std::string ConstantNode::toLatex() {
    return (value < 0) ? "{" + fmtNum(value) + "}" : fmtNum(value);
}
 
double ConstantNode::eval(double /*t*/) const { return value; }
Node*  ConstantNode::clone()    const         { return new ConstantNode(value); }
Node*  ConstantNode::simplify()               { return clone(); }
 
Node* ConstantNode::diff() {
    // Rule: d/dt[c] = 0
    gLog.log("Constant rule: d/dt[" + fmtNum(value) + "] = 0");
    return new ConstantNode(0);
}
 
Node* ConstantNode::integrate() {
    // Rule: ∫c dt = c·t
    gLog.log("Constant integral: ∫" + fmtNum(value) + " dt = " + fmtNum(value) + "·t");
    return new BinaryOpNode("*", new ConstantNode(value), new VariableNode("t"));
}
 
Node* ConstantNode::laplace() {
    // Rule: L{c} = c/s
    gLog.log("Laplace of constant: L{" + fmtNum(value) + "} = " + fmtNum(value) + "/s");
    return new BinaryOpNode("/", new ConstantNode(value), new VariableNode("s"));
}
 
Node* ConstantNode::invLaplace() {
    // A bare constant in s-domain corresponds to a scaled impulse
    gLog.log("L⁻¹{c} = c·δ(t)  (scaled impulse)");
    return new SymbolicNode(fmtNum(value) + "·δ(t)");
}
 
// ============================================================================
//  SECTION 7B — VARIABLE NODE IMPLEMENTATIONS
// ============================================================================
 
std::string VariableNode::toString() { return name; }
std::string VariableNode::toLatex()  { return name; }
 
double VariableNode::eval(double t) const {
    if (name == "t")  return t;
    if (name == "e")  return M_E;
    if (name == "pi") return M_PI;
    return std::numeric_limits<double>::quiet_NaN(); // s is not evaluable in t-domain
}
 
Node* VariableNode::clone()   const { return new VariableNode(name); }
Node* VariableNode::simplify()      { return clone(); }
 
Node* VariableNode::diff() {
    if (name == "t") {
        gLog.log("Power rule: d/dt[t] = 1");
        return new ConstantNode(1);
    }
    // All other variables are constants with respect to t
    gLog.log("d/dt[" + name + "] = 0  (" + name + " is not a function of t)");
    return new ConstantNode(0);
}
 
Node* VariableNode::integrate() {
    if (name == "t") {
        // ∫t dt = t²/2
        gLog.log("Power rule: ∫t dt = t²/2");
        return new BinaryOpNode("/",
            new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(2)),
            new ConstantNode(2));
    }
    // ∫k dt = k·t  (k constant w.r.t. t)
    gLog.log("∫" + name + " dt = " + name + "·t  (treated as constant w.r.t. t)");
    return new BinaryOpNode("*", new VariableNode(name), new VariableNode("t"));
}
 
Node* VariableNode::laplace() {
    if (name == "t") {
        // L{t} = 1/s²
        gLog.log("Standard pair: L{t} = 1/s²");
        return new BinaryOpNode("/", new ConstantNode(1),
            new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(2)));
    }
    return new BinaryOpNode("/", clone(), new VariableNode("s"));
}
 
Node* VariableNode::invLaplace() {
    if (name == "s") {
        gLog.log("L⁻¹{s} = δ'(t)  (derivative of impulse)");
        return new SymbolicNode("δ'(t)");
    }
    return new SymbolicNode("L⁻¹{" + name + "}");
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
 
std::string FunctionNode::toString() {
    return name + "(" + arg->toString() + ")";
}
 
std::string FunctionNode::toLatex() {
    if (name == "sqrt") return "\\sqrt{" + arg->toLatex() + "}";
    if (name == "ln")   return "\\ln\\!(" + arg->toLatex() + ")";
    if (name == "abs")  return "\\left|" + arg->toLatex() + "\\right|";
    if (name == "exp")  return "e^{" + arg->toLatex() + "}";
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
    return std::numeric_limits<double>::quiet_NaN();
}
 
Node* FunctionNode::clone()   const { return new FunctionNode(name, arg->clone()); }
Node* FunctionNode::simplify()      { return new FunctionNode(name, arg->simplify()); }
 
// --- Chain Rule Derivatives ---
Node* FunctionNode::diff() {
    gLog.log("Chain rule on " + toString() + "  →  outer' · inner'");
 
    Node* u      = arg->clone();  // Inner function u
    Node* uPrime = arg->diff();   // Inner derivative u'
 
    if (name == "sin") {
        // d/dt[sin(u)] = cos(u) · u'
        gLog.log("  sin rule: d/dt[sin(u)] = cos(u) · u'");
        return new BinaryOpNode("*", new FunctionNode("cos", u), uPrime);
    }
    if (name == "cos") {
        // d/dt[cos(u)] = -sin(u) · u'
        gLog.log("  cos rule: d/dt[cos(u)] = -sin(u) · u'");
        return new BinaryOpNode("*",
            new BinaryOpNode("*", new ConstantNode(-1), new FunctionNode("sin", u)),
            uPrime);
    }
    if (name == "tan") {
        // d/dt[tan(u)] = u' / cos²(u)
        gLog.log("  tan rule: d/dt[tan(u)] = u' / cos²(u)");
        return new BinaryOpNode("/", uPrime,
            new BinaryOpNode("^", new FunctionNode("cos", u), new ConstantNode(2)));
    }
    if (name == "ln") {
        // d/dt[ln(u)] = u' / u
        gLog.log("  ln rule: d/dt[ln(u)] = u'/u");
        return new BinaryOpNode("/", uPrime, u);
    }
    if (name == "sqrt") {
        // d/dt[√u] = u' / (2√u)
        gLog.log("  sqrt rule: d/dt[√u] = u' / (2√u)");
        return new BinaryOpNode("/", uPrime,
            new BinaryOpNode("*", new ConstantNode(2), new FunctionNode("sqrt", u)));
    }
    if (name == "exp") {
        // d/dt[exp(u)] = exp(u) · u'
        gLog.log("  exp rule: d/dt[exp(u)] = exp(u) · u'");
        return new BinaryOpNode("*", new FunctionNode("exp", u), uPrime);
    }
    if (name == "abs") {
        // d/dt[|u|] = (u/|u|) · u'  — equivalent to sign(u) · u'
        gLog.log("  abs rule: d/dt[|u|] = sign(u) · u'");
        return new BinaryOpNode("*",
            new BinaryOpNode("/", u, new FunctionNode("abs", arg->clone())),
            uPrime);
    }
    if (name == "asin") {
        // d/dt[asin(u)] = u' / √(1 - u²)
        gLog.log("  asin rule: d/dt[asin(u)] = u' / √(1-u²)");
        return new BinaryOpNode("/", uPrime,
            new FunctionNode("sqrt",
                new BinaryOpNode("-", new ConstantNode(1),
                    new BinaryOpNode("^", u, new ConstantNode(2)))));
    }
    if (name == "acos") {
        // d/dt[acos(u)] = -u' / √(1 - u²)
        gLog.log("  acos rule: d/dt[acos(u)] = -u' / √(1-u²)");
        return new BinaryOpNode("/",
            new BinaryOpNode("*", new ConstantNode(-1), uPrime),
            new FunctionNode("sqrt",
                new BinaryOpNode("-", new ConstantNode(1),
                    new BinaryOpNode("^", u, new ConstantNode(2)))));
    }
    if (name == "atan") {
        // d/dt[atan(u)] = u' / (1 + u²)
        gLog.log("  atan rule: d/dt[atan(u)] = u' / (1+u²)");
        return new BinaryOpNode("/", uPrime,
            new BinaryOpNode("+", new ConstantNode(1),
                new BinaryOpNode("^", u, new ConstantNode(2))));
    }
 
    delete u; delete uPrime;
    gLog.log("  No rule for d/dt[" + name + "(...)] — returning symbolic");
    return new SymbolicNode("d/dt(" + toString() + ")");
}
 
// --- Function Integrals ---
Node* FunctionNode::integrate() {
    double b = 1.0;
    bool linear = extractLinearCoeff(arg, b); // Checks if arg = b*t
 
    if (linear) {
        if (name == "sin") {
            // ∫sin(bt) dt = -cos(bt)/b
            gLog.log("∫sin(bt)dt = -cos(bt)/b   [b = " + fmtNum(b) + "]");
            return new BinaryOpNode("*", new ConstantNode(-1.0 / b),
                new FunctionNode("cos", arg->clone()));
        }
        if (name == "cos") {
            // ∫cos(bt) dt = sin(bt)/b
            gLog.log("∫cos(bt)dt = sin(bt)/b   [b = " + fmtNum(b) + "]");
            return new BinaryOpNode("*", new ConstantNode(1.0 / b),
                new FunctionNode("sin", arg->clone()));
        }
        if (name == "tan") {
            // ∫tan(bt) dt = -ln|cos(bt)|/b
            gLog.log("∫tan(bt)dt = -ln|cos(bt)|/b   [b = " + fmtNum(b) + "]");
            return new BinaryOpNode("*", new ConstantNode(-1.0 / b),
                new FunctionNode("ln",
                    new FunctionNode("abs", new FunctionNode("cos", arg->clone()))));
        }
        if (name == "exp") {
            // ∫exp(bt) dt = exp(bt)/b
            gLog.log("∫exp(bt)dt = exp(bt)/b   [b = " + fmtNum(b) + "]");
            return new BinaryOpNode("*", new ConstantNode(1.0 / b),
                new FunctionNode("exp", arg->clone()));
        }
        if (name == "ln" && b == 1.0) {
            // ∫ln(t) dt = t·ln(t) - t   (integration by parts)
            gLog.log("∫ln(t)dt = t·ln(t) - t   [by parts: u=ln(t), dv=dt]");
            return new BinaryOpNode("-",
                new BinaryOpNode("*", new VariableNode("t"),
                    new FunctionNode("ln", new VariableNode("t"))),
                new VariableNode("t"));
        }
        if (name == "sqrt" && b == 1.0) {
            // ∫√t dt = (2/3)·t^(3/2)
            gLog.log("∫√t dt = (2/3)·t^(3/2)");
            return new BinaryOpNode("*", new ConstantNode(2.0 / 3.0),
                new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(1.5)));
        }
    }
 
    gLog.log("No closed-form integral for " + toString() + " — returning symbolic");
    return new SymbolicNode("∫(" + toString() + ") dt");
}
 
// --- Function Laplace Transforms ---
Node* FunctionNode::laplace() {
    double b = 1.0;
    bool linear = extractLinearCoeff(arg, b);
 
    if (linear) {
        if (name == "sin") {
            // L{sin(bt)} = b / (s² + b²)
            gLog.log("Pair: L{sin(bt)} = b/(s²+b²)   [b = " + fmtNum(b) + "]");
            return new BinaryOpNode("/", new ConstantNode(b),
                new BinaryOpNode("+",
                    new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(2)),
                    new ConstantNode(b * b)));
        }
        if (name == "cos") {
            // L{cos(bt)} = s / (s² + b²)
            gLog.log("Pair: L{cos(bt)} = s/(s²+b²)   [b = " + fmtNum(b) + "]");
            return new BinaryOpNode("/", new VariableNode("s"),
                new BinaryOpNode("+",
                    new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(2)),
                    new ConstantNode(b * b)));
        }
        if (name == "exp") {
            // L{exp(at)} = 1/(s - a)
            gLog.log("Pair: L{exp(at)} = 1/(s-a)   [a = " + fmtNum(b) + "]");
            return new BinaryOpNode("/", new ConstantNode(1),
                new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(b)));
        }
    }
 
    gLog.log("No Laplace pair for " + toString() + " — returning symbolic");
    return new SymbolicNode("L{" + toString() + "}");
}
 
Node* FunctionNode::invLaplace() {
    gLog.log("No inverse Laplace rule for function " + name + " — returning symbolic");
    return new SymbolicNode("L⁻¹{" + toString() + "}");
}
 
// ============================================================================
//  SECTION 7E — BINARY OP NODE IMPLEMENTATIONS
// ============================================================================
 
std::string BinaryOpNode::toString() {
    std::string L = left->toString(), R = right->toString();
    // Wrap sub-expressions in parens when their precedence is lower than ours
    if (left->isBinaryOp()  && getPrecedence(left->getOp())  < getPrecedence(op))              L = "(" + L + ")";
    if (right->isBinaryOp() && getPrecedence(right->getOp()) < getPrecedence(op))              R = "(" + R + ")";
    if (right->isBinaryOp() && getPrecedence(right->getOp()) == getPrecedence(op) && op == "/") R = "(" + R + ")";
    return L + " " + op + " " + R;
}
 
std::string BinaryOpNode::toLatex() {
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
 
Node* BinaryOpNode::clone() const {
    return new BinaryOpNode(op, left->clone(), right->clone());
}
 
// --- Simplification ---
// Applies constant folding and common algebraic identities.
Node* BinaryOpNode::simplify() {
    Node* L = left->simplify();
    Node* R = right->simplify();
 
    // Fold two constants immediately: 3 * 4 → 12
    if (L->isConstant() && R->isConstant()) {
        double lv = L->getValue(), rv = R->getValue();
        delete L; delete R;
        if (op == "+") return new ConstantNode(lv + rv);
        if (op == "-") return new ConstantNode(lv - rv);
        if (op == "*") return new ConstantNode(lv * rv);
        if (op == "/" && rv != 0.0) return new ConstantNode(lv / rv);
        if (op == "^") return new ConstantNode(std::pow(lv, rv));
    }
 
    // x * 1 = x  |  1 * x = x
    if (op == "*" && R->isConstant() && R->getValue() == 1.0) { delete R; return L; }
    if (op == "*" && L->isConstant() && L->getValue() == 1.0) { delete L; return R; }
 
    // x * 0 = 0  |  0 * x = 0
    if (op == "*" && ((L->isConstant() && L->getValue() == 0.0) ||
                      (R->isConstant() && R->getValue() == 0.0))) {
        delete L; delete R; return new ConstantNode(0);
    }
 
    // x + 0 = x  |  0 + x = x
    if (op == "+" && R->isConstant() && R->getValue() == 0.0) { delete R; return L; }
    if (op == "+" && L->isConstant() && L->getValue() == 0.0) { delete L; return R; }
 
    // x - 0 = x
    if (op == "-" && R->isConstant() && R->getValue() == 0.0) { delete R; return L; }
 
    // x^1 = x  |  x^0 = 1
    if (op == "^" && R->isConstant() && R->getValue() == 1.0) { delete R; return L; }
    if (op == "^" && R->isConstant() && R->getValue() == 0.0) {
        delete L; delete R; return new ConstantNode(1);
    }
 
    return new BinaryOpNode(op, L, R);
}
 
// --- Differentiation ---
Node* BinaryOpNode::diff() {
    if (op == "+" || op == "-") {
        // Sum/Difference Rule: (f ± g)' = f' ± g'
        gLog.log("Sum/difference rule: (f " + op + " g)' = f' " + op + " g'");
        return new BinaryOpNode(op, left->diff(), right->diff());
    }
 
    if (op == "*") {
        // Product Rule: (f·g)' = f'·g + f·g'
        gLog.log("Product rule: (f·g)' = f'·g + f·g'"
                 "   where f = " + left->toString() + ",  g = " + right->toString());
        return new BinaryOpNode("+",
            new BinaryOpNode("*", left->diff(),  right->clone()),
            new BinaryOpNode("*", left->clone(), right->diff()));
    }
 
    if (op == "/") {
        // Quotient Rule: (f/g)' = (f'g - fg') / g²
        gLog.log("Quotient rule: (f/g)' = (f'g - fg') / g²"
                 "   where f = " + left->toString() + ",  g = " + right->toString());
        return new BinaryOpNode("/",
            new BinaryOpNode("-",
                new BinaryOpNode("*", left->diff(),  right->clone()),
                new BinaryOpNode("*", left->clone(), right->diff())),
            new BinaryOpNode("^", right->clone(), new ConstantNode(2)));
    }
 
    if (op == "^") {
        // Power rule: d/dt[t^n] = n · t^(n-1)
        if (left->isVariable() && left->getVarName() == "t" && right->isConstant()) {
            double n = right->getValue();
            gLog.log("Power rule: d/dt[t^" + fmtNum(n) + "] = "
                     + fmtNum(n) + "·t^" + fmtNum(n - 1));
            return new BinaryOpNode("*",
                new ConstantNode(n),
                new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(n - 1)));
        }
        // Exponential rule: d/dt[e^f] = e^f · f'
        if (left->isVariable() && left->getVarName() == "e") {
            gLog.log("Exponential rule: d/dt[e^f] = e^f · f'"
                     "   where f = " + right->toString());
            return new BinaryOpNode("*", clone(), right->diff());
        }
        // General base: d/dt[a^u] = a^u · ln(a) · u'
        if (left->isConstant()) {
            double a = left->getValue();
            gLog.log("General power: d/dt[" + fmtNum(a) + "^u] = "
                     + fmtNum(a) + "^u · ln(" + fmtNum(a) + ") · u'");
            return new BinaryOpNode("*",
                new BinaryOpNode("*", clone(), new ConstantNode(std::log(a))),
                right->diff());
        }
    }
 
    gLog.log("No differentiation rule for (" + toString() + ") — returning symbolic");
    return new SymbolicNode("d/dt(" + toString() + ")");
}
 
// --- Integration ---
Node* BinaryOpNode::integrate() {
    if (op == "+" || op == "-") {
        // Linearity: ∫(f ± g) dt = ∫f dt ± ∫g dt
        gLog.log("Linearity: ∫(f " + op + " g) dt = ∫f dt " + op + " ∫g dt");
        return new BinaryOpNode(op, left->integrate(), right->integrate());
    }
 
    // Constant multiple: ∫c·f dt = c·∫f dt
    if (op == "*" && left->isConstant()) {
        gLog.log("Constant factor: ∫" + left->toString() + "·f dt = "
                 + left->toString() + "·∫f dt");
        return new BinaryOpNode("*", left->clone(), right->integrate());
    }
    if (op == "*" && right->isConstant()) {
        gLog.log("Constant factor: ∫f·" + right->toString() + " dt = "
                 + right->toString() + "·∫f dt");
        return new BinaryOpNode("*", left->integrate(), right->clone());
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
                return new BinaryOpNode("+",
                    new BinaryOpNode("*", new ConstantNode(-1.0 / b),
                        new BinaryOpNode("*", new VariableNode("t"),
                            new FunctionNode("cos", right->getArg()->clone()))),
                    new BinaryOpNode("*", new ConstantNode(1.0 / (b * b)),
                        new FunctionNode("sin", right->getArg()->clone())));
            }
        }
        // ∫t·cos(bt) dt = t·sin(bt)/b + cos(bt)/b²
        if (leftIsT && right->isFunction() && right->getFuncName() == "cos") {
            double b;
            if (extractLinearCoeff(right->getArg(), b)) {
                gLog.log("Integration by parts: ∫t·cos(bt)dt = t·sin(bt)/b + cos(bt)/b²   [b = " + fmtNum(b) + "]");
                return new BinaryOpNode("+",
                    new BinaryOpNode("*", new ConstantNode(1.0 / b),
                        new BinaryOpNode("*", new VariableNode("t"),
                            new FunctionNode("sin", right->getArg()->clone()))),
                    new BinaryOpNode("*", new ConstantNode(1.0 / (b * b)),
                        new FunctionNode("cos", right->getArg()->clone())));
            }
        }
        // ∫t·e^(at) dt = e^(at)·(t/a - 1/a²)
        bool rightIsExp = right->isBinaryOp() && right->getOp() == "^" &&
                          right->getLeft()->isVariable() && right->getLeft()->getVarName() == "e";
        if (leftIsT && rightIsExp) {
            double a;
            if (extractLinearCoeff(right->getRight(), a) && a != 0.0) {
                gLog.log("Integration by parts: ∫t·e^(at)dt = e^(at)·(t/a - 1/a²)   [a = " + fmtNum(a) + "]");
                return new BinaryOpNode("*", right->clone(),
                    new BinaryOpNode("-",
                        new BinaryOpNode("/", new VariableNode("t"), new ConstantNode(a)),
                        new ConstantNode(1.0 / (a * a))));
            }
        }
 
        // Mirror: if trig/exp is on the left, swap and retry
        if (rightIsT && (left->isFunction() || (left->isBinaryOp() && left->getOp() == "^"))) {
            Node* swapped = new BinaryOpNode("*", right->clone(), left->clone());
            Node* result  = swapped->integrate();
            delete swapped;
            return result;
        }
    }
 
    // Power rule: ∫t^n dt = t^(n+1) / (n+1)
    if (op == "^" && left->isVariable() && left->getVarName() == "t" && right->isConstant()) {
        double n = right->getValue();
        gLog.log("Power rule: ∫t^" + fmtNum(n) + " dt = t^" + fmtNum(n + 1)
                 + "/" + fmtNum(n + 1));
        return new BinaryOpNode("/",
            new BinaryOpNode("^", new VariableNode("t"), new ConstantNode(n + 1)),
            new ConstantNode(n + 1));
    }
 
    // ∫e^(at) dt = e^(at)/a
    if (op == "^" && left->isVariable() && left->getVarName() == "e") {
        double a;
        if (extractLinearCoeff(right, a) && a != 0.0) {
            gLog.log("Exponential integral: ∫e^(at)dt = e^(at)/a   [a = " + fmtNum(a) + "]");
            return new BinaryOpNode("/", clone(), new ConstantNode(a));
        }
    }
 
    gLog.log("No closed-form integral for (" + toString() + ") — returning symbolic");
    return new SymbolicNode("∫(" + toString() + ") dt");
}
 
// --- Laplace Transform ---
Node* BinaryOpNode::laplace() {
    if (op == "+" || op == "-") {
        // Linearity: L{f ± g} = L{f} ± L{g}
        gLog.log("Linearity: L{f " + op + " g} = L{f} " + op + " L{g}");
        return new BinaryOpNode(op, left->laplace(), right->laplace());
    }
 
    // Scaling: L{c·f} = c·L{f}
    if (op == "*" && left->isConstant()) {
        gLog.log("Scaling: L{" + left->toString() + "·f} = " + left->toString() + "·L{f}");
        return new BinaryOpNode("*", left->clone(), right->laplace());
    }
    if (op == "*" && right->isConstant()) {
        gLog.log("Scaling: L{f·" + right->toString() + "} = " + right->toString() + "·L{f}");
        return new BinaryOpNode("*", left->laplace(), right->clone());
    }
 
    // Product patterns: detect (exp) × (sin | cos | t) from either side
    if (op == "*") {
        // Locate the exponential and the trig/t factor
        auto getExpCoeff = [](Node* n, double& a) -> bool {
            if (n->isBinaryOp() && n->getOp() == "^" &&
                n->getLeft()->isVariable() && n->getLeft()->getVarName() == "e")
                return extractLinearCoeff(n->getRight(), a);
            return false;
        };
 
        double a = 0, b = 0;
        bool foundExp   = false, foundSin = false, foundCos = false, foundT = false;
        Node* expSide = nullptr;
        Node* otherSide = nullptr;
 
        // Try left=exp, right=other
        if (getExpCoeff(left, a)) {
            foundExp = true; expSide = left; otherSide = right;
        } else if (getExpCoeff(right, a)) {
            foundExp = true; expSide = right; otherSide = left;
        }
 
        if (foundExp && otherSide) {
            if (otherSide->isFunction() && otherSide->getFuncName() == "sin" &&
                extractLinearCoeff(otherSide->getArg(), b)) {
                // First Shift: L{e^(at)·sin(bt)} = b / ((s-a)² + b²)
                gLog.log("First shift theorem: L{e^(at)·sin(bt)} = b/((s-a)²+b²)"
                         "   [a=" + fmtNum(a) + " b=" + fmtNum(b) + "]");
                return new BinaryOpNode("/", new ConstantNode(b),
                    new BinaryOpNode("+",
                        new BinaryOpNode("^",
                            new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)),
                            new ConstantNode(2)),
                        new ConstantNode(b * b)));
            }
            if (otherSide->isFunction() && otherSide->getFuncName() == "cos" &&
                extractLinearCoeff(otherSide->getArg(), b)) {
                // First Shift: L{e^(at)·cos(bt)} = (s-a) / ((s-a)² + b²)
                gLog.log("First shift theorem: L{e^(at)·cos(bt)} = (s-a)/((s-a)²+b²)"
                         "   [a=" + fmtNum(a) + " b=" + fmtNum(b) + "]");
                return new BinaryOpNode("/",
                    new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)),
                    new BinaryOpNode("+",
                        new BinaryOpNode("^",
                            new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)),
                            new ConstantNode(2)),
                        new ConstantNode(b * b)));
            }
            if (otherSide->isVariable() && otherSide->getVarName() == "t") {
                // L{t·e^(at)} = 1/(s-a)²
                gLog.log("Pair: L{t·e^(at)} = 1/(s-a)²   [a=" + fmtNum(a) + "]");
                return new BinaryOpNode("/", new ConstantNode(1),
                    new BinaryOpNode("^",
                        new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)),
                        new ConstantNode(2)));
            }
        }
    }
 
    // L{t^n} = n! / s^(n+1)
    if (op == "^" && left->isVariable() && left->getVarName() == "t" && right->isConstant()) {
        int n = static_cast<int>(right->getValue());
        gLog.log("Pair: L{t^n} = n!/s^(n+1)   [n=" + std::to_string(n)
                 + ", n!=" + std::to_string(factorial(n)) + "]");
        return new BinaryOpNode("/",
            new ConstantNode(factorial(n)),
            new BinaryOpNode("^", new VariableNode("s"), new ConstantNode(n + 1)));
    }
 
    // L{e^(at)} = 1/(s-a)
    if (op == "^" && left->isVariable() && left->getVarName() == "e") {
        double a;
        if (extractLinearCoeff(right, a)) {
            gLog.log("Pair: L{e^(at)} = 1/(s-a)   [a=" + fmtNum(a) + "]");
            return new BinaryOpNode("/", new ConstantNode(1),
                new BinaryOpNode("-", new VariableNode("s"), new ConstantNode(a)));
        }
    }
 
    gLog.log("No Laplace rule for (" + toString() + ") — returning symbolic");
    return new SymbolicNode("L{" + toString() + "}");
}
 
// --- Inverse Laplace Transform (pattern matching on F(s)) ---
Node* BinaryOpNode::invLaplace() {
    // Linearity: L⁻¹{F ± G} = L⁻¹{F} ± L⁻¹{G}
    if (op == "+" || op == "-") {
        gLog.log("Linearity: L⁻¹{F " + op + " G} = L⁻¹{F} " + op + " L⁻¹{G}");
        return new BinaryOpNode(op, left->invLaplace(), right->invLaplace());
    }
    // Scaling: L⁻¹{c·F} = c·L⁻¹{F}
    if (op == "*" && left->isConstant()) {
        gLog.log("Scaling: L⁻¹{" + left->toString() + "·F} = " + left->toString() + "·L⁻¹{F}");
        return new BinaryOpNode("*", left->clone(), right->invLaplace());
    }
    if (op == "*" && right->isConstant()) {
        return new BinaryOpNode("*", right->clone(), left->invLaplace());
    }
 
    if (op == "/") {
        // L⁻¹{c/s} = c  (constant in t-domain)
        if (right->isVariable() && right->getVarName() == "s" && left->isConstant()) {
            gLog.log("L⁻¹{c/s} = c   [c = " + fmtNum(left->getValue()) + "]");
            return new ConstantNode(left->getValue());
        }
        // L⁻¹{1/s²} = t
        if (left->isConstant() && left->getValue() == 1.0 &&
            right->isBinaryOp() && right->getOp() == "^" &&
            right->getLeft()->isVariable() && right->getLeft()->getVarName() == "s" &&
            right->getRight()->isConstant() && right->getRight()->getValue() == 2.0) {
            gLog.log("L⁻¹{1/s²} = t");
            return new VariableNode("t");
        }
        // L⁻¹{1/(s-a)} = e^(at)
        if (left->isConstant() && left->getValue() == 1.0 &&
            right->isBinaryOp() && right->getOp() == "-" &&
            right->getLeft()->isVariable() && right->getLeft()->getVarName() == "s" &&
            right->getRight()->isConstant()) {
            double a = right->getRight()->getValue();
            gLog.log("L⁻¹{1/(s-a)} = e^(at)   [a = " + fmtNum(a) + "]");
            return new BinaryOpNode("^", new VariableNode("e"),
                new BinaryOpNode("*", new ConstantNode(a), new VariableNode("t")));
        }
        // L⁻¹{b/(s²+b²)} = sin(bt)
        if (left->isConstant()) {
            double bv = left->getValue();
            if (right->isBinaryOp() && right->getOp() == "+") {
                Node* rL = right->getLeft(); Node* rR = right->getRight();
                if (rL->isBinaryOp() && rL->getOp() == "^" &&
                    rL->getLeft()->isVariable() && rL->getLeft()->getVarName() == "s" &&
                    rL->getRight()->isConstant() && rL->getRight()->getValue() == 2.0 &&
                    rR->isConstant() && std::fabs(std::sqrt(rR->getValue()) - bv) < 1e-9) {
                    gLog.log("L⁻¹{b/(s²+b²)} = sin(bt)   [b = " + fmtNum(bv) + "]");
                    return new FunctionNode("sin",
                        new BinaryOpNode("*", new ConstantNode(bv), new VariableNode("t")));
                }
            }
        }
        // L⁻¹{s/(s²+b²)} = cos(bt)
        if (left->isVariable() && left->getVarName() == "s") {
            if (right->isBinaryOp() && right->getOp() == "+") {
                Node* rL = right->getLeft(); Node* rR = right->getRight();
                if (rL->isBinaryOp() && rL->getOp() == "^" &&
                    rL->getLeft()->isVariable() && rL->getLeft()->getVarName() == "s" &&
                    rL->getRight()->isConstant() && rL->getRight()->getValue() == 2.0 &&
                    rR->isConstant()) {
                    double b = std::sqrt(rR->getValue());
                    gLog.log("L⁻¹{s/(s²+b²)} = cos(bt)   [b = " + fmtNum(b) + "]");
                    return new FunctionNode("cos",
                        new BinaryOpNode("*", new ConstantNode(b), new VariableNode("t")));
                }
            }
        }
    }
 
    gLog.log("No inverse Laplace rule matched for (" + toString() + ") — returning symbolic");
    return new SymbolicNode("L⁻¹{" + toString() + "}");
}
 
// ============================================================================
//  SECTION 8 — LEXER
//  Converts a raw string into a sequence of tokens.
//  Also inserts implicit '*' tokens so "2t" and "3sin(t)" work correctly.
// ============================================================================
 
enum TokenType { TOKEN_NUM, TOKEN_VAR, TOKEN_FUNC, TOKEN_OP,
                 TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_EOF };
 
struct Token { TokenType type; std::string value; };
 
class Lexer {
    std::string src;
    size_t pos = 0;
 
    // Returns true if the identifier is a known function name
    static bool isKnownFunction(const std::string& s) {
        static const std::vector<std::string> fns = {
            "sin","cos","tan","asin","acos","atan","ln","sqrt","abs","exp"
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
                while (pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.'))
                    num += src[pos++];
                raw.push_back({TOKEN_NUM, num});
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
            TokenType cur = raw[i].type, nxt = raw[i + 1].type;
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
    Node* parseExpression() {
        Node* node = parseTerm();
        while (peek().type == TOKEN_OP && (peek().value == "+" || peek().value == "-")) {
            std::string op = consume().value;
            node = new BinaryOpNode(op, node, parseTerm());
        }
        return node;
    }
 
    // Multiplication and division
    Node* parseTerm() {
        Node* node = parsePower();
        while (peek().type == TOKEN_OP && (peek().value == "*" || peek().value == "/")) {
            std::string op = consume().value;
            node = new BinaryOpNode(op, node, parsePower());
        }
        return node;
    }
 
    // Exponentiation — right-associative so 2^3^2 = 2^(3^2) = 512
    Node* parsePower() {
        Node* base = parseUnary();
        if (peek().type == TOKEN_OP && peek().value == "^") {
            consume();
            Node* exp = parsePower(); // Right-recursive
            base = new BinaryOpNode("^", base, exp);
        }
        return base;
    }
 
    // Unary minus or plus before an expression
    Node* parseUnary() {
        if (peek().type == TOKEN_OP && peek().value == "+") { consume(); return parseUnary(); }
        if (peek().type == TOKEN_OP && peek().value == "-") {
            consume();
            Node* f = parseUnary();
            // Fold -number immediately so we store -3 not (-1)*3
            if (f->isConstant()) { double v = f->getValue(); delete f; return new ConstantNode(-v); }
            return new BinaryOpNode("*", new ConstantNode(-1), f);
        }
        return parsePrimary();
    }
 
    // Atoms: numbers, variables, function calls, parenthesized expressions
    Node* parsePrimary() {
        Token tok = peek();
 
        if (tok.type == TOKEN_NUM) {
            consume();
            return new ConstantNode(std::stod(tok.value));
        }
        if (tok.type == TOKEN_VAR) {
            consume();
            if (tok.value == "pi") return new ConstantNode(M_PI); // pi → 3.14159…
            return new VariableNode(tok.value);
        }
        if (tok.type == TOKEN_FUNC) {
            std::string fname = consume().value;
            if (consume().type != TOKEN_LPAREN)
                throw std::runtime_error("Expected '(' after '" + fname + "'");
            Node* arg = parseExpression();
            if (consume().type != TOKEN_RPAREN)
                throw std::runtime_error("Expected ')' to close '" + fname + "(...)'");
            return new FunctionNode(fname, arg);
        }
        if (tok.type == TOKEN_LPAREN) {
            consume();
            Node* node = parseExpression();
            if (consume().type != TOKEN_RPAREN)
                throw std::runtime_error("Mismatched parenthesis — missing ')'");
            return node;
        }
 
        throw std::runtime_error("Unexpected token: '" + tok.value + "'");
    }
 
public:
    explicit Parser(std::vector<Token> t) : tokens(std::move(t)) {}
 
    Node* parse() {
        Node* result = parseExpression();
        if (peek().type != TOKEN_EOF)
            throw std::runtime_error("Unexpected trailing token: '" + peek().value + "'");
        return result;
    }
};
 
// ============================================================================
//  SECTION 10 — HIGHER-LEVEL MATH OPERATIONS
//  These functions take a parsed Node* and perform extended analysis on it.
// ============================================================================
 
// --- Nth Derivative ---
// Applies diff() and simplify() n times to obtain f^(n)(t).
Node* nthDerivative(Node* node, int n) {
    Node* current = node->clone();
    for (int i = 0; i < n; ++i) {
        gLog.log("--- Pass " + std::to_string(i + 1) + " of " + std::to_string(n) + " ---");
        Node* next = current->diff();
        Node* simp = next->simplify();
        delete next;
        delete current;
        current = simp;
    }
    return current;
}
 
// --- Maclaurin / Taylor Series (expanded around t = a) ---
// T(t) = Σ [f^(n)(a) / n!] · (t-a)^n   for n = 0 … (terms-1)
std::string taylorSeries(Node* node, double a, int terms) {
    std::ostringstream out;
    out << "T(t) = ";
 
    Node* current = node->clone();
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
            Node* next = current->diff();
            Node* simp = next->simplify();
            delete next; delete current;
            current = simp;
        }
    }
    delete current;
 
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
    Node* sFs = new BinaryOpNode("*", new VariableNode("s"), laplace_F->clone());
    Node* simp = sFs->simplify();
    delete sFs;
    std::ostringstream ss;
    ss << "FVT: lim(t→∞) f(t) = lim(s→0) s·F(s)\n";
    ss << "     s·F(s) = " << simp->toString() << "\n";
    ss << "     (Valid only when all poles of F(s) are in the left half-plane)";
    delete simp;
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
    Node* lap = node->laplace();
    Node* simp = lap->simplify();
    std::string lapStr = simp->toString();
    delete lap; delete simp;
    return "F(ω) = F(s)|_{s=jω}  where F(s) = " + lapStr
         + "\n       → substitute s = jω to obtain F(ω)";
}
 
// ============================================================================
//  SECTION 11 — RESULT DISPLAY
//  Formats all operation results with section headers, step workings,
//  and numerical tables for easy reading.
// ============================================================================
 
void printResults(Node* root, const std::string& input) {
    std::cout << "\n";
    printLine('=', 65);
    std::cout << "  INPUT    :  f(t) = " << input           << "\n";
    std::cout << "  PARSED   :  f(t) = " << root->toString() << "\n";
    std::cout << "  LaTeX    :  f(t) = " << root->toLatex()  << "\n";
    printLine('=', 65);
 
    // ── Numerical Evaluation Table ────────────────────────────────────────
    std::cout << "\n▶ NUMERICAL EVALUATION\n";
    printLine();
    std::cout << std::fixed << std::setprecision(6);
    for (double t : {0.0, 0.25, 0.5, 1.0, 2.0, M_PI})
        std::cout << "  f(" << std::setw(6) << t << ") = " << root->eval(t) << "\n";
 
    // ── First Derivative ──────────────────────────────────────────────────
    std::cout << "\n▶ DIFFERENTIATION  —  d/dt [ f(t) ]\n";
    printLine();
    gLog.clear();
    Node* rawD  = root->diff();
    Node* d1    = rawD->simplify();
    delete rawD;
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  f'(t) = " << d1->toString() << "\n";
    std::cout << "  LaTeX   :         " << d1->toLatex()  << "\n";
    delete d1;
 
    // ── Second Derivative ─────────────────────────────────────────────────
    std::cout << "\n▶ SECOND DERIVATIVE  —  d²/dt² [ f(t) ]\n";
    printLine();
    gLog.clear();
    Node* d2 = nthDerivative(root, 2);
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  f''(t) = " << d2->toString() << "\n";
    delete d2;
 
    // ── Integration ───────────────────────────────────────────────────────
    std::cout << "\n▶ INTEGRATION  —  ∫ f(t) dt\n";
    printLine();
    gLog.clear();
    Node* rawI = root->integrate();
    Node* intg = rawI->simplify();
    delete rawI;
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  F(t) = " << intg->toString() << " + C\n";
    std::cout << "  LaTeX   :        " << intg->toLatex()  << "\n";
    delete intg;
 
    // ── Taylor / Maclaurin Series ─────────────────────────────────────────
    std::cout << "\n▶ MACLAURIN SERIES  (expanded around t=0, 6 terms)\n";
    printLine();
    gLog.clear();
    std::cout << "  " << taylorSeries(root, 0.0, 6) << "\n";
 
    // ── Laplace Transform ─────────────────────────────────────────────────
    std::cout << "\n▶ LAPLACE TRANSFORM  —  L{ f(t) } = F(s)\n";
    printLine();
    gLog.clear();
    Node* rawL  = root->laplace();
    Node* Fs    = rawL->simplify();
    delete rawL;
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  F(s) = " << Fs->toString() << "\n";
    std::cout << "  LaTeX   :        " << Fs->toLatex()  << "\n";
 
    // ── Initial & Final Value Theorems ────────────────────────────────────
    std::cout << "\n▶ VALUE THEOREMS\n";
    printLine();
    std::cout << "  " << initialValueTheorem(root) << "\n";
    std::cout << "  " << finalValueTheorem(Fs)     << "\n";
 
    // ── Inverse Laplace (round-trip check) ────────────────────────────────
    std::cout << "\n▶ INVERSE LAPLACE  —  L⁻¹{ F(s) }\n";
    printLine();
    gLog.clear();
    Node* inv = Fs->invLaplace();
    if (!gLog.empty()) { std::cout << "  Workings:\n"; gLog.print(); }
    std::cout << "  Result  :  L⁻¹{F(s)} = " << inv->toString() << "\n";
    delete inv;
    delete Fs;
 
    // ── Fourier Transform ─────────────────────────────────────────────────
    std::cout << "\n▶ FOURIER TRANSFORM  —  F{ f(t) } = F(ω)\n";
    printLine();
    gLog.clear();
    std::cout << "  " << fourierTransform(root) << "\n";
 
    printLine('=', 65);
    std::cout << "\n";
}
 
// ============================================================================
//  SECTION 12 — MAIN REPL
//  Interactive loop with history, recall, and steps toggle.
//
//  Commands (type at the prompt):
//    history      — list all previous inputs
//    recall N     — re-run input number N from history
//    steps on     — show step-by-step workings  (default)
//    steps off    — hide step-by-step workings
//    exit / quit  — close the program
// ============================================================================
 
int main() {
    std::vector<std::string> history;
    std::string input;
 
    std::cout << "\n";
    printLine('*', 65);
    std::cout << "  MULTI-DOMAIN MATH ENGINE — Extended Edition\n\n";
    std::cout << "  Operators : + - * / ^ (implicit * also works: 2t, 3sin)\n";
    std::cout << "  Functions : sin cos tan asin acos atan ln sqrt abs exp\n";
    std::cout << "  Variables : t  (time)   e  (Euler)   pi  (3.14159...)\n\n";
    std::cout << "  Examples  : t^3 - 2t + 1\n";
    std::cout << "              e^(-2*t)*cos(3t)\n";
    std::cout << "              t*sin(2t) + ln(t)\n\n";
    std::cout << "  Commands  : history | recall N | steps on/off | exit\n";
    printLine('*', 65);
 
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
        if (input == "history") {
            if (history.empty()) { std::cout << "  No history yet.\n"; continue; }
            for (size_t i = 0; i < history.size(); ++i)
                std::cout << "  [" << (i + 1) << "] " << history[i] << "\n";
            continue;
        }
        if (input.rfind("recall ", 0) == 0) {
            try {
                int n = std::stoi(input.substr(7));
                if (n < 1 || n > (int)history.size())
                    { std::cout << "  Index out of range.\n"; continue; }
                input = history[n - 1];
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
            Node*  root   = parser.parse();
 
            history.push_back(input);
            printResults(root, input);
            delete root;
        }
        catch (const std::exception& ex) {
            std::cout << "  ! Error: " << ex.what() << "\n";
        }
    }
 
    return 0;
}
