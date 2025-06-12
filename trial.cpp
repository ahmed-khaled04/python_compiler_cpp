#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace std;

struct SymbolEntry {
    int id;
    vector<int> lines;
    string type = "unknown";  // Data type
    string value = "undefined";  // Stores the assigned value
    string scope = "global";     // Track variable scope
};

// Enhanced Python keywords with type hints
const unordered_set<string> KEYWORDS = {
    "False", "None", "True", "and", "as", "assert", "async", "await",
    "break", "class", "continue", "def", "del", "elif", "else", "except",
    "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
    "try", "while", "with", "yield", "int", "str", "float", "bool",
    "list", "dict", "tuple", "set"
};

// Enhanced Python operators
const unordered_set<string> OPERATORS = {
    "+", "-", "*", "/", "%", "**", "//", "=", "+=", "-=", "*=", "/=",
    "%=", "**=", "//=", "==", "!=", "<", ">", "<=", ">=", "&", "|",
    "^", "~", "<<", ">>", "and", "or", "not", "is", "in", "not in", 
    "is not", ":="  // Walrus operator
};

// Enhanced Python delimiters
const unordered_set<string> DELIMITERS = {
    "(", ")", "[", "]", "{", "}", ",", ":", ".", ";", "@", "->"
};

struct Token {
    string type;
    string value;
    int line;
    int column;  // Added column position for better error reporting
};

// State machine states
enum class State {
    START,
    IN_IDENTIFIER,
    IN_NUMBER,
    IN_OPERATOR,
    IN_STRING,
    IN_COMMENT,
    IN_MULTILINE_STRING,
    IN_FSTRING,
    IN_COMPLEX_NUMBER
};

// Helper functions
bool isKeyword(const string& str) {
    return KEYWORDS.find(str) != KEYWORDS.end();
}

bool isOperator(const string& str) {
    return OPERATORS.find(str) != OPERATORS.end();
}

bool isDelimiter(const string& str) {
    return DELIMITERS.find(str) != DELIMITERS.end();
}

bool isIdentifier(const string& str) {
    if (str.empty() || isdigit(str[0])) return false;
    for (char c : str) {
        if (!isalnum(c) && c != '_') return false;
    }
    return true;
}

bool isNumber(const string& str) {
    bool hasDecimal = false;
    bool isHex = (str.size() > 2 && str[0] == '0' && tolower(str[1]) == 'x');
    bool isBinary = (str.size() > 2 && str[0] == '0' && tolower(str[1]) == 'b');
    bool isOctal = (str.size() > 2 && str[0] == '0' && tolower(str[1]) == 'o');
    
    for (size_t i = 0; i < str.size(); i++) {
        char c = str[i];
        
        if (isHex) {
            if (i < 2) continue; // Skip 0x prefix
            if (!isxdigit(c)) return false;
        } else if (isBinary) {
            if (i < 2) continue; // Skip 0b prefix
            if (c != '0' && c != '1') return false;
        } else if (isOctal) {
            if (i < 2) continue; // Skip 0o prefix
            if (c < '0' || c > '7') return false;
        } else if (c == '.') {
            if (hasDecimal) return false;
            hasDecimal = true;
        } else if (!isdigit(c)) {
            return false;
        }
    }
    return !str.empty();
}

