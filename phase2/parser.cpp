#include "lexical_analyzer.h"
#include "parser.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <iomanip>
#include <algorithm>

using namespace std;

vector<Token> tokens;
Token currentToken;
int tokenIndex = 0;

Token peek(){
    if (tokenIndex >= tokens.size()) {
        
        exit(1);
    }

    return tokens[tokenIndex];
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
        exit(1);
    }
}

void parse_program() {
    cout << "\nDEBUG: Starting program parsing..." << endl;
    while (tokenIndex < tokens.size() && peek().type != "END_OF_FILE") {
        parse_statement();
    }
    cout << "DEBUG: Program parsing completed" << endl;
}

void parse_statement() {
    cout << "\nDEBUG: Parsing statement" << endl;
    cout << "DEBUG: Current token - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;

    if (peek().type == "IDENTIFIER" && tokenIndex + 1 < tokens.size()) {
        string nextVal = tokens[tokenIndex + 1].value;
        if (nextVal == "+=" || nextVal == "-=" || nextVal == "*=" ||
            nextVal == "/=" || nextVal == "%=" || nextVal == "//=") 
        {
            cout << "DEBUG: Found augmented assignment" << endl;
            parse_augmented_assignment();
        }
        else if (nextVal == "=") {
            cout << "DEBUG: Found assignment statement" << endl;
            parse_assignment();
        }
        else if (nextVal == "(") {
            cout << "DEBUG: Found function call" << endl;
            parse_func_call();
        } else {
            cout << "DEBUG: Unexpected second token after IDENTIFIER: '" << nextVal << "'" << endl;
            exit(1);
        }
    }
    else if (peek().value == "import" || peek().value == "from") {
        cout << "DEBUG: Found import statement" << endl;
        parse_import_stmt();
    }
    else if (peek().value == "def") {
        cout << "DEBUG: Found function definition" << endl;
        parse_func_def();
    }
    else if (peek().value == "class") {
        cout << "DEBUG: Found class definition" << endl;
        parse_class_def();
    }
    else if (peek().value == "try") {
        cout << "DEBUG: Found try statement" << endl;
        parse_try_stmt();
    }
    else if (peek().value == "return") {
        cout << "DEBUG: Found return statement" << endl;
        parse_return_stmt();
    }
    else if (peek().value == "if") {
        cout << "DEBUG: Found if statement" << endl;
        parse_if_stmt();
    }
    else if (peek().value == "while") {
        cout << "DEBUG: Found while statement" << endl;
        parse_while_stmt();
    }
    else if (peek().value == "for") {
        cout << "DEBUG: Found for-loop" << endl;
        parse_for_stmt();
    }
    else if (peek().type == "NEWLINE") {
        cout << "DEBUG: Found newline" << endl;
        advance();
    }
    else {
        cout << "DEBUG: Unexpected token in statement" << endl;
        cout << "Syntax error: unexpected token " << peek().type 
             << " with value '" << peek().value << "'" << endl;
        exit(1);
    }  
}

void parse_assignment(){
    cout << "\nDEBUG: Starting assignment parsing" << endl;
    match("IDENTIFIER");
    match("OPERATOR");
    parse_expression();
    
    // Only match NEWLINE if there is one (for ternary expressions)
    if (peek().type == "NEWLINE") {
        match("NEWLINE");
    }
    
    cout << "DEBUG: Assignment parsing completed" << endl;
}

void parse_return_stmt(){
    cout << "\nDEBUG: Starting return statement parsing" << endl;
    match("KEYWORD");
    parse_expression();
    match("NEWLINE");
    cout << "DEBUG: Return statement parsing completed" << endl;
}


void parse_if_stmt(){
    cout << "\nDEBUG: Starting if statement parsing" << endl;
    match("KEYWORD");
    parse_expression();
    match("OPERATOR");     
    match("NEWLINE");
    match("INDENT");
    parse_statement_list();
    match("DEDENT");
    parse_elif_stmt();
    parse_else_part();
    cout << "DEBUG: If statement parsing completed" << endl;
}

