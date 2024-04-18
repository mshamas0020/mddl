// expr.cpp

#include "expr.hpp"

namespace MDDL {

Expr::Expr( ExprType expr_type, DataType return_type, bool error )
    : expr_type     { expr_type }
    , return_type   { return_type }
    , error         { error }
{}

FunctionCallExpr::FunctionCallExpr()
    : Expr( ExprType::FUNCTION_CALL, DataType::VSEQ )
{}

FunctionCallExpr::~FunctionCallExpr()
{
    for ( Expr* child : children )
        delete child;
}

std::string FunctionCallExpr::to_string() const
{
    std::string str = "FN " + symbol_to_str( chord ) + "( ";

    for ( int i = 0; i < (int )children.size(); i ++ ) {
        str += expr_to_string( children[i] );
        if ( i != (int )children.size() - 1 )
            str += ", ";
    }

    str += " )";
    return str;
}


OperationExpr::OperationExpr()
    : Expr( ExprType::OPERATION, DataType::UNKNOWN )
{}

OperationExpr::~OperationExpr()
{
    delete child_lhs;
    delete child_rhs;
}

std::string OperationExpr::to_string() const
{
    return std::string( name )
        // + "_" + dt_to_string( lhs_type ) + "_" + dt_to_string( rhs_type )
        + "( " + operands_to_string() + " )";
}

std::string OperationExpr::operands_to_string() const
{
    std::string lhs_str = "";
    const Expr* lhs = child_lhs;
    while ( lhs->expr_type == ExprType::OPERATION ) {
        const OperationExpr* op = dynamic_cast<const OperationExpr*>( lhs );
        if ( op->note != note )
            break;

        lhs_str = ", " + expr_to_string( op->child_rhs ) + lhs_str;
        lhs = op->child_lhs;
    }

    lhs_str = expr_to_string( lhs ) + lhs_str;

    const std::string rhs_str = (child_rhs == nullptr)
        ? "" : ", " + expr_to_string( child_rhs );

    return lhs_str + rhs_str;
}

void OperationExpr::query_book( bool force_copy )
{
    if ( force_copy ) {
        lhs_type = to_copy_type( lhs_type );
        rhs_type = to_copy_type( rhs_type );
    }

    OpBookKey key( group, lhs_type, rhs_type );
    
    if ( op_book.contains( key ) ) {
        from_book( key, op_book[key] );
        return;
    }

    while ( has_implicit_cast( key.rhs_t ) ) {
        key.rhs_t = implicit_cast( key.rhs_t  );

        if ( op_book.contains( key ) ) {
            from_book( key, op_book[key] );
            return;
        }
    }

    // key.rhs_t = rhs_type; // return to original type?

    while ( has_implicit_cast( key.lhs_t ) ) {
        key.lhs_t = implicit_cast( key.lhs_t  );

        if ( op_book.contains( key ) ) {
            from_book( key, op_book[key] );
            return;
        }
    }

    error = true;
}

void OperationExpr::from_book( const OpBookKey& key, const OpBookEntry& entry )
{
    lhs_type = key.lhs_t;
    rhs_type = key.rhs_t;
    name = entry.name;
    fn = entry.fn;
    return_type = entry.return_t;
}



BranchExpr::BranchExpr()
    : Expr( ExprType::BRANCH, DataType::VOID )
{}

BranchExpr::~BranchExpr()
{
    delete child;
}

std::string BranchExpr::to_string() const
{
    return "BR " + symbol_to_str( id ) + "(" +
        ((child == nullptr) ? "" : " " + child->operands_to_string() + " ")
        + ")";
}



VariableExpr::VariableExpr()
    : Expr( ExprType::VARIABLE, DataType::SEQ )
{}

std::string VariableExpr::to_string() const
{
    return symbol_to_str( id );
}



ValueLiteralExpr::ValueLiteralExpr()
    : Expr( ExprType::VALUE_LITERAL, DataType::VALUE )
{}

std::string ValueLiteralExpr::to_string() const
{
    return std::to_string( value );
}



SequenceLiteralExpr::SequenceLiteralExpr()
    : Expr( ExprType::SEQUENCE_LITERAL, DataType::SEQ_LIT )
{}

SequenceLiteralExpr::~SequenceLiteralExpr()
{
    ref.release();
}

std::string SequenceLiteralExpr::to_string() const
{
    return "[" + std::string( symbol_to_str( id ) ) + "]";
}



ErrorExpr::ErrorExpr()
    : Expr( ExprType::ERROR, DataType::ERROR, true )
{}

std::string ErrorExpr::to_string() const
{
    return "Error";
}

} // namepspace MDDL