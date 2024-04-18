// expr.hpp

#ifndef __MDDL_EXPR_HPP__
#define __MDDL_EXPR_HPP__

#include "data_ref.hpp"
#include "operations.hpp"
#include "utils.hpp"

#include <ostream>

namespace MDDL {

enum class ExprType {
    UNKNOWN,
    OPERATION,
    BRANCH,
    FUNCTION_CALL,
    VARIABLE,
    VALUE_LITERAL,
    SEQUENCE_LITERAL,
    ERROR
};

class Scope;
class ExprRoot;

class Expr
{
public:
    Expr( ExprType type, DataType return_type, bool error = false );
    virtual ~Expr() = default;

    virtual std::string to_string() const = 0;

    const ExprType  expr_type;
    DataType        return_type = DataType::UNKNOWN;
    Expr*           parent      = nullptr;
    bool            error       = false;
};

inline std::string expr_to_string( const Expr* expr )
{
    if ( expr == nullptr )
        return "NULL";
    return expr->to_string();
}

class ExprRoot 
{
public:
    ExprRoot() = default;
    ~ExprRoot() { delete next; delete expr; }
    bool is_branch() const { return (expr->expr_type == ExprType::BRANCH); }

    ExprRoot*   next    = nullptr;
    Expr*       expr    = nullptr;
};

class FunctionCallExpr : public Expr
{
public:
    FunctionCallExpr();
    ~FunctionCallExpr();

    std::string to_string() const override;

    Symbol              chord       = "";
    Symbol              id          = "";
    std::vector<Expr*>  children    = {};
    Scope*              scope       = nullptr;
};

class OperationExpr : public Expr
{
public:
    OperationExpr();
    ~OperationExpr();

    std::string to_string() const override;
    std::string operands_to_string() const;
    void query_book( bool force_copy );
    void from_book( const OpBookKey& key, const OpBookEntry& entry );

    Expr*           child_lhs   = nullptr;
    Expr*           child_rhs   = nullptr;
    DataType        lhs_type    = DataType::UNKNOWN;
    DataType        rhs_type    = DataType::UNKNOWN;
    uint8_t         note        = 0;
    OpId            group       = OP_UNKNOWN;
    OpFn            fn          = nullptr;
    const char*     name        = "UNKNOWN";
};

class BranchExpr : public Expr
{
public:
    static const OpId COMPARE_OP_ID = OP_MI;

    BranchExpr();
    ~BranchExpr();

    std::string to_string() const override;

    Symbol          id          = "";
    OperationExpr*  child       = nullptr;
    ExprRoot*       branch_up   = nullptr;
    ExprRoot*       branch_down = nullptr;
};

class VariableExpr : public Expr
{
public:
    VariableExpr();
    ~VariableExpr() = default;

    std::string to_string() const override;

    Symbol  id              = "";
    int     stack_offset    = 0;
};

class ValueLiteralExpr : public Expr
{
public:
    ValueLiteralExpr();
    ~ValueLiteralExpr() = default;

    std::string to_string() const override;

    int64_t value = 0;
};

class SequenceLiteralExpr : public Expr
{
public:
    SequenceLiteralExpr();
    ~SequenceLiteralExpr();

    std::string to_string() const override;

    Symbol  id      = "";
    DataRef ref     = DataType::SEQ_LIT;
    uint8_t note    = 0;
};

using SeqLit = SequenceLiteralExpr;

class ErrorExpr : public Expr
{
public:
    ErrorExpr();
    ~ErrorExpr() = default;

    std::string to_string() const override;
};

} // namepspace MDDL

#endif // __MDDL_EXPR_HPP__