void parse_elif_stmt(){
    cout << "\nDEBUG: Starting elif statement parsing" << endl;
    if(peek().value != "elif"){
        cout << "DEBUG: No elif clause found" << endl;
        return;
    }
    match("KEYWORD");
    parse_expression();
    match("OPERATOR");
    match("NEWLINE");
    match("INDENT");
    parse_statement_list();
    match("DEDENT");
    cout << "DEBUG: Elif statement parsing completed" << endl;
}

void parse_else_part(){
    cout << "\nDEBUG: Starting else part parsing" << endl;
    if(peek().value == "else"){
        cout << "DEBUG: Found else clause" << endl;
        match("KEYWORD");
        match("OPERATOR");
        match("NEWLINE");
        match("INDENT");
        parse_statement_list();
        match("DEDENT");
    } else {
        cout << "DEBUG: No else clause found" << endl;
    }
    cout << "DEBUG: Else part parsing completed" << endl;
}

void parse_while_stmt(){
    cout << "\nDEBUG: Starting while statement parsing" << endl;
    match("KEYWORD");
    parse_expression();
    match("OPERATOR");
    match("NEWLINE");
    match("INDENT");
    parse_loop_statement_list();
    match("DEDENT");
    cout << "DEBUG: While statement parsing completed" << endl;
}

void parse_func_call(){
    cout << "\nDEBUG: Starting function call parsing" << endl;
    match("IDENTIFIER");
    match("DELIMITER");  // Opening parenthesis
    parse_argument_list();
    match("DELIMITER");  // Closing parenthesis
    if (tokenIndex < tokens.size() && peek().type == "NEWLINE") {
        match("NEWLINE");
    }
    cout << "DEBUG: Function call parsing completed" << endl;
}

void parse_argument_list() {
    cout << "\nDEBUG: Starting argument list parsing" << endl;
    if (peek().type != "DELIMITER" || peek().value != ")") {
        cout << "DEBUG: Found first argument" << endl;
        if (peek().type == "STRING_QUOTE") {
            match("STRING_QUOTE");  // Match opening quote
            if (peek().type == "STRING_LITERAL") {
                match("STRING_LITERAL");  // Match string content
            }
            match("STRING_QUOTE");  // Match closing quote
        } else {
            parse_expression();  // Handle other types of arguments
        }
        parse_argument_list_prime();  // Check for additional ones
    } else {
        cout << "DEBUG: Empty argument list" << endl;
    }
    cout << "DEBUG: Argument list parsing completed" << endl;
}

void parse_argument_list_prime(){
    cout << "\nDEBUG: Starting argument list prime parsing" << endl;
    if(peek().type == "DELIMITER" && peek().value == ","){
        cout << "DEBUG: Found additional argument" << endl;
        match("DELIMITER");  // Comma
        parse_expression();
        parse_argument_list_prime();
    } else {
        cout << "DEBUG: No more arguments" << endl;
    }
    cout << "DEBUG: Argument list prime parsing completed" << endl;
}

void parse_statement_list(){
    cout << "\nDEBUG: Starting statement list parsing" << endl;
    if(peek().type == "IDENTIFIER" || peek().value == "return" || peek().value == "if" || 
       peek().value == "while" || peek().value == "try" || peek().type == "NEWLINE"){
        cout << "DEBUG: Found valid statement" << endl;
        parse_statement();
        parse_statement_list();
    } else {
        cout << "DEBUG: End of statement list" << endl;
    }
    cout << "DEBUG: Statement list parsing completed" << endl;
}

void parse_expression() {
    cout << "\nDEBUG: Starting expression parsing" << endl;
    cout << "DEBUG: Current token in expression - Type: " << peek().type
         << ", Value: '" << peek().value << "'" << endl;

    parse_bool_term();
    parse_bool_expr_prime();

    // Handle inline if-else
    if (peek().type == "KEYWORD" && peek().value == "if") {
        parse_inline_if_else();
    }

    cout << "DEBUG: Expression parsing completed" << endl;
}

