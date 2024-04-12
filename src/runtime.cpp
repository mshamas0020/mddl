// runtime.cpp

#include "expr.hpp"
#include "errors.hpp"
#include "runtime.hpp"



namespace MDDL {

DataRef Runtime::execute( const Scope* scope )
{
    attach_stack( scope );
    DataRef return_v( DataType::UNDEFINED );

    const ExprRoot* node = scope->head;
    while ( node != nullptr ) {
        // std::cout << "\nExpr: " << node->expr->to_string() << "\n";

        if ( node->is_branch() ) {
            const auto& [v, next] = process_root( node );
            node = next;
        } else {
            return_v.release();
            const auto& [v, next] = process_root( node );
            return_v = v;
            node = next;
        }

        /*
        // debug
        std::cout << "held: ";
        return_v.print();
        if ( !return_v.empty() )
            return_v.get().print();

        for ( int i = 0; i < (int )scope->vars.size(); i ++ ) {
            std::cout << "stack '" << symbol_to_str( scope->vars[i] ) << "': ";
            stack[stack_pos + i].print();
        }
        */
    }

    release_stack( scope );

    return return_v.cast_to_vseq();
}

void Runtime::attach_stack( const Scope* scope )
{
    // stack already has scope args initialized /after/ stack pos
    const int undefined_count = (int )(scope->vars.size() - scope->args.size());
    for ( int i = 0; i < undefined_count; i ++ )
        push_to_stack( DataRef( DataType::SEQ, new Sequence ) );
}

void Runtime::release_stack( const Scope* scope )
{
    const int stack_start = stack_pos;
    const int stack_end = stack_pos + (int )scope->vars.size();
    for ( int i = stack_start; i < stack_end; i ++ )
        stack[i].release();
    
    stack.resize( stack_pos );
}

void Runtime::push_to_stack( const DataRef& ref )
{
    const int64_t top = (int64_t )stack.size();
    stack.push_back( ref );
    stack.back().stack_pos = top;
}

std::pair<DataRef, ExprRoot*> Runtime::process_root( const ExprRoot* root )
{
    if ( root->is_branch() ) {
        BranchExpr* br_expr = dynamic_cast<BranchExpr*>( root->expr );
        if ( br_expr->child == nullptr )
            return { DataType::VOID, br_expr->branch_down };

        DataRef v = process_operation( br_expr->child );
        assert( v.type == DataType::VALUE );
        
        return { DataType::VOID, v.value > 0 ? br_expr->branch_up : br_expr->branch_down };
    }

    return { process_expr( root->expr ), root->next };
}

DataRef Runtime::process_expr( const Expr* expr )
{
    switch ( expr->expr_type ) {
        case ExprType::FUNCTION_CALL:
            return process_function_call( dynamic_cast<const FunctionCallExpr*>( expr ) );
        case ExprType::OPERATION:
            return process_operation( dynamic_cast<const OperationExpr*>( expr ) );
        case ExprType::VARIABLE:
            return process_variable( dynamic_cast<const VariableExpr*>( expr ) );
        case ExprType::VALUE_LITERAL:
            return process_value_literal( dynamic_cast<const ValueLiteralExpr*>( expr ) );
        case ExprType::SEQUENCE_LITERAL:
            return process_sequence_literal( dynamic_cast<const SequenceLiteralExpr*>( expr ) );
        default: enum_error(); break;
    }  
    
    return { DataType::ERROR };
}

DataRef Runtime::process_function_call( const FunctionCallExpr* fn_expr )
{
    const int curr_stack_pos = stack_pos;
    const int child_stack_pos = (int )stack.size();
    
    sys_assert( fn_expr->scope != nullptr );
    sys_assert( fn_expr->children.size() == fn_expr->scope->args.size() );

    for ( const Expr* child : fn_expr->children )
        push_to_stack( process_expr( child ).cast_to_seq() );

    stack_pos = child_stack_pos;

    const DataRef& v = execute( fn_expr->scope );

    stack_pos = curr_stack_pos;

    return v;
}

DataRef Runtime::process_operation( const OperationExpr* op_expr )
{
    DataRef lhs = process_expr( op_expr->child_lhs );
    DataRef rhs = (op_expr->child_rhs == nullptr) ? DataType::NONE : process_expr( op_expr->child_rhs );

    lhs.implicit_cast( op_expr->lhs_type );
    rhs.implicit_cast( op_expr->rhs_type );

    DataRef v = op_expr->fn( this, lhs, rhs );

    sys_assert( v.type == op_expr->return_type );
    // v.implicit_cast( op_expr->return_type );

    sys_assert( lhs.empty() );
    sys_assert( rhs.empty() );

    return v;
}

DataRef Runtime::process_variable( const VariableExpr* var_expr )
{
    return stack[stack_pos + var_expr->stack_offset].duplicate();
}

DataRef Runtime::process_value_literal( const ValueLiteralExpr* val_expr )
{
    return val_expr->value;
}

DataRef Runtime::process_sequence_literal( const SequenceLiteralExpr* seq_expr )
{
    return seq_expr->ref.duplicate();
}



} // namespace MDDL