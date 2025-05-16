#include "lexical_analyzer.h"
#include "parser_tree.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <iomanip>
#include <algorithm>
#include <memory>

using namespace std;

vector<Token> tokens;
Token currentToken;
int tokenIndex = 0;
shared_ptr<ParseTreeNode> parseTreeRoot;

bool error_recovery = false;
vector<string> error_messages;

void report_error(const string& message) {
    stringstream ss;
    ss << "Line " << currentToken.line << ": " << message;
    error_messages.push_back(ss.str());
}

void synchronize() {
    error_recovery = true;
    int startIdx = tokenIndex;
    // Skip tokens until we reach a likely statement boundary
    while (tokenIndex < tokens.size()) {
        string ttype = peek().type;
        if (ttype == "KEYWORD" || ttype == "IDENTIFIER" || ttype == "DEDENT" || ttype == "NEWLINE" || ttype == "END_OF_FILE") {
            break;
        }
        advance();
    }
    // Optionally, skip the NEWLINE/DEDENT itself
    if ((peek().type == "NEWLINE" || peek().type == "DEDENT") && tokenIndex < tokens.size()) {
        advance();
    }
    // If we didn't move, forcibly advance to avoid infinite loop
    if (tokenIndex == startIdx && tokenIndex < tokens.size()) {
        advance();
    }
}

Token peek(){
    if (tokenIndex > tokens.size()) {
        exit(1);
    }
    return tokens[tokenIndex];
}

bool is_assignment_target(int idx, string& op) {
    if (tokens[idx].type != "IDENTIFIER") return false;
    idx++;
    while (idx < tokens.size()) {
        if (tokens[idx].type == "DELIMITER" && tokens[idx].value == ".") {
            idx++;
            if (idx >= tokens.size() || tokens[idx].type != "IDENTIFIER") return false;
            idx++;
        } else if (tokens[idx].type == "DELIMITER" && tokens[idx].value == "[") {
            int bracketDepth = 1;
            idx++;
            while (idx < tokens.size() && bracketDepth > 0) {
                if (tokens[idx].type == "DELIMITER" && tokens[idx].value == "[") bracketDepth++;
                else if (tokens[idx].type == "DELIMITER" && tokens[idx].value == "]") bracketDepth--;
                idx++;
            }
        } else {
            break;
        }
    }
    if (idx < tokens.size() && tokens[idx].type == "OPERATOR") {
        string val = tokens[idx].value;
        if (val == "=" || val == "+=" || val == "-=" || val == "*=" ||
            val == "/=" || val == "%=" || val == "//=") {
            op = val;
            return true;
        }
    }
    return false;
}

bool is_inside_loop() {
    for (int i = tokenIndex - 1; i >= 0; i--) {
        if (tokens[i].value == "for" || tokens[i].value == "while") {
            int indent_level = 0;
            for (int j = i + 1; j < tokenIndex; j++) {
                if (tokens[j].type == "INDENT") indent_level++;
                else if (tokens[j].type == "DEDENT") indent_level--;
            }
            return indent_level > 0;
        }
    }
    return false;
}

void advance(){
    if(tokenIndex < tokens.size()){
        tokenIndex++;
        if(tokenIndex < tokens.size()) {
            currentToken = tokens[tokenIndex];
        }
    }
}

bool match(string expectedType){
    cout << "\nDEBUG: Matching - Expected: " << expectedType 
         << ", Current token - Type: " << currentToken.type 
         << ", Value: '" << currentToken.value << "'" << endl;
    
    if(currentToken.type == expectedType){
        advance();
        cout << "DEBUG: Match successful" << endl;
        return true;
    }
    else{
        cout << "DEBUG: Match failed" << endl;
        cout << "Syntax error: expected type '" << expectedType
             << "' but found type '" << currentToken.type << "' with value '" << currentToken.value
             << "' at line " << currentToken.line << endl;
        report_error("Syntax error: expected type '" + expectedType + "' but found type '" + currentToken.type + "'");
        synchronize();
        return false;

    }
}