void parse_bool_expr_prime(){
    cout << "DEBUG: Parsing boolean expression prime" << endl;
    if(peek().type == "OPERATOR" && peek().value == "or"){
        cout << "DEBUG: Found 'or' operator" << endl;
        match("OPERATOR");
        parse_bool_term();
        parse_bool_expr_prime();
    }
    cout << "DEBUG: Boolean expression prime parsing completed" << endl;
}

void parse_bool_term(){
    cout << "DEBUG: Starting boolean term parsing" << endl;
    cout << "DEBUG: Current token in bool_term - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    parse_bool_factor();
    parse_bool_term_prime();
    cout << "DEBUG: Boolean term parsing completed" << endl;
}

void parse_bool_term_prime(){
    cout << "DEBUG: Parsing boolean term prime" << endl;
    if(peek().type == "OPERATOR" && peek().value == "and"){
        cout << "DEBUG: Found 'and' operator" << endl;
        match("OPERATOR");
        parse_bool_factor();
        parse_bool_term_prime();
    }
    cout << "DEBUG: Boolean term prime parsing completed" << endl;
}

void parse_bool_factor(){
    cout << "DEBUG: Starting boolean factor parsing" << endl;
    cout << "DEBUG: Current token in bool_factor - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    if(peek().type == "OPERATOR" && peek().value == "not"){
        cout << "DEBUG: Found 'not' operator" << endl;
        match("OPERATOR");
        parse_bool_factor();
    }
    else{
        parse_rel_expr();
    }
    cout << "DEBUG: Boolean factor parsing completed" << endl;
}

void parse_rel_expr(){
    cout << "DEBUG: Starting relational expression parsing" << endl;
    cout << "DEBUG: Current token in rel_expr - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    parse_arith_expr();
    if(peek().type == "OPERATOR" && (peek().value == ">" || peek().value == "<" || 
       peek().value == "==" || peek().value == "!=" || peek().value == ">=" || 
       peek().value == "<=")){
        cout << "DEBUG: Found relational operator" << endl;
        parse_rel_op();
        parse_arith_expr();
    }
    cout << "DEBUG: Relational expression parsing completed" << endl;
}

void parse_rel_op(){
    cout << "DEBUG: Parsing relational operator" << endl;
    if(peek().type == "OPERATOR" && (peek().value == ">" || peek().value == "<" || 
       peek().value == "==" || peek().value == "!=" || peek().value == ">=" || 
       peek().value == "<=")){
        match("OPERATOR");
    }
    else{
        cout << "Syntax error: expected relational operator but found " << peek().type << endl;
        exit(1);
    }
}

void parse_arith_expr(){
    cout << "DEBUG: Starting arithmetic expression parsing" << endl;
    cout << "DEBUG: Current token in arith_expr - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    parse_term();
    parse_arith_expr_prime();
    cout << "DEBUG: Arithmetic expression parsing completed" << endl;
}

void parse_arith_expr_prime(){
    cout << "DEBUG: Parsing arithmetic expression prime" << endl;
    cout << "DEBUG: Current token - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    
    if(peek().type == "OPERATOR" && (peek().value == "+" || peek().value == "-")){
        cout << "DEBUG: Found addition/subtraction operator" << endl;
        match("OPERATOR");
        parse_term();
        parse_arith_expr_prime();
    }
    cout << "DEBUG: Arithmetic expression prime parsing completed" << endl;
}

void parse_term(){
    cout << "DEBUG: Starting term parsing" << endl;
    cout << "DEBUG: Current token in term - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    parse_factor();
    parse_term_prime();
    cout << "DEBUG: Term parsing completed" << endl;
}

void parse_term_prime(){
    cout << "DEBUG: Parsing term prime" << endl;
    cout << "DEBUG: Current token - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    
    if(peek().type == "OPERATOR" && (peek().value == "*" || peek().value == "/")){
        cout << "DEBUG: Found multiplication/division operator" << endl;
        match("OPERATOR");
        parse_factor();
        parse_term_prime();
    }
    cout << "DEBUG: Term prime parsing completed" << endl;
}

