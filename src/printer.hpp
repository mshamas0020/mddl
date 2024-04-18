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
    void print( const CST& cst )
    {
        Scope tmp( nullptr, "", Scope::Stage::BODY );

        try {
            AST ast;
            ast.build_from_cst( cst );
            const AST::Node* node = ast.head;

            if ( node == nullptr )
                return;

            if ( node->type == SyntaxType::FUNCTION_DEF ) {
                print_line( "DEF " + symbol_to_str( node->id ) );
                return;
            }

            Expr* expr = tmp.build_expr_root( node );

            if ( expr != nullptr && expr->expr_type != ExprType::VALUE_LITERAL )
                print_line( expr->to_string() );
                
            delete expr;

            ast.reset();
        } catch ( ... ) {
            
        }
    }

    void print_line( const std::string& str )
    {
        std::cout << str;
        const int diff = prev_length - (int )str.size();
        if ( diff > 0 )
            std::cout << std::string( diff, ' ' );
        prev_length = (int )str.size();
    }

    int prev_length = 0;
};

} // namespace MDDL

#endif // __MDDL_PRINTER_HPP__