shared_ptr<ParseTreeNode> parse_program() {
    auto node = make_shared<ParseTreeNode>("program");
    cout << "\nDEBUG: Starting program parsing..." << endl;
    while (tokenIndex < tokens.size() && peek().type != "END_OF_FILE") {
        auto child = parse_statement();
        if (child) node->addChild(child);
    }
    cout << "DEBUG: Program parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_statement() {
    if (error_recovery) {
        error_recovery = false;
        return nullptr;
    }
    auto node = make_shared<ParseTreeNode>("statement");
    cout << "\nDEBUG: Parsing statement" << endl;
    cout << "DEBUG: Current token - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;

    string op;
    if (peek().value == "for") {
        auto child = parse_for_stmt();
        if (child) node->addChild(child);
        return node;
    }
    if (peek().type == "IDENTIFIER" && is_assignment_target(tokenIndex, op)) {
        if(op == "=") {
            cout << "DEBUG: Found assignment statement" << endl;
            auto child = parse_assignment();
            if (child) node->addChild(child);
        } else if (op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=" || op == "//=") {
            cout << "DEBUG: Found augmented assignment statement" << endl;
            auto child = parse_augmented_assignment();
            if (child) node->addChild(child);
        }
    }
    else if(peek().type == "IDENTIFIER" && tokens[tokenIndex + 1].value == "("){
        cout << "DEBUG: Found function call" << endl;
        auto child = parse_func_call();
        if (child) node->addChild(child);
    }
    else if (peek().value == "import" || peek().value == "from") {
        cout << "DEBUG: Found import statement" << endl;
        auto child = parse_import_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "def") {
        cout << "DEBUG: Found function definition" << endl;
        auto child = parse_func_def();
        if (child) node->addChild(child);
    }
    else if(peek().value == "class") {
        cout<< "DEBUG: Found class definition" << endl;
        auto child = parse_class_def();
        if (child) node->addChild(child);
    }
    else if(peek().value == "try"){
        cout << "DEBUG: Found try statement" << endl;
        auto child = parse_try_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "return") {
        cout << "DEBUG: Found return statement" << endl;
        auto child = parse_return_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "if") {
        cout << "DEBUG: Found if statement" << endl;
        auto child = parse_if_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "while") {
        cout << "DEBUG: Found while statement" << endl;
        auto child = parse_while_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "for") {
        cout << "DEBUG: Found for-loop" << endl;
        auto child = parse_for_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "break") {
        cout << "DEBUG: Found break statement" << endl;
        auto child = parse_break_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "continue") {
        cout << "DEBUG: Found continue statement" << endl;
        auto child = parse_continue_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().type == "NEWLINE") {
        cout << "DEBUG: Found newline" << endl;
        node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
        advance();
    }
    else if(peek().value == "del"){
        cout<< "DEBUG: Found delete statement" << endl;
        auto child = parse_del_stmt();
        if (child) node->addChild(child);
    }
    else {
        cout << "DEBUG: Unexpected token in statement" << endl;
        cout << "Syntax error: unexpected token " << peek().type 
             << " with value '" << peek().value << "'" << endl;
        report_error("Syntax error: unexpected token " + peek().type + " with value '" + peek().value + "'");
        synchronize();
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_assignment(){
    auto node = make_shared<ParseTreeNode>("assignment");
    cout << "\nDEBUG: Starting assignment parsing" << endl;
    auto child1 = parse_assign_target();
    if (child1) node->addChild(child1);
    
    auto opNode = make_shared<ParseTreeNode>("OPERATOR", currentToken.value);
    node->addChild(opNode);
    match("OPERATOR");
    
    auto child2 = parse_expression();
    if (child2) node->addChild(child2);
    
    if (peek().type == "NEWLINE") {
        node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
        match("NEWLINE");
    }

    cout << "DEBUG: Assignment parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_assign_target(){
    auto node = make_shared<ParseTreeNode>("assign_target");
    cout<< "\nDEBUG: Starting assignment target parsing" << endl;
    auto child1 = parse_primary_target();
    if (child1) node->addChild(child1);
    auto child2 = parse_assign_target_tail();
    if (child2) node->addChild(child2);
    cout << "DEBUG: Assignment target parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_primary_target(){
    auto node = make_shared<ParseTreeNode>("primary_target");
    cout<< "\nDEBUG: Starting primary target parsing" << endl;
    if(peek().type == "IDENTIFIER"){
        cout << "DEBUG: Found identifier in primary target" << endl;
        node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
        match("IDENTIFIER");
        if(peek().type == "DELIMITER" && peek().value == "[") {
            cout << "DEBUG: Found list literal in primary target" << endl;
            node->addChild(make_shared<ParseTreeNode>("DELIMITER", "["));
            match("DELIMITER");
            auto child = parse_expression();
            if (child) node->addChild(child);
            node->addChild(make_shared<ParseTreeNode>("DELIMITER", "]"));
            match("DELIMITER");
        }
    }
    cout<< "DEBUG: Primary target parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_assign_target_tail(){
    auto node = make_shared<ParseTreeNode>("assign_target_tail");
    if(peek().type == "DELIMITER" && peek().value == "."){
        cout << "DEBUG: Found dot operator in assignment target" << endl;
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", "."));
        match("DELIMITER");
        node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
        match("IDENTIFIER");
        auto child = parse_assign_target_tail();
        if (child) node->addChild(child);
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_return_stmt(){
    auto node = make_shared<ParseTreeNode>("return_stmt");
    cout << "\nDEBUG: Starting return statement parsing" << endl;
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "return"));
    match("KEYWORD");
    auto child = parse_expression();
    if (child) node->addChild(child);
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    cout << "DEBUG: Return statement parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_if_stmt() {
    auto node = make_shared<ParseTreeNode>("if_stmt");
    cout << "\nDEBUG: Starting if statement parsing" << endl;
    
    // 'if' keyword
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "if"));
    match("KEYWORD");
    
    // Expression
    auto expr = parse_expression();
    if (expr) node->addChild(expr);
    
    // Colon operator
    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR");
    
    // Newline
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    
    // Indent
    node->addChild(make_shared<ParseTreeNode>("INDENT"));
    match("INDENT");
    
    // Statement list
    auto stmtList = parse_statement_list();
    if (stmtList) node->addChild(stmtList);
    
    // Dedent
    node->addChild(make_shared<ParseTreeNode>("DEDENT"));
    match("DEDENT");
    
    // Optional elif and else parts (they'll handle their own existence checks)
    if (peek().value == "elif") {
        auto elif = parse_elif_stmt();
        if (elif) node->addChild(elif);
    }
    
    if (peek().value == "else") {
        auto elsePart = parse_else_part();
        if (elsePart) node->addChild(elsePart);
    }
    
    cout << "DEBUG: If statement parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_elif_stmt(){
    auto node = make_shared<ParseTreeNode>("elif_stmt");
    cout << "\nDEBUG: Starting elif statement parsing" << endl;
    if(peek().value != "elif"){
        cout << "DEBUG: No elif clause found" << endl;
        return node;
    }
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "elif"));
    match("KEYWORD");
    auto expr = parse_expression();
    if (expr) node->addChild(expr);
    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR");
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    node->addChild(make_shared<ParseTreeNode>("INDENT"));
    match("INDENT");
    auto stmtList = parse_statement_list();
    if (stmtList) node->addChild(stmtList);
    node->addChild(make_shared<ParseTreeNode>("DEDENT"));
    match("DEDENT");
    cout << "DEBUG: Elif statement parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_else_part(){
    auto node = make_shared<ParseTreeNode>("else_part");
    cout << "\nDEBUG: Starting else part parsing" << endl;
    if(peek().value == "else"){
        cout << "DEBUG: Found else clause" << endl;
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", "else"));
        match("KEYWORD");
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
        match("OPERATOR");
        node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
        match("NEWLINE");
        node->addChild(make_shared<ParseTreeNode>("INDENT"));
        match("INDENT");
        auto stmtList = parse_statement_list();
        if (stmtList) node->addChild(stmtList);
        node->addChild(make_shared<ParseTreeNode>("DEDENT"));
        match("DEDENT");
    } else {
        cout << "DEBUG: No else clause found" << endl;
    }
    cout << "DEBUG: Else part parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_while_stmt(){
    auto node = make_shared<ParseTreeNode>("while_stmt");
    cout << "\nDEBUG: Starting while statement parsing" << endl;
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "while"));
    match("KEYWORD");
    auto expr = parse_expression();
    if (expr) node->addChild(expr);
    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR");
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    node->addChild(make_shared<ParseTreeNode>("INDENT"));
    match("INDENT");
    auto loopList = parse_loop_statement_list();
    if (loopList) node->addChild(loopList);
    node->addChild(make_shared<ParseTreeNode>("DEDENT"));
    match("DEDENT");
    cout << "DEBUG: While statement parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_func_call(){
    auto node = make_shared<ParseTreeNode>("func_call");
    cout << "\nDEBUG: Starting function call parsing" << endl;
    node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
    match("IDENTIFIER");
    node->addChild(make_shared<ParseTreeNode>("DELIMITER", "("));
    match("DELIMITER");  // Opening parenthesis
    auto argList = parse_argument_list();
    if (argList) node->addChild(argList);
    node->addChild(make_shared<ParseTreeNode>("DELIMITER", ")"));
    match("DELIMITER");  // Closing parenthesis
    if (tokenIndex < tokens.size() && peek().type == "NEWLINE") {
        node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
        match("NEWLINE");
    }
    cout << "DEBUG: Function call parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_argument_list() {
    auto node = make_shared<ParseTreeNode>("argument_list");
    cout << "\nDEBUG: Starting argument list parsing" << endl;
    if (peek().type != "DELIMITER" || peek().value != ")") {
        cout << "DEBUG: Found first argument" << endl;
        if (peek().type == "STRING_QUOTE") {
            node->addChild(make_shared<ParseTreeNode>("STRING_QUOTE", currentToken.value));
            match("STRING_QUOTE");  // Match opening quote
            if (peek().type == "STRING_LITERAL") {
                node->addChild(make_shared<ParseTreeNode>("STRING_LITERAL", currentToken.value));
                match("STRING_LITERAL");  // Match string content
            }
            node->addChild(make_shared<ParseTreeNode>("STRING_QUOTE", currentToken.value));
            match("STRING_QUOTE");  // Match closing quote
        } else {
            auto expr = parse_expression();
            if (expr) node->addChild(expr);  // Handle other types of arguments
        }
        auto prime = parse_argument_list_prime();
        if (prime) node->addChild(prime);  // Check for additional ones
    } else {
        cout << "DEBUG: Empty argument list" << endl;
    }
    cout << "DEBUG: Argument list parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_argument_list_prime(){
    auto node = make_shared<ParseTreeNode>("argument_list_prime");
    cout << "\nDEBUG: Starting argument list prime parsing" << endl;
    if(peek().type == "DELIMITER" && peek().value == ","){
        cout << "DEBUG: Found additional argument" << endl;
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", ","));
        match("DELIMITER");  // Comma
        auto expr = parse_expression();
        if (expr) node->addChild(expr);
        auto prime = parse_argument_list_prime();
        if (prime) node->addChild(prime);
    } else {
        cout << "DEBUG: No more arguments" << endl;
    }
    cout << "DEBUG: Argument list prime parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_statement_list(){
    if (error_recovery) {
        error_recovery = false;
        return nullptr;
    }
    auto node = make_shared<ParseTreeNode>("statement_list");
    cout << "\nDEBUG: Starting statement list parsing" << endl;
    if(peek().type == "IDENTIFIER" || peek().value == "return" || peek().value == "if" || 
       peek().value == "while" || peek().type == "NEWLINE"|| peek().type == "KEYWORD"|| peek().value == "try"){
        cout << "DEBUG: Found valid statement" << endl;
        auto stmt = parse_statement();
        if (stmt) node->addChild(stmt);
        auto rest = parse_statement_list();
        if (rest) node->addChild(rest);
    } else {
        cout << "DEBUG: End of statement list" << endl;
    }
    cout << "DEBUG: Statement list parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_expression(){
    auto node = make_shared<ParseTreeNode>("expression");
    cout << "\nDEBUG: Starting expression parsing" << endl;
    cout << "DEBUG: Current token in expression - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    auto boolTerm = parse_bool_term();
    if (boolTerm) node->addChild(boolTerm);
    auto boolExprPrime = parse_bool_expr_prime();
    if (boolExprPrime) node->addChild(boolExprPrime);

    if (peek().type == "KEYWORD" && peek().value == "if") {
        auto inlineIf = parse_inline_if_else();
        if (inlineIf) node->addChild(inlineIf);
    }
    cout << "DEBUG: Expression parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_bool_expr_prime(){
    auto node = make_shared<ParseTreeNode>("bool_expr_prime");
    cout << "DEBUG: Parsing boolean expression prime" << endl;
    if(peek().type == "OPERATOR" && peek().value == "or"){
        cout << "DEBUG: Found 'or' operator" << endl;
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", "or"));
        match("OPERATOR");
        auto boolTerm = parse_bool_term();
        if (boolTerm) node->addChild(boolTerm);
        auto boolExprPrime = parse_bool_expr_prime();
        if (boolExprPrime) node->addChild(boolExprPrime);
    }
    cout << "DEBUG: Boolean expression prime parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_bool_term(){
    auto node = make_shared<ParseTreeNode>("bool_term");
    cout << "DEBUG: Starting boolean term parsing" << endl;
    cout << "DEBUG: Current token in bool_term - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    auto boolFactor = parse_bool_factor();
    if (boolFactor) node->addChild(boolFactor);
    auto boolTermPrime = parse_bool_term_prime();
    if (boolTermPrime) node->addChild(boolTermPrime);
    cout << "DEBUG: Boolean term parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_bool_term_prime(){
    auto node = make_shared<ParseTreeNode>("bool_term_prime");
    cout << "DEBUG: Parsing boolean term prime" << endl;
    if(peek().type == "OPERATOR" && peek().value == "and"){
        cout << "DEBUG: Found 'and' operator" << endl;
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", "and"));
        match("OPERATOR");
        auto boolFactor = parse_bool_factor();
        if (boolFactor) node->addChild(boolFactor);
        auto boolTermPrime = parse_bool_term_prime();
        if (boolTermPrime) node->addChild(boolTermPrime);
    }
    cout << "DEBUG: Boolean term prime parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_bool_factor(){
    auto node = make_shared<ParseTreeNode>("bool_factor");
    cout << "DEBUG: Starting boolean factor parsing" << endl;
    cout << "DEBUG: Current token in bool_factor - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    if(peek().type == "OPERATOR" && peek().value == "not"){
        cout << "DEBUG: Found 'not' operator" << endl;
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", "not"));
        match("OPERATOR");
        auto boolFactor = parse_bool_factor();
        if (boolFactor) node->addChild(boolFactor);
    }
    else{
        auto relExpr = parse_rel_expr();
        if (relExpr) node->addChild(relExpr);
    }
    cout << "DEBUG: Boolean factor parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_rel_expr(){
    auto node = make_shared<ParseTreeNode>("rel_expr");
    cout << "DEBUG: Starting relational expression parsing" << endl;
    cout << "DEBUG: Current token in rel_expr - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    auto arithExpr = parse_arith_expr();
    if (arithExpr) node->addChild(arithExpr);
    if(peek().type == "OPERATOR" && (peek().value == ">" || peek().value == "<" || 
       peek().value == "==" || peek().value == "!=" || peek().value == ">=" || 
       peek().value == "<=")){
        cout << "DEBUG: Found relational operator" << endl;
        auto relOp = parse_rel_op();
        if (relOp) node->addChild(relOp);
        auto arithExpr2 = parse_arith_expr();
        if (arithExpr2) node->addChild(arithExpr2);
    }
    cout << "DEBUG: Relational expression parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_rel_op(){
    auto node = make_shared<ParseTreeNode>("rel_op");
    cout << "DEBUG: Parsing relational operator" << endl;
    if(peek().type == "OPERATOR" && (peek().value == ">" || peek().value == "<" || 
       peek().value == "==" || peek().value == "!=" || peek().value == ">=" || 
       peek().value == "<=")){
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", currentToken.value));
        match("OPERATOR");
    }
    else{
        cout << "Syntax error: expected relational operator but found " << peek().type << endl;
        report_error("Syntax error: expected relational operator but found " + peek().type);
        synchronize();

    }
    return node;
}