vector<Token> tokenize(const string& source) {
    vector<Token> tokens;
    string currentToken;
    int lineNumber = 1;
    int columnNumber = 1;
    State state = State::START;
    char stringQuote = '\0';
    bool escapeNext = false;
    bool potentialMultilineComment = true;
    string lastTokenType;
    vector<string> scopeStack;  // Track function/class scopes

    auto flushCurrentToken = [&]() {
        if (!currentToken.empty()) {
            string type;
            if (state == State::IN_IDENTIFIER) {
                if (isKeyword(currentToken)) type = "KEYWORD";
                else if (currentToken == "None") type = "NONE";
                else type = "IDENTIFIER";
            }
            else if (state == State::IN_NUMBER || state == State::IN_COMPLEX_NUMBER) {
                type = "NUMBER";
            }
            else if (state == State::IN_OPERATOR) type = "OPERATOR";
            
            if (!type.empty()) {
                tokens.push_back({type, currentToken, lineNumber, columnNumber - (int)currentToken.length()});
                lastTokenType = type;
            }
            currentToken.clear();
        }
    };

    for (size_t i = 0; i < source.size(); i++, columnNumber++) {
        char c = source[i];
        char nextChar = (i + 1 < source.size()) ? source[i + 1] : '\0';
        
        // Handle newlines for line counting
        if (c == '\n') {
            lineNumber++;
            columnNumber = 0;
        }

        // State machine transitions
        switch (state) {
            case State::START:
                if (isspace(c)) {
                    continue;
                }
                else if (isalpha(c) || c == '_') {
                    state = State::IN_IDENTIFIER;
                    currentToken += c;
                }
                else if (isdigit(c)) {
                    state = State::IN_NUMBER;
                    currentToken += c;
                }
                else if (c == '\'' || c == '"') {
                    // Check for multi-line string or f-string
                    if (i + 2 < source.size() && source[i+1] == c && source[i+2] == c) {
                        state = State::IN_MULTILINE_STRING;
                        stringQuote = c;
                        i += 2; // Skip the next two quotes
                        columnNumber += 2;
                        currentToken = "'''";
                    } else {
                        state = State::IN_STRING;
                        stringQuote = c;
                        currentToken += c;
                    }
                }
                else if (c == '#' && nextChar == '#') {
                    // Docstring comment
                    state = State::IN_COMMENT;
                    i++; // Skip next #
                    columnNumber++;
                }
                else if (c == '#') {
                    state = State::IN_COMMENT;
                }
                else if (c == '-' && i + 1 < source.size() && isdigit(nextChar)) {
                    if (lastTokenType.empty() || lastTokenType == "OPERATOR" || lastTokenType == "DELIMITER") {
                        state = State::IN_NUMBER;
                        currentToken += c; // start of a negative number
                    } else {
                        state = State::IN_OPERATOR;
                        currentToken += c;
                    }
                }
                else if (isOperator(string(1, c))) {
                    // Check for multi-character operators
                    state = State::IN_OPERATOR;
                    currentToken += c;
                    if (c == '=' || c == '(') potentialMultilineComment = false;
                }
                else if (isDelimiter(string(1, c))) {
                    tokens.push_back({"DELIMITER", string(1, c), lineNumber, columnNumber});
                }
                break;

            case State::IN_IDENTIFIER:
                if (isalnum(c) || c == '_') {
                    currentToken += c;
                } else {
                    flushCurrentToken();
                    state = State::START;
                    i--; // Reprocess this character
                    columnNumber--;
                }
                break;

            case State::IN_NUMBER:
                if (isdigit(c) || c == '.' || tolower(c) == 'e') {
                    currentToken += c;
                    // Check for complex numbers (e.g., 1j, 2.5j)
                    if (tolower(c) == 'e' && nextChar == '+' || nextChar == '-') {
                        currentToken += nextChar;
                        i++;
                        columnNumber++;
                    }
                } else if (tolower(c) == 'j') {
                    currentToken += c;
                    state = State::IN_COMPLEX_NUMBER;
                } else {
                    flushCurrentToken();
                    state = State::START;
                    i--; // Reprocess this character
                    columnNumber--;
                }
                break;

            case State::IN_COMPLEX_NUMBER:
                flushCurrentToken();
                state = State::START;
                i--; // Reprocess this character
                columnNumber--;
                break;

            case State::IN_OPERATOR:
                if (isOperator(currentToken + c)) {
                    currentToken += c;
                } else {
                    flushCurrentToken();
                    state = State::START;
                    i--; // Reprocess this character
                    columnNumber--;
                }
                break;

            case State::IN_STRING:
                if (escapeNext) {
                    currentToken += c;
                    escapeNext = false;
                } else if (c == '\\') {
                    escapeNext = true;
                    currentToken += c;
                } else if (c == stringQuote) {
                    currentToken += c;
                    tokens.push_back({"STRING_LITERAL", currentToken, lineNumber, columnNumber - (int)currentToken.length() + 1});
                    currentToken.clear();
                    state = State::START;
                } else {
                    currentToken += c;
                }
                break;

            case State::IN_MULTILINE_STRING:
                if (c == stringQuote && i + 2 < source.size() &&
                    source[i+1] == stringQuote && source[i+2] == stringQuote) {
                    currentToken += "'''";
                    if (potentialMultilineComment) {
                        // Do Nothing
                    } else {
                        tokens.push_back({"STRING_LITERAL", currentToken, lineNumber, columnNumber - (int)currentToken.length() + 1});
                    }
                    currentToken.clear();
                    state = State::START;
                    i += 2; // Skip closing quotes
                    columnNumber += 2;
                    potentialMultilineComment = true;            
                } else {
                    currentToken += c;
                }
                break;

            case State::IN_COMMENT:
                if (c == '\n') {
                    state = State::START;
                }
                break;
        }
    }

    // Flush any remaining token
    flushCurrentToken();

    return tokens;
}