void parse_factor(){
    cout << "DEBUG: Starting factor parsing" << endl;
    cout << "DEBUG: Current token in factor - Type: " << peek().type 
         << ", Value: '" << peek().value << "'" << endl;
    
    if(peek().value == "("){
        cout << "DEBUG: Found opening parenthesis" << endl;
        match("DELIMITER");
        parse_expression();
        match("DELIMITER");
    }
    else if(peek().type == "IDENTIFIER"){
        cout << "DEBUG: Found identifier" << endl;
        match("IDENTIFIER");
    }
    else if (peek().value == "{") {
    cout << "DEBUG: Found dictionary literal" << endl;
    parse_dict_literal();
    }
    else if (peek().type == "STRING") {
    cout << "DEBUG: Found string literal" << endl;
    match("STRING");
    }

    else if(peek().type == "NUMBER"){
        cout << "DEBUG: Found number" << endl;
        match("NUMBER");
    }
    else if (peek().type == "STRING_QUOTE") {
        cout << "DEBUG: Found string literal" << endl;
        match("STRING_QUOTE");  // Match opening quote
        if (peek().type == "STRING_LITERAL") {
            match("STRING_LITERAL");  // Match string content
        }
        match("STRING_QUOTE");  // Match closing quote
    }
    else if (peek().value == "[") {
    cout << "DEBUG: Found list literal" << endl;
    parse_list_literal();
    }
    else{
        cout << "DEBUG: Unexpected token in factor" << endl;
        cout << "Syntax error: expected factor but found " << peek().type 
             << " with value '" << peek().value << "'" << endl;
        exit(1);
    }
    cout << "DEBUG: Factor parsing completed" << endl;
}

void parse_augmented_assignment() {
    cout << "\nDEBUG: Starting augmented assignment parsing" << endl;

    match("IDENTIFIER");

    if (peek().type == "OPERATOR" && (
        peek().value == "+=" || peek().value == "-=" ||
        peek().value == "*=" || peek().value == "/=" ||
        peek().value == "%=" || peek().value == "//=")) 
    {
        match("OPERATOR");
    } else {
        cout << "Syntax error: expected augmented assignment operator but found '"
             << peek().value << "' of type " << peek().type << endl;
        exit(1);
    }

    parse_expression();
    match("NEWLINE");

    cout << "DEBUG: Augmented assignment parsing completed" << endl;
}

void parse_for_stmt() {
    cout << "\nDEBUG: Starting for-loop parsing" << endl;

    match("KEYWORD");         // 'for'
    match("IDENTIFIER");      // loop variable
    match("KEYWORD");         // 'in'
    parse_list_literal();         // iterable expression
    match("OPERATOR");        // ':'
    match("NEWLINE");
    match("INDENT");
    parse_loop_statement_list();
    match("DEDENT");

    cout << "DEBUG: For-loop parsing completed" << endl;
}

void parse_list_literal() {
    cout << "\nDEBUG: Starting list literal parsing" << endl;

    match("DELIMITER");  // '['

    if (peek().value != "]") {
        cout << "DEBUG: Parsing first list item" << endl;
        parse_expression();
        parse_list_items_prime();
    } else {
        cout << "DEBUG: Empty list" << endl;
    }

    match("DELIMITER");  // ']'

    cout << "DEBUG: List literal parsing completed" << endl;
}

void parse_list_items_prime() {
    cout << "DEBUG: Parsing list items prime" << endl;

    if (peek().type == "DELIMITER" && peek().value == ",") {
        match("DELIMITER");  // ','
        parse_expression();
        parse_list_items_prime();
    } else {
        cout << "DEBUG: No more list items" << endl;
    }
}

void parse_func_def() {
    cout << "\nDEBUG: Starting function definition parsing" << endl;

    match("KEYWORD");         // 'def'
    match("IDENTIFIER");      // function name
    match("DELIMITER");       // '('
    parse_param_list();
    match("DELIMITER");       // ')'

    // Optional return type
    if (peek().type == "OPERATOR" && peek().value == "->") {
        match("OPERATOR");
        parse_type();
    }

    match("OPERATOR");        // ':'

    // Detect if it's a single-line body
    if (peek().type != "NEWLINE") {
        cout << "DEBUG: Detected single-line function definition" << endl;
        parse_statement();  // just one statement (like return, assignment, etc.)
    } else {
        // Multiline function
        match("NEWLINE");
        match("INDENT");
        parse_statement_list();
        match("DEDENT");
    }

    cout << "DEBUG: Function definition parsing completed" << endl;
}

