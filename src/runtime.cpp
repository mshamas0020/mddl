// runtime.cpp

#include "expr.hpp"
#include "errors.hpp"
#include "runtime.hpp"



namespace MDDL {

DataRef Runtime::execute( const ExprRoot* node )
{
    DataRef return_v( DataType::UNDEFINED );
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

    return return_v;
}

DataRef Runtime::execute_scope( const Scope* scope )
{
    push_scope( scope );

    const ExprRoot* node = scope->head;
    const DataRef v = execute( node ).cast_to_vseq();

    pop_scope( scope );
    return v;
}

void Runtime::push_scope( const Scope* scope )
{
    // stack already has scope args initialized /after/ stack pos
    const int stack_target = stack_pos + (int )scope->vars.size();
    while ( (int )stack.size() < stack_target )
        push_to_stack( DataRef( DataType::SEQ, new Sequence ) );
}

void Runtime::pop_scope( const Scope* scope )
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
        if ( br_expr->child == nullptr ) {
            //std::cout << "TRACE " << root->expr->to_string() << "\n";
            //std::cout << "Branch down\n";
            return { DataType::VOID, br_expr->branch_down };
        }

        DataRef v = process_operation( br_expr->child );
        assert( v.type == DataType::VALUE );
        
        //std::cout << "TRACE " << root->expr->to_string() << "\n";
        //std::cout << (v.value > 0 ? "Branch up\n" : "Branch down\n");
        return { DataType::VOID, v.value > 0 ? br_expr->branch_up : br_expr->branch_down };
    }

    return { process_expr( root->expr ), root->next };
}

DataRef Runtime::process_expr( const Expr* expr )
{
    DataRef v;
    // TODO: move this into a virtual member fn of expr
    switch ( expr->expr_type ) {
        case ExprType::FUNCTION_CALL:
            v = process_function_call( dynamic_cast<const FunctionCallExpr*>( expr ) );
            break;
        case ExprType::OPERATION:
            v = process_operation( dynamic_cast<const OperationExpr*>( expr ) );
            break;
        case ExprType::VARIABLE:
            v = process_variable( dynamic_cast<const VariableExpr*>( expr ) );
            break;
        case ExprType::VALUE_LITERAL:
            v = process_value_literal( dynamic_cast<const ValueLiteralExpr*>( expr ) );
            break;
        case ExprType::SEQUENCE_LITERAL:
            v = process_sequence_literal( dynamic_cast<const SequenceLiteralExpr*>( expr ) );
            break;
        default:
            v = DataType::ERROR;
            break;
    }  
    
    //std::cout << "TRACE " << expr->to_string() << "\n";
    //v.print();
    //if ( !v.empty() && v.length() <= 13 )
    //    v.get().print();
    return v;
}

DataRef Runtime::process_function_call( const FunctionCallExpr* fn_expr )
{
    //std::cout << "SCOPE " << fn_expr->to_string() << "\n";
    const int curr_stack_pos = stack_pos;
    const int child_stack_pos = (int )stack.size();
    
    rt_assert( fn_expr->scope != nullptr, "Function definition for " + fn_expr->to_string() + " not found." );
    sys_assert( fn_expr->children.size() == fn_expr->scope->args.size() );

    for ( const Expr* child : fn_expr->children )
        push_to_stack( process_expr( child ).cast_to_seq() );

    stack_pos = child_stack_pos;
    const DataRef& v = execute_scope( fn_expr->scope );
    stack_pos = curr_stack_pos;

    return v;
}

DataRef Runtime::process_operation( const OperationExpr* op_expr )
{
    DataRef lhs = process_expr( op_expr->child_lhs );
    DataRef rhs = (op_expr->child_rhs == nullptr) ? DataType::NONE : process_expr( op_expr->child_rhs );

    lhs.implicit_cast( op_expr->lhs_type );
    rhs.implicit_cast( op_expr->rhs_type );

    std::mutex lhs_dummy, rhs_dummy;
    std::lock_guard<std::mutex> lhs_guard( lhs.empty() || lhs.get().ref_count == 1 ? lhs_dummy : lhs.mtx() );
    std::lock_guard<std::mutex> rhs_guard( rhs.empty() || rhs.get().ref_count == 1 ? rhs_dummy : rhs.mtx() );

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