// environment.cpp

#include "environment.hpp"
#include "errors.hpp"

#include <cassert>
#include <iostream>


namespace MDDL {

static Symbol make_scope_id( const Symbol& chord, int n_args )
{
    return chord + ":" + std::to_string( n_args );
}

static uint8_t detect_root_note( const Symbol& )
{
    return 0;
}

Scope::Scope( Scope* parent, const Symbol& chord, Stage stage )
    : parent    { parent }
    , chord     { chord }
    , root_note { detect_root_note( chord ) }
    , stage     { stage }
{}

Scope::~Scope()
{
    for ( Scope* child : children )
        delete child;
        
    delete head;
}

void Scope::complete_stage()
{
    switch ( stage ) {
        case Stage::SIGNATURE: complete_signature(); break;
        case Stage::BODY: complete_body(); break;
        default: enum_error(); break;
    }
}

SeqLit* Scope::slrx_pop()
{
    SeqLit* v = slrx_queue.front();
    slrx_queue.pop_front();
    return v;
}

void Scope::complete_signature()
{
    // grant full id
    id = make_scope_id( chord, (int )args.size() );
    vars = args;

    stage = Stage::BODY;
}

void Scope::complete_body()
{
    resolve_branch_links();

    stage = Stage::DEFINED;
}

bool Scope::add_ast( const AST::Node* ast )
{
    switch ( stage ) {
        case Stage::SIGNATURE: return add_to_signature( ast ); break;
        case Stage::BODY: return add_to_body( ast ); break;
        default: enum_error(); break;
    }

    return false;
}

bool Scope::add_to_signature( const AST::Node* ast )
{
    if ( ast->type == SyntaxType::VARIABLE ) {
        args.push_back( ast->id );
        return true;
    }

    return false;
    // else soft error, not a variable
}

bool Scope::add_to_body( const AST::Node* ast )
{
    // add to ExprRoot linked list
    if ( head == nullptr ) {
        head = new ExprRoot();
        tail = head;
    } else if ( tail->expr != nullptr ) {
        tail->next = new ExprRoot();
        tail = tail->next;
    }

    try {
        if ( ast->type == SyntaxType::BRANCH ) {
            tail->expr = (Expr* )build_branch( ast );
        } else if ( ast->type == SyntaxType::OPERATOR ) {
            tail->expr = (Expr* )build_operation( ast, true );
        } else {
            tail->expr = build_expr( nullptr, ast );
        }
    } catch ( ... ) {
        delete tail->expr;
        tail->expr = nullptr;
    }

    return tail->expr != nullptr;
}

Expr* Scope::build_expr( Expr* expr_parent, const AST::Node* ast )
{
    Expr* expr;
    switch ( ast->type ) {
        case SyntaxType::FUNCTION_CALL: expr = build_function_call( ast ); break;
        // case SyntaxType::BRANCH: expr = build_branch( ast ); break;// only happens at root
        case SyntaxType::OPERATOR: expr = build_operation( ast ); break;
        case SyntaxType::VARIABLE: expr = build_variable( ast ); break;
        case SyntaxType::VALUE_LITERAL: expr = build_value_literal( ast ); break;
        case SyntaxType::SEQUENCE_LITERAL: expr = build_sequence_literal( ast ); break;
        default: enum_error(); return nullptr;
    }

    if ( expr != nullptr )
        expr->parent = expr_parent;
    return expr;
}

FunctionCallExpr* Scope::build_function_call( const AST::Node* ast )
{
    FunctionCallExpr* fn_expr = new FunctionCallExpr();

    const AST::Node* child = ast->child;
    int n_args = 0;

    while ( child != nullptr ) {
        Expr* fn_child = build_expr( fn_expr, child );
        if ( fn_child == nullptr ) { delete fn_expr; return nullptr; }
        
        fn_expr->children.push_back( fn_child );

        n_args ++;
        child = child->sibling;
    }

    const Symbol fn_id = make_scope_id( ast->id, n_args );
    fn_expr->chord = ast->id;
    fn_expr->id = fn_id;
    fn_expr->scope = query_scope( fn_id );

    if ( fn_expr->scope == nullptr )
        unresolved_calls.push_back( fn_expr );

    return fn_expr;
}

BranchExpr* Scope::build_branch( const AST::Node* ast )
{
    BranchExpr* br_expr = new BranchExpr();

    br_expr->id = ast->id;
    if ( ast->child != nullptr ) {
        br_expr->child = build_operation( ast, false, BranchExpr::COMPARE_OP_ID );
        if ( br_expr->child == nullptr ) { delete br_expr; return nullptr; }

        sys_assert( br_expr->child->return_type == DataType::VALUE );
    }

    // it's quicker to traverse the complete body when the scope is fully defined
    // so we'll hold off on finding branch_up/down links for now
    return br_expr;
}

OperationExpr* Scope::build_operation( const AST::Node* ast, bool leftmost, OpId force_op )
{

    OperationExpr* op_expr = new OperationExpr();

    const bool force_copy = !leftmost;
    const uint8_t note = (uint8_t )ast->id[0];
    const OpId op_id = (force_op == OP_UNKNOWN) ? note_to_op_id( note, root_note ) : force_op;

    const AST::Node* lhs = ast->child;
    if ( lhs == nullptr ) { delete op_expr; return nullptr; }

    const AST::Node* rhs = lhs->sibling;

    op_expr->note = note;
    op_expr->group = op_id;

    if ( leftmost && lhs->type == SyntaxType::OPERATOR ) {
        // preserve leftmost
        op_expr->child_lhs = (Expr* )build_operation( lhs, true );
        if ( op_expr->child_lhs == nullptr ) { delete op_expr; return nullptr; }

        op_expr->child_lhs->parent = op_expr;
    } else {
        op_expr->child_lhs = build_expr( op_expr, lhs );
        if ( op_expr->child_lhs == nullptr ) { delete op_expr; return nullptr; }
    }

    op_expr->lhs_type = op_expr->child_lhs->return_type;

    if ( rhs == nullptr ) {
        // unary operation
        op_expr->rhs_type = DataType::NONE;
        op_expr->query_book( force_copy );
        sys_assert( op_expr->child_rhs == nullptr );
        return op_expr;
    }

    op_expr->child_rhs = build_expr( op_expr, rhs );
    if ( op_expr->child_rhs == nullptr ) { delete op_expr; return nullptr; }

    op_expr->rhs_type = op_expr->child_rhs->return_type;
    op_expr->query_book( force_copy );

    rhs = rhs->sibling;
    while ( rhs != nullptr ) {
        OperationExpr* new_expr = new OperationExpr();
        new_expr->note = note;
        new_expr->group = op_id;
        new_expr->child_lhs = op_expr;
        new_expr->child_rhs = build_expr( new_expr, rhs );
        if ( new_expr->child_rhs == nullptr ) { delete new_expr; return nullptr; }

        new_expr->lhs_type = new_expr->child_lhs->return_type;
        new_expr->rhs_type = new_expr->child_rhs->return_type;
        new_expr->query_book( force_copy );

        op_expr->parent = new_expr;
        op_expr = new_expr;

        rhs = rhs->sibling;
    }

    return op_expr;
}

VariableExpr* Scope::build_variable( const AST::Node* ast )
{
    VariableExpr* var_expr = new VariableExpr();
    var_expr->id = ast->id;
    int stack_offset = -1;

    for ( int i = 0; i < (int )vars.size(); i++ ) {
        if ( var_expr->id == vars[i] ) {
            stack_offset = i;
            break;
        }
    }

    if ( stack_offset == -1 ) {
        stack_offset = (int )vars.size();
        vars.push_back( var_expr->id );
    }

    var_expr->stack_offset = stack_offset;

    sys_assert( !ast->has_child() );

    return var_expr;
}

ValueLiteralExpr* Scope::build_value_literal( const AST::Node* ast )
{
    ValueLiteralExpr* val_expr = new ValueLiteralExpr();

    const Symbol& sym = ast->id;

    int64_t value = 0;

    if ( sym.size() > 0 ) {
        for ( int i = 1; i < (int )sym.size(); i ++ ) {
            value *= 10;
            value += std::abs( (int )sym[i] ) % 10;
        }

        if ( sym[1] < 0 )
            value = - value;
    }

    val_expr->value = value;

    return val_expr;
}

SequenceLiteralExpr* Scope::build_sequence_literal( const AST::Node* ast )
{
    SequenceLiteralExpr* seq_expr = new SequenceLiteralExpr();

    const Symbol& id = ast->id;
    seq_expr->id = id;
    seq_expr->note = ast->note_start;

    for ( const SequenceLiteralExpr* other : slrx_queue ) {
        if ( other->id == id ) {
            seq_expr->ref = other->ref.duplicate();
            return seq_expr;
        }
    }

    Sequence* seq = new Sequence;
    seq->complete = false;
    seq_expr->ref = DataRef( DataType::SEQ_LIT, seq );

    slrx_queue.push_back( seq_expr );
    return seq_expr;
}

Scope* Scope::query_scope( const Symbol& query )
{
    // search up the scope tree
    Scope* scope = this;
    while ( scope != nullptr ) {
        for ( Scope* child : scope->children )
            if ( child->id == query )
                return child;

        scope = scope->parent;
    }

    return nullptr;
}

void Scope::resolve_branch_links()
{
    ExprRoot* node = head;

    while ( node != nullptr ) {
        if ( !node->is_branch() ) {
            node = node->next;
            continue;
        }

        BranchExpr* br_expr = dynamic_cast<BranchExpr*>( node->expr );
        const Symbol& br_id = br_expr->id;

        br_expr->branch_down = node->next;

        ExprRoot* node_down = node->next;
        while ( node_down != nullptr ) {
            if ( !node_down->is_branch() ) {
                node_down = node_down->next;
                continue;
            }
            
            BranchExpr* br_down = dynamic_cast<BranchExpr*>( node_down->expr );
            if ( br_down->id == br_id ) {
                br_down->branch_up = node->next;
                br_expr->branch_down = node_down->next;
                break;
            }

            node_down = node_down->next;
        }
        
        if ( br_expr->branch_up == nullptr )
            br_expr->branch_up = node->next;

        node = node->next;
    }
}

void Scope::resolve_function_links()
{
    auto itr = unresolved_calls.begin();
    while ( itr != unresolved_calls.end() ) {
        FunctionCallExpr* fn_expr = *itr;
        fn_expr->scope = query_scope( fn_expr->id );

        if ( fn_expr->scope != nullptr ) {
            itr = unresolved_calls.erase( itr );
            continue;
        }

        itr ++;
    }

    for ( Scope* child : children )
        child->resolve_function_links();
}

void Scope::print() const
{
    std::cout << "\nFN " << symbol_to_str( chord ) << "( ";
    for ( const Symbol& arg : args ) {
        std::cout << symbol_to_str( arg );
        if ( arg != args.back() )
            std::cout << ", ";
    }
    std::cout << " ):\n";

    ExprRoot* node = head;
    
    while ( node != nullptr ) {
        std::cout << "    " << expr_to_string( node->expr ) << "\n";
        node = node->next;
    }

    for ( Scope* child : children )
        child->print();
}





StaticEnvironment::StaticEnvironment()
    : global( new Scope( nullptr, ":global", Scope::Stage::BODY ) )
    , tail  { global }
{}

bool StaticEnvironment::add_ast( const AST& ast )
{
    if ( ast.error )
        return false;

    const AST::Node* node = ast.head;
    tail->ief_code = ast.ief_code;

    if ( node->type == SyntaxType::FUNCTION_DEF ) {
        process_function_def( node->id );
        return false;
    }

    return tail->add_ast( node );
}

void StaticEnvironment::resolve_links()
{
    global->resolve_branch_links();
    global->resolve_function_links();
}

void StaticEnvironment::print() const
{
    std::cout << "\nGLOBAL\n";
    std::cout << "--------\n";

    ExprRoot* node = global->head;
    
    while ( node != nullptr ) {
        std::cout << "    " << node->expr->to_string() << "\n";
        node = node->next;
    }

    for ( Scope* child : global->children )
        child->print();

    std::cout << "--------\n";
}

void StaticEnvironment::process_function_def( const Symbol& chord )
{
    if ( tail->chord == chord ) {
        tail->complete_stage();

        if ( tail->stage == Scope::Stage::DEFINED ) {
            // legitimize child
            tail->parent->add_child_scope( tail );
            tail = tail->parent;
        }
    } else {
        // tail->id != id
        tail = new Scope( tail, chord, Scope::Stage::SIGNATURE );
    }
}


} // namespace MDDL