void printHorizontalLine(const vector<int>& columnWidths) {
    cout << "+";
    for (int width : columnWidths) {
        cout << "-" << string(width, '-') << "-+";
    }
    cout << endl;
}

void printTokenTable(const vector<Token>& tokens) {
    vector<int> columnWidths = {15, 20, 8, 8}; // Type, Value, Line, Column

    // Calculate column widths
    for (const auto& token : tokens) {
        columnWidths[0] = max(columnWidths[0], (int)token.type.length());
        columnWidths[1] = max(columnWidths[1], (int)token.value.length());
    }

    printHorizontalLine(columnWidths);
    
    // Print header
    cout << "| " << left << setw(columnWidths[0]) << "TOKEN TYPE" << " | "
         << setw(columnWidths[1]) << "VALUE" << " | "
         << setw(columnWidths[2]) << "LINE" << " | "
         << setw(columnWidths[3]) << "COLUMN" << " |" << endl;
    
    printHorizontalLine(columnWidths);

    // Print rows
    for (const auto& token : tokens) {
        cout << "| " << left << setw(columnWidths[0]) << token.type << " | "
             << setw(columnWidths[1]) << token.value << " | "
             << right << setw(columnWidths[2]) << token.line << " | "
             << setw(columnWidths[3]) << token.column << " |" << endl;
    }

    printHorizontalLine(columnWidths);
    cout << "Total tokens: " << tokens.size() << endl << endl;
}