void parse_param_list() {
    cout << "DEBUG: Starting parameter list parsing" << endl;

    if (peek().type == "IDENTIFIER") {
        parse_param();

        while (peek().type == "DELIMITER" && peek().value == ",") {
            match("DELIMITER");
            parse_param();
        }
    }

    cout << "DEBUG: Parameter list parsing completed" << endl;
}

void parse_param() {
    match("IDENTIFIER");

    if (peek().type == "OPERATOR" && peek().value == "=") {
        match("OPERATOR");
        parse_expression();
    }
}

void parse_type() {
    if (peek().type == "KEYWORD" && 
        (peek().value == "int" || peek().value == "float" || 
         peek().value == "str" || peek().value == "bool" || 
         peek().value == "None")) 
    {
        match("KEYWORD");
    } else {
        cout << "Syntax error: expected type but found " << peek().type 
             << " with value '" << peek().value << "'" << endl;
        exit(1);
    }
}

void parse_import_stmt() {
    cout << "\nDEBUG: Starting import statement parsing" << endl;

    if (peek().value == "import") {
        match("KEYWORD");  // 'import'
        parse_import_item();
        parse_import_tail();
    } 
    else if (peek().value == "from") {
        match("KEYWORD");        // 'from'
        match("IDENTIFIER");     // module name
        match("KEYWORD");        // 'import'
        parse_import_item();
        parse_import_tail();
    } 
    else {
        cout << "Syntax error: expected 'import' or 'from'" << endl;
        exit(1);
    }

    match("NEWLINE");
    cout << "DEBUG: Import statement parsing completed" << endl;
}


void parse_import_item() {
    if (peek().type == "IDENTIFIER") {
        match("IDENTIFIER");
        parse_import_alias_opt();
    } else {
        cout << "Syntax error: expected module name in import" << endl;
        exit(1);
    }
}


void parse_import_tail() {
    while (peek().type == "DELIMITER" && peek().value == ",") {
        match("DELIMITER");      // ','
        parse_import_item();
    }
}

void parse_import_alias_opt() {
    if (peek().value == "as") {
        match("KEYWORD");        // 'as'
        if (peek().type == "IDENTIFIER") {
            match("IDENTIFIER");  // alias
        } else {
            cout << "Syntax error: expected alias after 'as'" << endl;
            exit(1);
        }
    } else {
        cout << "DEBUG: No alias in import" << endl;
    }
}

void parse_dict_literal() {
    cout << "\nDEBUG: Starting dictionary literal parsing" << endl;

    match("DELIMITER");  // '{'

    if (peek().value != "}") {
        parse_dict_pair();
        parse_dict_items_prime();
    } else {
        cout << "DEBUG: Empty dictionary" << endl;
    }

    match("DELIMITER");  // '}'

    cout << "DEBUG: Dictionary literal parsing completed" << endl;
}

void parse_dict_items_prime() {
    while (peek().type == "DELIMITER" && peek().value == ",") {
        match("DELIMITER");
        parse_dict_pair();
    }
}

void parse_dict_pair() {
    cout << "DEBUG: Parsing dictionary key (string)" << endl;
    parse_string_key();

    if (peek().type == "OPERATOR" && peek().value == ":") {
        match("OPERATOR");  // ':'
        parse_expression();
    } else {
        cout << "Syntax error: expected ':' in dictionary pair" << endl;
        exit(1);
    }
}


void parse_loop_statement_list() {
    cout << "DEBUG: Starting loop statement list" << endl;

    while (peek().type != "DEDENT" && peek().type != "END_OF_FILE") {
        parse_loop_statement();
    }

    cout << "DEBUG: Completed loop statement list" << endl;
}

