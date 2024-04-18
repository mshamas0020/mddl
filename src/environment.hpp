// environment.hpp

#ifndef __MDDL_ENVIRONMENT_HPP__
#define __MDDL_ENVIRONMENT_HPP__

#include "expr.hpp"
#include "syntax.hpp"
#include "utils.hpp"

#include <cstdint>
#include <list>
#include <vector>


namespace MDDL {

namespace MIDI = libremidi;

class Scope
{
public:
    enum class Stage
    {
        SIGNATURE, BODY, DEFINED
    };

    Scope( Scope* parent, const Symbol& chord, Stage stage );
    ~Scope();

    void complete_stage();
    void complete_signature();
    void complete_body();
    
    bool slrx_pending() const { return slrx_queue.size() > 0; }
    SeqLit* slrx_pop();

    void add_child_scope( Scope* child ) { children.push_back( child ); };

    bool add_ast( const AST::Node* ast );
    Expr* build_expr( Expr* expr_parent, const AST::Node* ast, bool leftmost = false );
    Expr* build_expr_root( const AST::Node* ast );

    Scope* query_scope( const Symbol& id );

    void resolve_branch_links();
    void resolve_function_links();

    void print() const;

private:
    bool add_to_signature( const AST::Node* ast );
    bool add_to_body( const AST::Node* ast );

    FunctionCallExpr* build_function_call( const AST::Node* ast );
    BranchExpr* build_branch( const AST::Node* ast );
    OperationExpr* build_operation( const AST::Node* ast, bool leftmost = false, OpId force_op = OP_UNKNOWN );
    VariableExpr* build_variable( const AST::Node* ast );
    ValueLiteralExpr* build_value_literal( const AST::Node* ast );
    SequenceLiteralExpr* build_sequence_literal( const AST::Node* ast );

public:
    Scope*                  parent              = nullptr;
    Symbol                  chord               = "";
    Symbol                  id                  = "";
    uint8_t                 root_note           = 0;
    Stage                   stage               = Stage::SIGNATURE;
    std::vector<Symbol>     args                = {};  
    std::vector<Symbol>     vars                = {};
    ExprRoot*               head                = nullptr;
    ExprRoot*               tail                = nullptr;
    std::vector<Scope*>     children            = {};
    std::list<FunctionCallExpr*>
                            unresolved_calls    = {};
    std::list<SeqLit*>      slrx_queue          = {};
    OpId                    ief_code            = IEF_DEFAULT;
    bool                    error               = false;
};

class StaticEnvironment
{
public:
    StaticEnvironment();
    ~StaticEnvironment() { delete global; }

    bool add_ast( const AST& ast );
    bool slrx_pending() const { return tail->slrx_pending(); }
    SeqLit* slrx_pop() const { return tail->slrx_pop(); }

    bool at_global_scope() { return tail == global; }
    ExprRoot* get_global_tail() { return global->tail; }

    void resolve_links();

    void print() const;

    Scope* global   = nullptr;
    Scope* tail     = nullptr;

private:
    void process_function_def( const Symbol& id );
    
};

} // namespace MDDL

#endif // __MDDL_ENVIRONMENT_HPP__