shared_ptr<ParseTreeNode> parse_arith_expr(){
    auto node = make_shared<ParseTreeNode>("arith_expr");
    cout << "DEBUG: Starting arithmetic expression parsing" << endl;
    cout << "DEBUG: Current token in arith_expr - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    auto term = parse_term();
    if (term) node->addChild(term);
    auto arithExprPrime = parse_arith_expr_prime();
    if (arithExprPrime) node->addChild(arithExprPrime);
    cout << "DEBUG: Arithmetic expression parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_arith_expr_prime(){
    auto node = make_shared<ParseTreeNode>("arith_expr_prime");
    cout << "DEBUG: Parsing arithmetic expression prime" << endl;
    cout << "DEBUG: Current token - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    
    if(peek().type == "OPERATOR" && (peek().value == "+" || peek().value == "-")){
        cout << "DEBUG: Found addition/subtraction operator" << endl;
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", currentToken.value));
        match("OPERATOR");
        auto term = parse_term();
        if (term) node->addChild(term);
        auto arithExprPrime = parse_arith_expr_prime();
        if (arithExprPrime) node->addChild(arithExprPrime);
    }
    cout << "DEBUG: Arithmetic expression prime parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_term(){
    auto node = make_shared<ParseTreeNode>("term");
    cout << "DEBUG: Starting term parsing" << endl;
    cout << "DEBUG: Current token in term - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    auto factor = parse_factor();
    if (factor) node->addChild(factor);
    auto termPrime = parse_term_prime();
    if (termPrime) node->addChild(termPrime);
    cout << "DEBUG: Term parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_term_prime(){
    auto node = make_shared<ParseTreeNode>("term_prime");
    cout << "DEBUG: Parsing term prime" << endl;
    cout << "DEBUG: Current token - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    
    if(peek().type == "OPERATOR" && (peek().value == "*" || peek().value == "/")){
        cout << "DEBUG: Found multiplication/division operator" << endl;
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", currentToken.value));
        match("OPERATOR");
        auto factor = parse_factor();
        if (factor) node->addChild(factor);
        auto termPrime = parse_term_prime();
        if (termPrime) node->addChild(termPrime);
    }
    cout << "DEBUG: Term prime parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_factor(){
    auto node = make_shared<ParseTreeNode>("factor");
    cout << "DEBUG: Starting factor parsing" << endl;
    cout << "DEBUG: Current token in factor - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    
    if(peek().value == "("){
        cout << "DEBUG: Found opening parenthesis" << endl;
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", "("));
        match("DELIMITER");
        auto expr = parse_expression();
        if (expr) node->addChild(expr);
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", ")"));
        match("DELIMITER");
    }
    else if(peek().type == "IDENTIFIER"){
        cout << "DEBUG: Found identifier" << endl;

        if (tokenIndex + 1 < tokens.size() && tokens[tokenIndex + 1].value == "(") {
            auto funcCall = parse_func_call();
            if (funcCall) node->addChild(funcCall);
        }   else   {
            node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
            match("IDENTIFIER");
        }
    }
    else if (peek().value == "{") {
        cout << "DEBUG: Found dictionary literal" << endl;
        auto dictLit = parse_dict_literal();
        if (dictLit) node->addChild(dictLit);
    }
    else if(peek().type == "NUMBER"){
        cout << "DEBUG: Found number" << endl;
        node->addChild(make_shared<ParseTreeNode>("NUMBER", currentToken.value));
        match("NUMBER");
    }
    else if (peek().type == "STRING_QUOTE") {
        cout << "DEBUG: Found string literal" << endl;
        node->addChild(make_shared<ParseTreeNode>("STRING_QUOTE", currentToken.value));
        match("STRING_QUOTE");  // Match opening quote
        std::string literalContent;
        while (peek().type != "STRING_QUOTE") {
            if (peek().type == "END_OF_FILE") {
                cout << "Syntax error: unterminated string literal" << endl;
                report_error("Syntax error: unterminated string literal");
                synchronize();
            }
            if (peek().type == "STRING_LITERAL") {
                node->addChild(make_shared<ParseTreeNode>("STRING_LITERAL", currentToken.value));
                match("STRING_LITERAL");
            } else if (peek().type == "NEWLINE") {
                node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
                match("NEWLINE");
            } else {
                cout << "Syntax error: unexpected token inside string literal: " << peek().type << endl;
                report_error("Syntax error: unexpected token inside string literal: " + peek().type);
                synchronize();
            }
        }
        node->addChild(make_shared<ParseTreeNode>("STRING_QUOTE", currentToken.value));
        match("STRING_QUOTE");  // Match closing quote
    }
    else if (peek().value == "[") {
        cout << "DEBUG: Found list literal" << endl;
        auto listLit = parse_list_literal();
        if (listLit) node->addChild(listLit);
    }
    else{
        cout << "DEBUG: Unexpected token in factor" << endl;
        cout << "Syntax error: expected factor but found " << peek().type 
             << " with value '" << peek().value << "'" << endl;
        report_error("Syntax error: expected factor but found " + peek().type + " with value '" + peek().value + "'");
        synchronize();
    }
    cout << "DEBUG: Factor parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_augmented_assignment() {
    auto node = make_shared<ParseTreeNode>("augmented_assignment");
    cout << "\nDEBUG: Starting augmented assignment parsing" << endl;

    node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
    match("IDENTIFIER");

    if (peek().type == "OPERATOR" && (
        peek().value == "+=" || peek().value == "-=" ||
        peek().value == "*=" || peek().value == "/=" ||
        peek().value == "%=" || peek().value == "//=")) 
    {
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", currentToken.value));
        match("OPERATOR");
    } else {
        cout << "Syntax error: expected augmented assignment operator but found '"
             << peek().value << "' of type " << peek().type << endl;
        report_error("Syntax error: expected augmented assignment operator but found '" + peek().value + "'");
        synchronize();
    }

    auto expr = parse_expression();
    if (expr) node->addChild(expr);
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");

    cout << "DEBUG: Augmented assignment parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_for_stmt() {
    auto node = make_shared<ParseTreeNode>("for_stmt");
    cout << "\nDEBUG: Starting for-loop parsing" << endl;
    if (peek().value != "for") {
        cout << "Syntax error: expected 'for' keyword but found '" << peek().value << "'" << endl;
        report_error("Syntax error: expected 'for' keyword but found '" + peek().value + "'");
        synchronize();
    }

    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "for"));
    match("KEYWORD");         // 'for'

    if (peek().type != "IDENTIFIER")    {
        cout << "Syntax error: expected loop variable, but found '" << peek().value 
             << "' of type '" << peek().type << "'" << endl;
        report_error("Syntax error: expected loop variable, but found '" + peek().value + "' of type '" + peek().type + "'");
        synchronize();

        string loopVar = peek().value;
        if (loopVar == "for" || loopVar == "in" || loopVar == "if" || loopVar == "while" || peek().type == "NUMBER") {
            cout << "Syntax error: invalid loop variable '" << loopVar << "'" << endl;
            report_error("Syntax error: invalid loop variable '" + loopVar + "'");
            synchronize();
        }
    }

    node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
    match("IDENTIFIER");      // loop variable

    if (peek().value != "in") {
        cout << "Syntax error: expected 'in' keyword but found '" << peek().value << "'" << endl;
        report_error("Syntax error: expected 'in' keyword but found '" + peek().value + "'");
        synchronize();
    }

    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "in"));
    match("KEYWORD");         // 'in'

    int exprStartIndex = tokenIndex;

    if (peek().type == "OPERATOR" && peek().value == ":") {
        cout << "Syntax error: expected iterable expression after 'in', but found ':'" << endl;
        report_error("Syntax error: expected iterable expression after 'in', but found ':'");
        synchronize();
    }
    auto expr = parse_expression();
    if (expr) node->addChild(expr);

    if (tokenIndex == exprStartIndex) {
        cout << "Syntax error: expected iterator expression after 'in' but found nothing" << endl;
        report_error("Syntax error: expected iterator expression after 'in' but found nothing");
        synchronize();
    }

    if (peek().value != ":") {
        cout << "Syntax error: expected ':' after iterable but found '" << peek().value << "'" << endl;
        report_error("Syntax error: expected ':' after iterable but found '" + peek().value + "'");
        synchronize();
    }

    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR");       // ':'

    if (peek().type != "NEWLINE") {
        cout << "Syntax error: expected NEWLINE after ':' but found '" << peek().value << "'" << endl;
        report_error("Syntax error: expected NEWLINE after ':' but found '" + peek().value + "'");  
        synchronize();
    }
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");

    if (peek().type != "INDENT") {
        cout << "Syntax error: expected INDENT after NEWLINE but found '" << peek().value << "'" << endl;
        report_error("Syntax error: expected INDENT after NEWLINE but found '" + peek().value + "'");
        synchronize();
    }

    node->addChild(make_shared<ParseTreeNode>("INDENT"));
    match("INDENT");
    auto loopList = parse_loop_statement_list();
    if (loopList) node->addChild(loopList);
    
    if (peek().type != "DEDENT") {
        cout << "Syntax error: expected DEDENT after loop body but found '" << peek().value << "'" << endl;
        report_error("Syntax error: expected DEDENT after loop body but found '" + peek().value + "'");
        synchronize();
    }
    node->addChild(make_shared<ParseTreeNode>("DEDENT"));
    match("DEDENT");
    cout << "DEBUG: For-loop parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_list_literal() {
    auto node = make_shared<ParseTreeNode>("list_literal");
    cout << "\nDEBUG: Starting list literal parsing" << endl;

    node->addChild(make_shared<ParseTreeNode>("DELIMITER", "["));
    match("DELIMITER");  // '['

    if (peek().value != "]") {
        cout << "DEBUG: Parsing first list item" << endl;
        auto expr = parse_expression();
        if (expr) node->addChild(expr);
        auto prime = parse_list_items_prime();
        if (prime) node->addChild(prime);
    } else {
        cout << "DEBUG: Empty list" << endl;
    }

    node->addChild(make_shared<ParseTreeNode>("DELIMITER", "]"));
    match("DELIMITER");  // ']'

    cout << "DEBUG: List literal parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_list_items_prime() {
    auto node = make_shared<ParseTreeNode>("list_items_prime");
    cout << "DEBUG: Parsing list items prime" << endl;

    if (peek().type == "DELIMITER" && peek().value == ",") {
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", ","));
        match("DELIMITER");  // ','
        auto expr = parse_expression();
        if (expr) node->addChild(expr);
        auto prime = parse_list_items_prime();
        if (prime) node->addChild(prime);
    } else {
        cout << "DEBUG: No more list items" << endl;
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_func_def() {
    auto node = make_shared<ParseTreeNode>("func_def");
    cout << "\nDEBUG: Starting function definition parsing" << endl;

    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "def"));
    match("KEYWORD");         // 'def'
    node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
    match("IDENTIFIER");      // function name
    node->addChild(make_shared<ParseTreeNode>("DELIMITER", "("));
    match("DELIMITER");       // '('
    auto paramList = parse_param_list();
    if (paramList) node->addChild(paramList);
    node->addChild(make_shared<ParseTreeNode>("DELIMITER", ")"));
    match("DELIMITER");       // ')'

    // Optional return type
    if (peek().type == "OPERATOR" && peek().value == "->") {
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", "->"));
        match("OPERATOR");
        auto typeNode = parse_type();
        if (typeNode) node->addChild(typeNode);
    }

    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR");        // ':'
    // Detect if it's a single-line body

    if (peek().type != "NEWLINE") {
        cout << "DEBUG: Detected single-line function definition" << endl;
        auto stmt = parse_statement();  // just one statement (like return, assignment, etc.)
        if (stmt) node->addChild(stmt);
    } else {
        // Multiline function
        node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
        match("NEWLINE");
        node->addChild(make_shared<ParseTreeNode>("INDENT"));
        match("INDENT");
        auto stmtList = parse_statement_list();
        if (stmtList) node->addChild(stmtList);
        node->addChild(make_shared<ParseTreeNode>("DEDENT"));
        match("DEDENT");
    }

    cout << "DEBUG: Function definition parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_param_list() {
    auto node = make_shared<ParseTreeNode>("param_list");
    cout << "DEBUG: Starting parameter list parsing" << endl;

    if (peek().type == "IDENTIFIER") {
        auto param = parse_param();
        if (param) node->addChild(param);

        while (peek().type == "DELIMITER" && peek().value == ",") {
            node->addChild(make_shared<ParseTreeNode>("DELIMITER", ","));
            match("DELIMITER");
            auto param2 = parse_param();
            if (param2) node->addChild(param2);
        }
    }

    cout << "DEBUG: Parameter list parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_param() {
    auto node = make_shared<ParseTreeNode>("param");
    node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
    match("IDENTIFIER");

    if (peek().type == "OPERATOR" && peek().value == "=") {
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", "="));
        match("OPERATOR");
        auto expr = parse_expression();
        if (expr) node->addChild(expr);
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_type() {
    auto node = make_shared<ParseTreeNode>("type");
    if (peek().type == "KEYWORD" && 
        (peek().value == "int" || peek().value == "float" || 
         peek().value == "str" || peek().value == "bool" || 
         peek().value == "None")) 
    {
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", currentToken.value));
        match("KEYWORD");
    } else {
        cout << "Syntax error: expected type but found " << peek().type 
             << " with value '" << peek().value << "'" << endl;
        report_error("Syntax error: expected type but found " + peek().type + " with value '" + peek().value + "'");
        synchronize();
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_import_stmt() {
    auto node = make_shared<ParseTreeNode>("import_stmt");
    cout << "\nDEBUG: Starting import statement parsing" << endl;

    if (peek().value == "import") {
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", "import"));
        match("KEYWORD");  // 'import'
        auto importItem = parse_import_item();
        if (importItem) node->addChild(importItem);
        auto importTail = parse_import_tail();
        if (importTail) node->addChild(importTail);
    } 
    else if (peek().value == "from") {
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", "from"));
        match("KEYWORD");        // 'from'
        node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
        match("IDENTIFIER");     // module name
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", "import"));
        match("KEYWORD");        // 'import'
        auto importItem = parse_import_item();
        if (importItem) node->addChild(importItem);
        auto importTail = parse_import_tail();
        if (importTail) node->addChild(importTail);
    } 
    else {
        cout << "Syntax error: expected 'import' or 'from'" << endl;
        report_error("Syntax error: expected 'import' or 'from'");
        synchronize();
    }

    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    cout << "DEBUG: Import statement parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_import_item() {
    auto node = make_shared<ParseTreeNode>("import_item");
    if (peek().type == "IDENTIFIER") {
        node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
        match("IDENTIFIER");
        auto aliasOpt = parse_import_alias_opt();
        if (aliasOpt) node->addChild(aliasOpt);
    } 
    else if (peek().type == "OPERATOR" && peek().value == "*") {
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", "*"));
        match("OPERATOR");  // '*'
        auto aliasOpt = parse_import_alias_opt();
        if (aliasOpt) node->addChild(aliasOpt);
    }
    else {
        cout << "Syntax error: expected module name in import" << endl;
        report_error("Syntax error: expected module name in import");
        synchronize();
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_import_tail() {
    auto node = make_shared<ParseTreeNode>("import_tail");
    while (peek().type == "DELIMITER" && peek().value == ",") {
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", ","));
        match("DELIMITER");      // ','
        auto importItem = parse_import_item();
        if (importItem) node->addChild(importItem);
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_import_alias_opt() {
    auto node = make_shared<ParseTreeNode>("import_alias_opt");
    if (peek().value == "as") {
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", "as"));
        match("KEYWORD");        // 'as'
        if (peek().type == "IDENTIFIER") {
            node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
            match("IDENTIFIER");  // alias
        } else {
            cout << "Syntax error: expected alias after 'as'" << endl;
            report_error("Syntax error: expected alias after 'as'");
            synchronize();
        }
    } else {
        cout << "DEBUG: No alias in import" << endl;
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_dict_literal() {
    auto node = make_shared<ParseTreeNode>("dict_literal");
    cout << "\nDEBUG: Starting dictionary literal parsing" << endl;

    node->addChild(make_shared<ParseTreeNode>("DELIMITER", "{"));
    match("DELIMITER");  // '{'

    if (peek().value != "}") {
        auto dictPair = parse_dict_pair();
        if (dictPair) node->addChild(dictPair);
        auto dictItemsPrime = parse_dict_items_prime();
        if (dictItemsPrime) node->addChild(dictItemsPrime);
    } else {
        cout << "DEBUG: Empty dictionary" << endl;
    }

    node->addChild(make_shared<ParseTreeNode>("DELIMITER", "}"));
    match("DELIMITER");  // '}'

    cout << "DEBUG: Dictionary literal parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_dict_items_prime() {
    auto node = make_shared<ParseTreeNode>("dict_items_prime");
    while (peek().type == "DELIMITER" && peek().value == ",") {
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", ","));
        match("DELIMITER");
        auto dictPair = parse_dict_pair();
        if (dictPair) node->addChild(dictPair);
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_dict_pair() {
    auto node = make_shared<ParseTreeNode>("dict_pair");
    cout << "DEBUG: Parsing dictionary key" << endl;

    if (peek().type == "STRING_QUOTE") {
        auto strKey = parse_string_key();
        if (strKey) node->addChild(strKey);
    }
    else if (peek().type == "IDENTIFIER") {
        if (tokenIndex + 1 < tokens.size() && tokens[tokenIndex + 1].value == "(") {
            auto funcCall = parse_func_call();
            if (funcCall) node->addChild(funcCall);
        } else {
            node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
            match("IDENTIFIER");
        }
    }
    else if (peek().type == "NUMBER") {
        node->addChild(make_shared<ParseTreeNode>("NUMBER", currentToken.value));
        match("NUMBER");
    }
    else if (peek().type == "KEYWORD" && 
             (peek().value == "True" || peek().value == "False" || peek().value == "None")) {
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", currentToken.value));
        match("KEYWORD");
    }
    else {
        cout << "Syntax error: unsupported dictionary key type" << endl;
        report_error("Syntax error: unsupported dictionary key type");
        synchronize();
    }

    if (peek().type == "OPERATOR" && peek().value == ":") {
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
        match("OPERATOR");  // ':'
        auto expr = parse_expression();
        if (expr) node->addChild(expr); // value expression
    } else {
        cout << "Syntax error: expected ':' in dictionary pair" << endl;
        report_error("Syntax error: expected ':' in dictionary pair");
        synchronize();
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_loop_statement_list() {
    if (error_recovery) {
        error_recovery = false;
        return nullptr;
    }
    auto node = make_shared<ParseTreeNode>("loop_statement_list");
    cout << "DEBUG: Starting loop statement list" << endl;
    while (peek().type != "DEDENT" && peek().type != "END_OF_FILE") {
        auto stmt = parse_loop_statement();
        if (stmt) node->addChild(stmt);
    }
    cout << "DEBUG: Completed loop statement list" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_loop_statement() {
    auto node = make_shared<ParseTreeNode>("loop_statement");
    if (peek().value == "break") {
        auto child = parse_break_stmt();
        if (child) node->addChild(child);
    }
    else if (peek().value == "continue") {
        auto child = parse_continue_stmt();
        if (child) node->addChild(child);
    }
    else {
        auto child = parse_statement();
        if (child) node->addChild(child);
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_del_stmt() {
    auto node = make_shared<ParseTreeNode>("del_stmt");
    cout << "\nDEBUG: Starting delete statement parsing" << endl;

    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "del"));
    match("KEYWORD");  // 'del'
    auto delTarget = parse_del_target();
    if (delTarget) node->addChild(delTarget);
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");

    cout << "DEBUG: Delete statement parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_del_target() {
    auto node = make_shared<ParseTreeNode>("del_target");
    cout << "DEBUG: Starting delete target parsing" << endl;
    node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
    match("IDENTIFIER");  
    if(peek().type == "DELIMITER" && peek().value == "["){
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", "["));
        match("DELIMITER");  // '['
        auto expr = parse_expression();
        if (expr) node->addChild(expr);
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", "]"));
        match("DELIMITER");  // ']'
    }
    else if(peek().type == "DELIMITER" && peek().value == "."){
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", "."));
        match("DELIMITER");  // '.'
        node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
        match("IDENTIFIER");  // attribute to delete
    }
    else{
        cout << "DEBUG: No additional delete target found" << endl;
    }

    cout << "DEBUG: Delete target parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_inline_if_else() {
    auto node = make_shared<ParseTreeNode>("inline_if_else");
    cout << "\nDEBUG: Starting inline if/else expression parsing" << endl;
    
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "if"));
    match("KEYWORD");  // match 'if'
    auto expr1 = parse_expression();
    if (expr1) node->addChild(expr1);
    
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "else"));
    match("KEYWORD");  // match 'else'
    auto expr2 = parse_expression();
    if (expr2) node->addChild(expr2);  // parse expression after else

    cout << "DEBUG: Inline if/else expression parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_string_key() {
    auto node = make_shared<ParseTreeNode>("string_key");
    if (peek().type == "STRING_QUOTE") {
        node->addChild(make_shared<ParseTreeNode>("STRING_QUOTE", currentToken.value));
        match("STRING_QUOTE");         // opening quote
        if (peek().type == "STRING_LITERAL") {
            node->addChild(make_shared<ParseTreeNode>("STRING_LITERAL", currentToken.value));
            match("STRING_LITERAL");   // string content
        } else {
            cout << "Syntax error: expected string literal inside quotes" << endl;
            report_error("Syntax error: expected string literal inside quotes");
            synchronize();
        }
        if (peek().type == "STRING_QUOTE") {
            node->addChild(make_shared<ParseTreeNode>("STRING_QUOTE", currentToken.value));
            match("STRING_QUOTE");     // closing quote
        } else {
            cout << "Syntax error: expected closing quote" << endl;
            report_error("Syntax error: expected closing quote");
            synchronize();
        }
    } else {
        cout << "Syntax error: expected opening quote for string key" << endl;
        report_error("Syntax error: expected opening quote for string key");
        synchronize();
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_class_def() {
    auto node = make_shared<ParseTreeNode>("class_def");
    cout << "\nDEBUG: Starting class definition parsing" << endl;

    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "class"));
    match("KEYWORD");        // 'class'
    node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
    match("IDENTIFIER");     // class name
    auto inhOpt = parse_class_inheritance_opt();
    if (inhOpt) node->addChild(inhOpt);
    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR");       // ':'
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    node->addChild(make_shared<ParseTreeNode>("INDENT"));
    match("INDENT");
    auto stmtList = parse_statement_list();
    if (stmtList) node->addChild(stmtList);
    node->addChild(make_shared<ParseTreeNode>("DEDENT"));
    match("DEDENT");

    cout << "DEBUG: Class definition parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_class_inheritance_opt() {
    auto node = make_shared<ParseTreeNode>("class_inheritance_opt");
    if (peek().value == "(") {
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", "("));
        match("DELIMITER");      // '('
        node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
        match("IDENTIFIER");     // base class
        node->addChild(make_shared<ParseTreeNode>("DELIMITER", ")"));
        match("DELIMITER");      // ')'
    } else {
        cout << "DEBUG: No base class (inheritance) specified" << endl;
    }
    return node;
}

shared_ptr<ParseTreeNode> parse_try_stmt() {
    auto node = make_shared<ParseTreeNode>("try_stmt");
    cout << "\nDEBUG: Starting try statement parsing" << endl;
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "try"));
    match("KEYWORD");  // 'try'
    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR"); // ':'
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    node->addChild(make_shared<ParseTreeNode>("INDENT"));
    match("INDENT");
    auto stmtList = parse_statement_list();
    if (stmtList) node->addChild(stmtList);
    node->addChild(make_shared<ParseTreeNode>("DEDENT"));
    match("DEDENT");
    auto excepts = parse_except_clauses();
    if (excepts) node->addChild(excepts);
    auto finally = parse_finally_clause();
    if (finally) node->addChild(finally);
    cout << "DEBUG: Try statement parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_except_clauses() {
    auto node = make_shared<ParseTreeNode>("except_clauses");
    cout << "\nDEBUG: Starting except clauses parsing" << endl;
    while (peek().value == "except") {
        auto except = parse_except_clause();
        if (except) node->addChild(except);
    }
    cout << "DEBUG: Except clauses parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_except_clause() {
    auto node = make_shared<ParseTreeNode>("except_clause");
    cout << "\nDEBUG: Starting except clause parsing" << endl;
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "except"));
    match("KEYWORD");  // 'except'
    
    // Optional exception type
    if (peek().type != "OPERATOR" || peek().value != ":") {
        auto expr = parse_expression();
        if (expr) node->addChild(expr);
    }
    
    // Optional 'as' identifier
    if (peek().value == "as") {
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", "as"));
        match("KEYWORD");  // 'as'
        node->addChild(make_shared<ParseTreeNode>("IDENTIFIER", currentToken.value));
        match("IDENTIFIER");
    }
    
    node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
    match("OPERATOR"); // ':'
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    node->addChild(make_shared<ParseTreeNode>("INDENT"));
    match("INDENT");
    auto stmtList = parse_statement_list();
    if (stmtList) node->addChild(stmtList);
    node->addChild(make_shared<ParseTreeNode>("DEDENT"));
    match("DEDENT");
    cout << "DEBUG: Except clause parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_finally_clause() {
    auto node = make_shared<ParseTreeNode>("finally_clause");
    cout << "\nDEBUG: Checking for finally clause" << endl;
    if (peek().value == "finally") {
        cout << "DEBUG: Found finally clause" << endl;
        node->addChild(make_shared<ParseTreeNode>("KEYWORD", "finally"));
        match("KEYWORD");  // 'finally'
        node->addChild(make_shared<ParseTreeNode>("OPERATOR", ":"));
        match("OPERATOR"); // ':'
        node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
        match("NEWLINE");
        node->addChild(make_shared<ParseTreeNode>("INDENT"));
        match("INDENT");
        auto stmtList = parse_statement_list();
        if (stmtList) node->addChild(stmtList);
        node->addChild(make_shared<ParseTreeNode>("DEDENT"));
        match("DEDENT");
    } else {
        cout << "DEBUG: No finally clause found" << endl;
    }
    cout << "DEBUG: Finally clause parsing completed" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_break_stmt() {
    auto node = make_shared<ParseTreeNode>("break_stmt");
    cout << "\nDEBUG: Parsing break statement" << endl;
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "break"));
    match("KEYWORD");  // 'break'
    
    // Check if we're inside a loop
    if (!is_inside_loop()) {
        cout << "Syntax error: 'break' outside loop" << endl;
        report_error("Syntax error: 'break' outside loop"); 
        synchronize();
    }
    
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    cout << "DEBUG: Break statement parsed successfully" << endl;
    return node;
}

shared_ptr<ParseTreeNode> parse_continue_stmt() {
    auto node = make_shared<ParseTreeNode>("continue_stmt");
    cout << "\nDEBUG: Parsing continue statement" << endl;
    node->addChild(make_shared<ParseTreeNode>("KEYWORD", "continue"));
    match("KEYWORD");  // 'continue'
    
    // Check if we're inside a loop
    if (!is_inside_loop()) {
        cout << "Syntax error: 'continue' outside loop" << endl;
        report_error("Syntax error: 'continue' outside loop");
        synchronize();
    }
    
    node->addChild(make_shared<ParseTreeNode>("NEWLINE"));
    match("NEWLINE");
    cout << "DEBUG: Continue statement parsed successfully" << endl;
    return node;
}

void printParseTree(const shared_ptr<ParseTreeNode>& node, int depth = 0) {
    if (!node) return;
    
    cout << string(depth * 2, ' ') << node->name;
    if (!node->value.empty()) {
        cout << " (" << node->value << ")";
    }
    cout << endl;
    
    for (const auto& child : node->children) {
        printParseTree(child, depth + 1);
    }
}

void exportParseTreeToDot(const shared_ptr<ParseTreeNode>& node, ofstream& out, int& counter, int parent = -1) {
    if (!node) return;
    
    int current = counter++;
    out << "    node" << current << " [label=\"";
    out << escapeDotString(node->name);
    if (!node->value.empty()) {
        out << "\\n" << escapeDotString(node->value);
    }
    out << "\"];\n";
    
    if (parent != -1) {
        out << "    node" << parent << " -> node" << current << ";\n";
    }
    
    for (const auto& child : node->children) {
        exportParseTreeToDot(child, out, counter, current);
    }
}

string escapeDotString(const string& input) {
    string output;
    for (char c : input) {
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c;
        }
    }
    return output;
}

void saveParseTreeToDot(const shared_ptr<ParseTreeNode>& root, const string& filename) {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error opening file for writing: " << filename << endl;
        return;
    }
    
    out << "digraph ParseTree {\n";
    out << "    node [shape=box, fontname=\"Arial\"];\n";
    out << "    edge [arrowhead=vee];\n";
    out << "    rankdir=TB;\n";
    
    int counter = 0;
    exportParseTreeToDot(root, out, counter);
    
    out << "}\n";
    out.close();
    
    cout << "Parse tree saved to " << filename << endl;
    
    // Add automatic PNG generation
    string pngFilename = filename.substr(0, filename.find_last_of('.')) + ".png";
    string command = "dot -Tpng " + filename + " -o " + pngFilename;
    int result = system(command.c_str());
    
    if (result == 0) {
        cout << "PNG image generated: " << pngFilename << endl;
    } else {
        cout << "Failed to generate PNG. Make sure Graphviz is installed." << endl;
        cout << "You can try manually with: dot -Tpng " << filename << " -o tree.png" << endl;
    }
}

int main() {
    string input;
    cout << "PYTHON LEXICAL ANALYZER\n";
    cout << "=======================\n";
    cout << "1. Enter Python code manually\n";
    cout << "2. Read from file\n";
    cout << "Choose option (1/2): ";

    int option;
    cin >> option;
    cin.ignore();

    if (option == 1) {
        cout << "\nEnter Python code (end with empty line):\n";
        string line;
        while (true) {
            getline(cin, line);
            if (line.empty()) break;
            input += line + "\n";
        }
    } else if (option == 2) {
        string filename;
        cout << "\nEnter filename: ";
        getline(cin, filename);

        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error opening file!\n";
            return 1;
        }

        input = string((istreambuf_iterator<char>(file)),
            istreambuf_iterator<char>());
        file.close();
    } else {
        cerr << "Invalid option!\n";
        return 1;
    }

    cout << "\nDEBUG: Tokenizing input..." << endl;
    tokens = tokenize(input);

    cout << "\nTOKENS FOUND\n";
    cout << "============\n";
    printTokenTable(tokens);

    generateSymbolTable(tokens);

    cout << "\nDEBUG: Starting parser..." << endl;
    tokenIndex = 0;
    currentToken = tokens[tokenIndex];
    parseTreeRoot = parse_program();
    
    cout << "\nPARSE TREE:\n";
    printParseTree(parseTreeRoot);
    
    saveParseTreeToDot(parseTreeRoot, "parse_tree.dot");
         if (!error_messages.empty()) {
        cout << "\nERRORS FOUND DURING PARSING\n";
        cout << "===========================\n";
        for (const auto& msg : error_messages) {
            cout << msg << endl;
        }
    } else {
        cout << "DEBUG: Parser completed successfully" << endl;
    }

    cout << "DEBUG: Parser completed successfully" << endl;
    return 0;
}