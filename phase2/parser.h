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
void parse_elif_stmt();
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
void parse_func_def();
void parse_param_list();
void parse_param();
void parse_type();
void parse_import_stmt();
void parse_import_tail();
void parse_import_item();
void parse_import_alias_opt();
void parse_dict_literal();
void parse_dict_items_prime();
void parse_dict_pair();
void parse_loop_statement_list();
void parse_loop_statement();
void parse_or_test();
void parse_inline_if_else();
void parse_string_key();
void parse_class_def();
void parse_class_inheritance_opt();
void parse_try_stmt();
void parse_except_clauses();
void parse_except_clause();
void parse_finally_clause();
void parse_del_stmt();
void parse_del_target();
void parse_assign_target();
void parse_primary_target();
void parse_assign_target_tail();
void parse_continue_stmt();
void parse_break_stmt();


void next_token();
bool match(string expectedType);

Token peek();
void advance();


#endif