void generateSymbolTable(const vector<Token>& tokens) {
    unordered_map<string, SymbolEntry> symbolTable;
    int currentId = 1;
    vector<string> scopeStack = {"global"};
    bool inFunctionDef = false;
    string currentFunction;

    // First pass: Collect all identifiers and their locations
    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];

        // Track scope changes
        if (token.value == "def" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i+1].value);
            currentFunction = tokens[i+1].value;
            inFunctionDef = true;
        }
        else if (token.value == "class" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i+1].value);
        }
        else if (token.value == "return" && inFunctionDef) {
            // Check for return type annotation (Python 3.5+)
            if (i + 1 < tokens.size() && tokens[i+1].value == "->") {
                if (i + 2 < tokens.size()) {
                    const Token& typeToken = tokens[i+2];
                    if (symbolTable.find(currentFunction) != symbolTable.end()) {
                        symbolTable[currentFunction].type = typeToken.value;
                    }
                }
            }
        }

        if (token.type == "IDENTIFIER") {
            string scope = scopeStack.back();
            string key = scope + ":" + token.value; // Create scoped identifier
            
            if (symbolTable.find(key) == symbolTable.end()) {
                symbolTable[key] = { currentId++, {token.line}, "unknown", "undefined", scope };
            } else {
                bool lineExists = false;
                for (int line : symbolTable[key].lines) {
                    if (line == token.line) {
                        lineExists = true;
                        break;
                    }
                }
                if (!lineExists) {
                    symbolTable[key].lines.push_back(token.line);
                }
            }
        }

        // Reset function definition flag after we pass the parameters
        if (inFunctionDef && token.value == ":") {
            inFunctionDef = false;
        }
    }

    // Second pass: Infer types and track values
    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];

        // Check for assignments (e.g., `x = 5`)
        if (token.type == "IDENTIFIER" && i + 1 < tokens.size() && tokens[i+1].value == "=") {
            string identifier = token.value;
            string scope = scopeStack.back();
            string key = scope + ":" + identifier;
            
            if (i + 2 < tokens.size()) {
                const Token& valueToken = tokens[i+2]; // The token after '='
                
                // Handle type annotations (Python 3.6+)
                if (i > 0 && tokens[i-1].value == ":") {
                    const Token& typeToken = tokens[i-2];
                    symbolTable[key].type = typeToken.value;
                }

                // Update type inference based on value
                if (valueToken.type == "NUMBER") {
                    symbolTable[key].type = "numeric";
                    symbolTable[key].value = valueToken.value;
                }
                else if (valueToken.type == "STRING_LITERAL") {
                    symbolTable[key].type = "string";
                    symbolTable[key].value = valueToken.value;
                }
                else if (valueToken.value == "True" || valueToken.value == "False") {
                    symbolTable[key].type = "boolean";
                    symbolTable[key].value = valueToken.value;
                }
                else if (valueToken.value == "[") {
                    symbolTable[key].type = "list";
                    symbolTable[key].value = "[]";
                }
                else if (valueToken.value == "{") {
                    symbolTable[key].type = "dict";
                    symbolTable[key].value = "{}";
                }
                else if (valueToken.type == "IDENTIFIER") {
                    // If assigned from another variable, try to inherit its type/value
                    string valueKey = scope + ":" + valueToken.value;
                    if (symbolTable.find(valueKey) != symbolTable.end()) {
                        symbolTable[key].type = symbolTable[valueKey].type;
                        symbolTable[key].value = symbolTable[valueKey].value;
                    }
                }
            }
        }

        // Check for function definitions (`def foo():`)
        if (token.value == "def" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            string key = "global:" + tokens[i+1].value;
            symbolTable[key].type = "function";
            symbolTable[key].value = "function";
            
            // Check for return type annotation (Python 3.5+)
            for (size_t j = i; j < tokens.size(); j++) {
                if (tokens[j].value == "->") {
                    if (j + 1 < tokens.size()) {
                        symbolTable[key].type += " -> " + tokens[j+1].value;
                    }
                    break;
                }
                if (tokens[j].value == ":") break;
            }
        }

        // Check for class definitions (`class Bar:`)
        if (token.value == "class" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            string key = "global:" + tokens[i+1].value;
            symbolTable[key].type = "class";
            symbolTable[key].value = "class";
        }

        // Check for type annotations (Python 3.6+)
        if (token.value == ":" && i + 1 < tokens.size() && tokens[i-1].type == "IDENTIFIER") {
            string identifier = tokens[i-1].value;
            string scope = scopeStack.back();
            string key = scope + ":" + identifier;
            
            if (i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
                symbolTable[key].type = tokens[i+1].value;
            }
        }
    }

    // Print the symbol table
    cout << "\nSYMBOL TABLE\n";
    cout << "------------\n";

    if (symbolTable.empty()) {
        cout << "No identifiers found in the code.\n";
        return;
    }

    // Calculate column widths
    vector<int> columnWidths = {5, 25, 15, 20, 15, 30}; // ID, Identifier, Type, Value, Scope, Lines

    for (const auto& entry : symbolTable) {
        string displayName = entry.first.substr(entry.first.find(':') + 1);
        columnWidths[1] = max(columnWidths[1], (int)displayName.length());
        columnWidths[2] = max(columnWidths[2], (int)entry.second.type.length());
        columnWidths[3] = max(columnWidths[3], (int)entry.second.value.length());
        columnWidths[4] = max(columnWidths[4], (int)entry.second.scope.length());
    }

    // Print table header
    printHorizontalLine(columnWidths);
    cout << "| " << left << setw(columnWidths[0]) << "ID" << " | "
         << setw(columnWidths[1]) << "IDENTIFIER" << " | "
         << setw(columnWidths[2]) << "TYPE" << " | "
         << setw(columnWidths[3]) << "VALUE" << " | "
         << setw(columnWidths[4]) << "SCOPE" << " | "
         << setw(columnWidths[5]) << "LINES" << " |" << endl;
    printHorizontalLine(columnWidths);

    // Print table rows
    for (const auto& entry : symbolTable) {
        string displayName = entry.first.substr(entry.first.find(':') + 1);
        string linesStr;
        for (size_t i = 0; i < entry.second.lines.size(); i++) {
            if (i > 0) linesStr += ", ";
            linesStr += to_string(entry.second.lines[i]);
        }

        cout << "| " << right << setw(columnWidths[0]) << entry.second.id << " | "
             << left << setw(columnWidths[1]) << displayName << " | "
             << setw(columnWidths[2]) << entry.second.type << " | "
             << setw(columnWidths[3]) << entry.second.value << " | "
             << setw(columnWidths[4]) << entry.second.scope << " | "
             << setw(columnWidths[5]) << linesStr << " |" << endl;
    }

    printHorizontalLine(columnWidths);
    cout << "Total identifiers: " << symbolTable.size() << endl << endl;
}

