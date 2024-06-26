// environment.hpp

#ifndef __MDDL_RUNTIME_HPP__
#define __MDDL_RUNTIME_HPP__

#include "environment.hpp"
#include "data_ref.hpp"
#include "scheduler.hpp"

#include <utility>
#include <vector>

namespace MDDL {

class Runtime
{
public:
    Runtime( Scheduler* scheduler )
        : scheduler { scheduler }
    {}

    DataRef execute( const ExprRoot* node );
    DataRef execute_scope( const Scope* scope );

    void push_scope( const Scope* scope );
    void pop_scope( const Scope* scope );
    void push_to_stack( const DataRef& ref );

    std::pair<DataRef, ExprRoot*> process_root( const ExprRoot* root );
    DataRef process_expr( const Expr* expr );
    DataRef process_function_call( const FunctionCallExpr* fn_expr );
    DataRef process_operation( const OperationExpr* op_expr );
    DataRef process_variable( const VariableExpr* var_expr );
    DataRef process_value_literal( const ValueLiteralExpr* val_expr );
    DataRef process_sequence_literal( const SequenceLiteralExpr* seq_expr );

    Scheduler* scheduler    = nullptr;
    std::vector<DataRef> stack;
    int stack_pos = 0;
};

} // namespace MDDL

#endif // __MDDL_RUNTIME_HPP__