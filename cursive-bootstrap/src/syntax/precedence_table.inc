// Precedence levels for binary operators (higher = binds tighter)
// Used by Pratt parsing in ParseBinaryExpr
enum class BinaryPrecedence : int {
  NONE = 0,
  LOGICAL_OR = 1,   // ||
  LOGICAL_AND = 2,  // &&
  COMPARISON = 3,   // ==, !=, <, <=, >, >=
  BIT_OR = 4,       // |
  BIT_XOR = 5,      // ^
  BIT_AND = 6,      // &
  SHIFT = 7,        // <<, >>
  ADD = 8,          // +, -
  MUL = 9,          // *, /, %
  // Power (**), Cast (as), Unary, Postfix handled separately
};

// Returns the precedence of a binary operator, or NONE if not a binary op
inline BinaryPrecedence GetBinaryPrecedence(std::string_view op) {
  if (op == "||") return BinaryPrecedence::LOGICAL_OR;
  if (op == "&&") return BinaryPrecedence::LOGICAL_AND;
  if (op == "==" || op == "!=" || op == "<" ||
      op == "<=" || op == ">" || op == ">=") {
    return BinaryPrecedence::COMPARISON;
  }
  if (op == "|") return BinaryPrecedence::BIT_OR;
  if (op == "^") return BinaryPrecedence::BIT_XOR;
  if (op == "&") return BinaryPrecedence::BIT_AND;
  if (op == "<<" || op == ">>") return BinaryPrecedence::SHIFT;
  if (op == "+" || op == "-") return BinaryPrecedence::ADD;
  if (op == "*" || op == "/" || op == "%") return BinaryPrecedence::MUL;
  return BinaryPrecedence::NONE;
}