void parse_loop_statement() {
    if (peek().value == "break") {
        cout << "DEBUG: Found 'break' inside loop" << endl;
        match("KEYWORD");
        match("NEWLINE");
    }
    else if (peek().value == "continue") {
        cout << "DEBUG: Found 'continue' inside loop" << endl;
        match("KEYWORD");
        match("NEWLINE");
    }
    else {
        parse_statement();  // fall back to general statement parsing
    }
}

void parse_inline_if_else() {
    cout << "\nDEBUG: Starting inline if/else expression parsing" << endl;
    
    // We've already parsed the first expression before 'if'
    match("KEYWORD");  // match 'if'
    parse_expression();   // parse condition
    
    match("KEYWORD");  // match 'else'
    parse_expression();  // parse expression after else

    cout << "DEBUG: Inline if/else expression parsing completed" << endl;
}

void parse_string_key() {
    if (peek().type == "STRING_QUOTE") {
        match("STRING_QUOTE");         // opening quote
        if (peek().type == "STRING_LITERAL") {
            match("STRING_LITERAL");   // string content
        } else {
            cout << "Syntax error: expected string literal inside quotes" << endl;
            exit(1);
        }
        if (peek().type == "STRING_QUOTE") {
            match("STRING_QUOTE");     // closing quote
        } else {
            cout << "Syntax error: expected closing quote" << endl;
            exit(1);
        }
    } else {
        cout << "Syntax error: expected opening quote for string key" << endl;
        exit(1);
    }
}

void parse_class_def() {
    cout << "\nDEBUG: Starting class definition parsing" << endl;

    match("KEYWORD");        // 'class'
    match("IDENTIFIER");     // class name
    parse_class_inheritance_opt();
    match("OPERATOR");       // ':'
    match("NEWLINE");
    match("INDENT");
    parse_statement();
    match("DEDENT");

    cout << "DEBUG: Class definition parsing completed" << endl;
}

void parse_class_inheritance_opt() {
    if (peek().value == "(") {
        match("DELIMITER");      // '('
        match("IDENTIFIER");     // base class
        match("DELIMITER");      // ')'
    } else {
        cout << "DEBUG: No base class (inheritance) specified" << endl;
    }
}

//TRY CATCH BLOCKS
void parse_try_stmt() {
    cout << "\nDEBUG: Starting try statement parsing" << endl;
    match("KEYWORD");  // 'try'
    match("OPERATOR"); // ':'
    match("NEWLINE");
    match("INDENT");
    parse_statement_list();
    match("DEDENT");
    parse_except_clauses();
    parse_finally_clause();
    cout << "DEBUG: Try statement parsing completed" << endl;
}

void parse_except_clauses() {
    cout << "\nDEBUG: Starting except clauses parsing" << endl;
    while (peek().value == "except") {
        parse_except_clause();
    }
    cout << "DEBUG: Except clauses parsing completed" << endl;
}

void parse_except_clause() {
    cout << "\nDEBUG: Starting except clause parsing" << endl;
    match("KEYWORD");  // 'except'
    
    // Optional exception type
    if (peek().type != "OPERATOR" || peek().value != ":") {
        parse_expression();
    }
    
    // Optional 'as' identifier
    if (peek().value == "as") {
        match("KEYWORD");  // 'as'
        match("IDENTIFIER");
    }
    
    match("OPERATOR"); // ':'
    match("NEWLINE");
    match("INDENT");
    parse_statement_list();
    match("DEDENT");
    cout << "DEBUG: Except clause parsing completed" << endl;
}

void parse_finally_clause() {
    cout << "\nDEBUG: Checking for finally clause" << endl;
    if (peek().value == "finally") {
        cout << "DEBUG: Found finally clause" << endl;
        match("KEYWORD");  // 'finally'
        match("OPERATOR"); // ':'
        match("NEWLINE");
        match("INDENT");
        parse_statement_list();
        match("DEDENT");
    } else {
        cout << "DEBUG: No finally clause found" << endl;
    }
    cout << "DEBUG: Finally clause parsing completed" << endl;
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
    parse_program();
    cout << "DEBUG: Parser completed successfully" << endl;

    return 0;
}