void printAST(const vector<Token>& tokens) {
    cout << "\nABSTRACT SYNTAX TREE (SIMPLIFIED)\n";
    cout << "--------------------------------\n";
    
    int indent = 0;
    bool inFunctionDef = false;
    bool inClassDef = false;
    
    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];
        
        // Handle indentation for blocks
        if (token.value == ":" && i + 1 < tokens.size() && tokens[i+1].line != token.line) {
            indent++;
        }
        
        // Print with proper indentation
        if (i == 0 || token.line != tokens[i-1].line) {
            cout << "\n" << string(indent * 4, ' ');
        }
        
        // Highlight important constructs
        if (token.type == "KEYWORD" && 
            (token.value == "def" || token.value == "class" || token.value == "if" || 
             token.value == "for" || token.value == "while" || token.value == "try")) {
            cout << "\033[1;32m" << token.value << "\033[0m ";  // Green for keywords
            if (token.value == "def") inFunctionDef = true;
            if (token.value == "class") inClassDef = true;
        } 
        else if (token.type == "IDENTIFIER" && (inFunctionDef || inClassDef)) {
            cout << "\033[1;33m" << token.value << "\033[0m ";  // Yellow for def/class names
            inFunctionDef = false;
            inClassDef = false;
        }
        else if (token.type == "STRING_LITERAL") {
            cout << "\033[1;35m" << token.value << "\033[0m ";  // Magenta for strings
        }
        else if (token.type == "NUMBER") {
            cout << "\033[1;34m" << token.value << "\033[0m ";  // Blue for numbers
        }
        else {
            cout << token.value << " ";
        }
        
        // Reduce indentation when block ends
        if ((token.value == "pass" || token.value == "return" || 
             token.value == "break" || token.value == "continue") &&
            i + 1 < tokens.size() && tokens[i+1].line != token.line) {
            indent = max(0, indent - 1);
        }
    }
    cout << "\n\n";
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
    }
    else if (option == 2) {
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
    }
    else {
        cerr << "Invalid option!\n";
        return 1;
    }

    vector<Token> tokens = tokenize(input);

    cout << "\nTOKENS FOUND\n";
    cout << "============\n";
    printTokenTable(tokens);

    generateSymbolTable(tokens);
    
    printAST(tokens);

    return 0;
}
