#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <vector>
#include <string>
#include <memory>

using namespace std;

enum class TokenType {     IDENTIFIER, KEYWORD, OPERATOR, DELIMITER, 
    NUMBER, STRING_QUOTE, STRING_LITERAL,
    ASSIGN, IF, ELSE, WHILE, FOR, RETURN, 
    NEWLINE, INDENT, DEDENT, END_OF_FILE};

struct ParseTreeNode {
    std::string name;
    std::string value;
    std::vector<std::shared_ptr<ParseTreeNode>> children;
    
    ParseTreeNode(const std::string& n, const std::string& v = "") 
        : name(n), value(v) {}
    
    void addChild(std::shared_ptr<ParseTreeNode> child) {
        children.push_back(child);
    }
};

void report_error(const string& message);
void synchronize();
std::string escapeDotString(const std::string& input);
shared_ptr<ParseTreeNode> parse_program();
shared_ptr<ParseTreeNode> parse_statement();
shared_ptr<ParseTreeNode> parse_assignment();
shared_ptr<ParseTreeNode> parse_return_stmt();
shared_ptr<ParseTreeNode> parse_if_stmt();
shared_ptr<ParseTreeNode> parse_elif_stmt();
shared_ptr<ParseTreeNode> parse_else_part();
shared_ptr<ParseTreeNode> parse_while_stmt();
shared_ptr<ParseTreeNode> parse_func_call();
shared_ptr<ParseTreeNode> parse_argument_list();
shared_ptr<ParseTreeNode> parse_argument_list_prime();
shared_ptr<ParseTreeNode> parse_statement_list();
shared_ptr<ParseTreeNode> parse_expression();
shared_ptr<ParseTreeNode> parse_bool_expr_prime();
shared_ptr<ParseTreeNode> parse_bool_term();
shared_ptr<ParseTreeNode> parse_bool_term_prime();
shared_ptr<ParseTreeNode> parse_bool_factor();
shared_ptr<ParseTreeNode> parse_rel_expr();
shared_ptr<ParseTreeNode> parse_rel_op();
shared_ptr<ParseTreeNode> parse_arith_expr();
shared_ptr<ParseTreeNode> parse_arith_expr_prime();
shared_ptr<ParseTreeNode> parse_term();
shared_ptr<ParseTreeNode> parse_term_prime();
shared_ptr<ParseTreeNode> parse_factor();
shared_ptr<ParseTreeNode> parse_augmented_assignment();
shared_ptr<ParseTreeNode> parse_for_stmt();
shared_ptr<ParseTreeNode> parse_list_items_prime();
shared_ptr<ParseTreeNode> parse_list_literal();
shared_ptr<ParseTreeNode> parse_func_def();
shared_ptr<ParseTreeNode> parse_param_list();
shared_ptr<ParseTreeNode> parse_param();
shared_ptr<ParseTreeNode> parse_type();
shared_ptr<ParseTreeNode> parse_import_stmt();
shared_ptr<ParseTreeNode> parse_import_tail();
shared_ptr<ParseTreeNode> parse_import_item();
shared_ptr<ParseTreeNode> parse_import_alias_opt();
shared_ptr<ParseTreeNode> parse_dict_literal();
shared_ptr<ParseTreeNode> parse_dict_items_prime();
shared_ptr<ParseTreeNode> parse_dict_pair();
shared_ptr<ParseTreeNode> parse_loop_statement_list();
shared_ptr<ParseTreeNode> parse_loop_statement();
shared_ptr<ParseTreeNode> parse_or_test();
shared_ptr<ParseTreeNode> parse_inline_if_else();
shared_ptr<ParseTreeNode> parse_string_key();
shared_ptr<ParseTreeNode> parse_class_def();
shared_ptr<ParseTreeNode> parse_class_inheritance_opt();
shared_ptr<ParseTreeNode> parse_try_stmt();
shared_ptr<ParseTreeNode> parse_except_clauses();
shared_ptr<ParseTreeNode> parse_except_clause();
shared_ptr<ParseTreeNode> parse_finally_clause();
shared_ptr<ParseTreeNode> parse_del_stmt();
shared_ptr<ParseTreeNode> parse_del_target();
shared_ptr<ParseTreeNode> parse_assign_target();
shared_ptr<ParseTreeNode> parse_primary_target();
shared_ptr<ParseTreeNode> parse_assign_target_tail();
shared_ptr<ParseTreeNode> parse_continue_stmt();
shared_ptr<ParseTreeNode> parse_break_stmt();


void next_token();
bool match(string expectedType);

Token peek();
void advance();


#endif
