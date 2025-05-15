#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <vector>
#include <string>

using namespace std;

enum class TokenType { IDENTIFIER, ASSIGN, IF, ELSE, WHILE, RETURN, NEWLINE, END_OF_FILE};





void parse_program();
void parse_statement();
void parse_assignment();
void parse_return_stmt();
void parse_if_stmt();
void parse_else_part();
void parse_while_stmt();
void parse_func_call();
void parse_argument_list();
void parse_argument_list_prime();
void parse_statement_list();
void parse_expression();
void parse_bool_expr_prime();
void parse_bool_term();
void parse_bool_term_prime();
void parse_bool_factor();
void parse_rel_expr();
void parse_rel_op();
void parse_arith_expr();
void parse_arith_expr_prime();
void parse_term();
void parse_term_prime();
void parse_factor();
void parse_augmented_assignment();
void parse_for_stmt();
void parse_list_items_prime();
void parse_list_literal();


void next_token();
bool match(string expectedType);

Token peek();
void advance();


#endif