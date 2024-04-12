// printer.hpp

#include "syntax.hpp"
#include "environment.hpp"

#include <iostream>

#ifndef __MDDL_PRINTER_HPP__
#define __MDDL_PRINTER_HPP__



namespace MDDL {

class Printer
{
public:
    void print( const CST& cst, Scope* scope )
    {
        try {
            AST ast;
            ast.build_from_cst( cst );
            const AST::Node* node = ast.head;

            if ( node == nullptr )
                return;

            if ( node->type == SyntaxType::FUNCTION_DEF ) {
                //process_function_def( node->id );
                return;
            }

            Expr* expr = scope->build_expr( nullptr, node );

            if ( expr != nullptr && expr->expr_type != ExprType::VALUE_LITERAL ) {
                const std::string& str = expr->to_string();
                std::cout << str;
                const int diff = prev_length - (int )str.size();
                if ( diff > 0 )
                    std::cout << std::string( diff, ' ' );
                prev_length = (int )str.size();
            }
            delete expr;

            ast.reset();
        } catch ( ... ) {
            
        }
    }

    int prev_length = 0;
};

} // namespace MDDL

#endif // __MDDL_PRINTER_HPP__