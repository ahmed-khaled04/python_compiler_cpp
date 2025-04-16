#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <iomanip>

using namespace std;

struct SymbolEntry {
    int id;
    vector<int> lines;
    string type = "unknown";  // Data type
    string value = "undefined";  // Stores the assigned value
};

// Python keywords
const unordered_set<string> KEYWORDS = {
    "False", "None", "True", "and", "as", "assert", "async", "await",
    "break", "class", "continue", "def", "del", "elif", "else", "except",
    "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
    "try", "while", "with", "yield"
};

// Python operators
const unordered_set<string> OPERATORS = {
    "+", "-", "", "/", "%", "", "//", "=", "+=", "-=", "=", "/=",
    "%=", "=", "//=", "==", "!=", "<", ">", "<=", ">=", "&", "|",
    "^", "~", "<<", ">>", "and", "or", "not", "is", ":="
};

// Python delimiters
const unordered_set<string> DELIMITERS = {
    "(", ")", "[", "]", "{", "}", ",", ":", ".", ";", "@", "..."
};

struct Token {
    string type;
    string value;
    int line;
};

// State machine states
enum class State {
    START,
    IN_IDENTIFIER,
    IN_NUMBER,
    IN_OPERATOR,
    IN_STRING,
    IN_COMMENT,
    IN_MULTILINE_STRING
};

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

bool isHexDigit(char c) {
    return isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f');
}

bool isBinaryDigit(char c) {
    return c == '0' || c == '1';
}

bool isOctalDigit(char c) {
    return c >= '0' && c <= '7';
}

bool isNumber(const string& str) {
    if (str.empty()) return false;
    
    // Check for complex numbers
    if (tolower(str.back()) == 'j') {
        if (str.length() == 1) return false;
        string realPart = str.substr(0, str.length() - 1);
        return isNumber(realPart);
    }
    
    // Check for hexadecimal
    if (str.length() > 2 && str[0] == '0' && tolower(str[1]) == 'x') {
        for (size_t i = 2; i < str.length(); i++) {
            if (!isHexDigit(str[i])) return false;
        }
        return str.length() > 2; // Must have at least one digit after 0x
    }
    
    // Check for binary
    if (str.length() > 2 && str[0] == '0' && tolower(str[1]) == 'b') {
        for (size_t i = 2; i < str.length(); i++) {
            if (!isBinaryDigit(str[i])) return false;
        }
        return str.length() > 2;
    }
    
    // Check for octal
    if (str.length() > 2 && str[0] == '0' && tolower(str[1]) == 'o') {
        for (size_t i = 2; i < str.length(); i++) {
            if (!isOctalDigit(str[i])) return false;
        }
        return str.length() > 2;
    }
    
    // Regular decimal numbers
    bool hasDecimal = false;
    bool hasExponent = false;
    bool needsExponentDigit = false;
    
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        
        if (isdigit(c)) {
            if (needsExponentDigit) needsExponentDigit = false;
            continue;
        }
        
        switch (c) {
            case '.':
                if (hasDecimal || hasExponent) return false;
                hasDecimal = true;
                break;
                
            case 'e': case 'E':
                if (hasExponent) return false;
                hasExponent = true;
                needsExponentDigit = true;
                if (i+1 < str.length() && (str[i+1] == '+' || str[i+1] == '-')) {
                    i++;
                }
                break;
                
            default:
                return false;
        }
    }
    
    return !needsExponentDigit;
}

vector<Token> tokenize(const string& source) {
    vector<Token> tokens;
    string currentToken;
    int lineNumber = 1;
    State state = State::START;
    char stringQuote = '\0';
    bool escapeNext = false;
    bool potentialMultilineComment = true;
    string lastTokenType;

    auto flushCurrentToken = [&]() {
        if (!currentToken.empty()) {
            string type;
            if (state == State::IN_IDENTIFIER) {
                if (isKeyword(currentToken)) type = "KEYWORD";
                else type = "IDENTIFIER";
            }
            else if (state == State::IN_NUMBER) type = "NUMBER";
            else if (state == State::IN_OPERATOR) type = "OPERATOR";
            
            if (!type.empty()) {
                tokens.push_back({type, currentToken, lineNumber});
                lastTokenType = type;
            }
            currentToken.clear();
        }
    };

    for (size_t i = 0; i < source.size(); i++) {
        char c = source[i];
        
        // Handle newlines for line counting
        if (c == '\n') {
            lineNumber++;
        }

        // State machine transitions
        switch (state) {
            case State::START:
                if (isspace(c)) {
                    continue;
                }
                else if (c == '.' && i + 2 < source.size() && 
                source[i+1] == '.' && source[i+2] == '.') {
                    tokens.push_back({"ELLIPSIS", "...", lineNumber});
                     i += 2; // Skip next two dots
                 }
                else if (isalpha(c) || c == '_') {
                    state = State::IN_IDENTIFIER;
                    currentToken += c;
                }
                else if (isdigit(c)) {
                    state = State::IN_NUMBER;
                    currentToken += c;
                }
                else if (c == ':') {
                    // Start operator processing (could be : or :=)
                    state = State::IN_OPERATOR;
                    currentToken += c;
                }
                else if (isOperator(string(1, c))) {
                    // Start operator processing
                    state = State::IN_OPERATOR;
                    currentToken += c;
                }
                else if (c == '\'' || c == '"') {
                    // Check for multi-line string
                    if (i + 2 < source.size() && source[i+1] == c && source[i+2] == c) {
                        state = State::IN_MULTILINE_STRING;
                        stringQuote = c;
                        i += 2; // Skip the next two quotes
                        currentToken = "'''";
                    } else {
                        state = State::IN_STRING;
                        stringQuote = c;
                        currentToken += c;
                    }
                }
                else if (c == '#') {
                    state = State::IN_COMMENT;
                }
                else if(c == '-' && i + 1 < source.size() && isdigit(source[i + 1])){
                    if(lastTokenType.empty() || lastTokenType == "OPERATOR" || lastTokenType == "DELIMITER") {
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
                    if (c == '.' && i + 2 < source.size() && 
                        source[i+1] == '.' && source[i+2] == '.') {
                         // Handled above in the special case
                    }
                    else {
                         tokens.push_back({"DELIMITER", string(1, c), lineNumber});
                    }
                }
                break;

            case State::IN_IDENTIFIER:
                if (isalnum(c) || c == '_') {
                    currentToken += c;
                } else {
                    flushCurrentToken();
                    state = State::START;
                    i--; // Reprocess this character
                }
                break;

            case State::IN_NUMBER:
                // Skip if this dot is part of ellipsis
                if (c == '.' && i + 2 < source.size() && 
                    source[i+1] == '.' && source[i+2] == '.') {
                    // Finish current number token
                if (isNumber(currentToken)) {
                    tokens.push_back({"NUMBER", currentToken, lineNumber});
                }
                currentToken.clear();
                state = State::START;
                // The ellipsis will be caught on next iteration
                i--; // Reprocess this character
                break;
            }
                // Check for valid number continuations
                if (isdigit(c) || 
                    c == '.' || 
                    tolower(c) == 'e' || 
                    (tolower(c) == 'x' && currentToken == "0") ||
                    (tolower(c) == 'b' && currentToken == "0") ||
                    (tolower(c) == 'o' && currentToken == "0") ||
                    (tolower(c) == 'j' && !currentToken.empty()) ||
                    (currentToken.size() >= 2 && 
                    currentToken[0] == '0' && 
                    tolower(currentToken[1]) == 'x' && 
                    isHexDigit(c))) {
                    
                    // Handle exponent signs
                    if (tolower(c) == 'e' && i + 1 < source.size() && 
                        (source[i+1] == '+' || source[i+1] == '-')) {
                        currentToken += c;
                        currentToken += source[i+1];
                        i++;
                    } 
                    else {
                        currentToken += c;
                    }
                } 
                else {
                    // Check if we have a valid number
                    if (isNumber(currentToken)) {
                        tokens.push_back({"NUMBER", currentToken, lineNumber});
                        currentToken.clear();
                        state = State::START;
                        i--; // Reprocess this character
                    }
                    else {
                        // Try to find valid number prefix
                        bool found = false;
                        for (size_t len = currentToken.length(); len > 0; len--) {
                            string prefix = currentToken.substr(0, len);
                            if (isNumber(prefix)) {
                                tokens.push_back({"NUMBER", prefix, lineNumber});
                                currentToken = currentToken.substr(len);
                                found = true;
                                break;
                            }
                        }
                        
                        if (!found) {
                            // No valid number found - treat as identifier
                            state = State::IN_IDENTIFIER;
                        }
                        i--; // Reprocess this character
                    }
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
                    tokens.push_back({"STRING_LITERAL", currentToken, lineNumber});
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
                    //Do Nothing
                } else {
                    tokens.push_back({"STRING_LITERAL", currentToken, lineNumber});
                }
                currentToken.clear();
                state = State::START;
                i += 2; // Skip closing quotes
                potentialMultilineComment = true;            
                }   
                else {
                currentToken += c;
                }
                break;

            case State::IN_COMMENT:
                if (c == '\n') {
                    state = State::START;
                }
                break;
            case State::IN_OPERATOR:
                // Special handling for walrus operator
                if (currentToken == ":" && c == '=') {
                    currentToken += c;
                    tokens.push_back({"OPERATOR", currentToken, lineNumber});
                    currentToken.clear();
                    state = State::START;
                }
                // Check if we can extend the current operator
                else if (isOperator(currentToken + c)) {
                    currentToken += c;
                }
                else {
                    // Current operator is complete
                    tokens.push_back({"OPERATOR", currentToken, lineNumber});
                    currentToken.clear();
                    state = State::START;
                    i--; // Reprocess this character
                }
            break;
        }
    }

    // Flush any remaining token
    flushCurrentToken();

    return tokens;
}

void printHorizontalLine(int tokenColWidth, int valueColWidth, int lineColWidth) {
    cout << "+-" << string(tokenColWidth, '-') << "-+-" << string(valueColWidth, '-')
        << "-+-" << string(lineColWidth, '-') << "-+" << endl;
}

void printTokenTable(const vector<Token>& tokens) {
    int tokenColWidth = 15;
    int valueColWidth = 20;
    int lineColWidth = 5;

    for (const auto& token : tokens) {
        if (token.type.length() > tokenColWidth) tokenColWidth = token.type.length();
        if (token.value.length() > valueColWidth) valueColWidth = token.value.length();
    }

    printHorizontalLine(tokenColWidth, valueColWidth, lineColWidth);
    cout << "| " << left << setw(tokenColWidth) << "TOKEN TYPE" << " | "
        << setw(valueColWidth) << "VALUE" << " | "
        << setw(lineColWidth) << "LINE" << " |" << endl;
    printHorizontalLine(tokenColWidth, valueColWidth, lineColWidth);

    for (const auto& token : tokens) {
        cout << "| " << left << setw(tokenColWidth) << token.type << " | "
            << setw(valueColWidth) << token.value << " | "
            << right << setw(lineColWidth) << token.line << " |" << endl;
    }

    printHorizontalLine(tokenColWidth, valueColWidth, lineColWidth);
    cout << "Total tokens: " << tokens.size() << endl << endl;
}

void generateSymbolTable(const vector<Token>& tokens) {


    unordered_map<string, SymbolEntry> symbolTable;
    int currentId = 1;

    // First pass: Collect all identifiers and their locations
    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];

        if (token.type == "IDENTIFIER") {
            if (symbolTable.find(token.value) == symbolTable.end()) {
                symbolTable[token.value] = { currentId++, {token.line}, "unknown", "undefined" };
            } else {
                bool lineExists = false;
                for (int line : symbolTable[token.value].lines) {
                    if (line == token.line) {
                        lineExists = true;
                        break;
                    }
                }
                if (!lineExists) {
                    symbolTable[token.value].lines.push_back(token.line);
                }
            }
        }
    }

    // Second pass: Infer types and track values
    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];

        // Check for assignments (e.g., x = 5)
        if (token.type == "IDENTIFIER" && i + 1 < tokens.size() && tokens[i+1].value == "=") {
            string identifier = token.value;
            const Token& valueToken = tokens[i+2]; // The token after '='

            // Update type inference
            if (valueToken.type == "NUMBER") {
                symbolTable[identifier].type = "numeric";
                symbolTable[identifier].value = valueToken.value;
            }
            else if (valueToken.type == "STRING_LITERAL") {
                symbolTable[identifier].type = "string";
                symbolTable[identifier].value = valueToken.value;
            }
            else if (valueToken.value == "True" || valueToken.value == "False") {
                symbolTable[identifier].type = "boolean";
                symbolTable[identifier].value = valueToken.value;
            }
            else if (valueToken.value == "[") {
                symbolTable[identifier].type = "list";
                symbolTable[identifier].value = "[]";
            }
            else if (valueToken.value == "{") {
                symbolTable[identifier].type = "dict";
                symbolTable[identifier].value = "{}";
            }
            else if (valueToken.type == "IDENTIFIER") {
                // If assigned from another variable, try to inherit its type/value
                if (symbolTable.find(valueToken.value) != symbolTable.end()) {
                    symbolTable[identifier].type = symbolTable[valueToken.value].type;
                    symbolTable[identifier].value = symbolTable[valueToken.value].value;
                }
            }
        }

        // Check for function definitions (def foo():)
        if (token.value == "def" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            symbolTable[tokens[i+1].value].type = "function";
            symbolTable[tokens[i+1].value].value = "function";
        }

        // Check for class definitions (class Bar:)
        if (token.value == "class" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            symbolTable[tokens[i+1].value].type = "class";
            symbolTable[tokens[i+1].value].value = "class";
        }
    }

    // Print the symbol table with the new "VALUE" column
    cout << "\nSYMBOL TABLE\n";
    cout << "------------\n";

    if (symbolTable.empty()) {
        cout << "No identifiers found in the code.\n";
        return;
    }

    // Calculate column widths
    int idColWidth = 5;
    int nameColWidth = 20;
    int typeColWidth = 15;
    int valueColWidth = 20;
    int linesColWidth = 30;

    for (const auto& entry : symbolTable) {
        if (entry.first.length() > nameColWidth) nameColWidth = entry.first.length();
        if (entry.second.type.length() > typeColWidth) typeColWidth = entry.second.type.length();
        if (entry.second.value.length() > valueColWidth) valueColWidth = entry.second.value.length();
    }

    // Print table header
    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
         << "-+-" << string(typeColWidth, '-') << "-+-" << string(valueColWidth, '-')
         << "-+-" << string(linesColWidth, '-') << "-+" << endl;
    cout << "| " << left << setw(idColWidth) << "ID" << " | "
         << setw(nameColWidth) << "IDENTIFIER" << " | "
         << setw(typeColWidth) << "TYPE" << " | "
         << setw(valueColWidth) << "VALUE" << " | "
         << setw(linesColWidth) << "LINES" << " |" << endl;
    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
         << "-+-" << string(typeColWidth, '-') << "-+-" << string(valueColWidth, '-')
         << "-+-" << string(linesColWidth, '-') << "-+" << endl;

    // Print table rows
    for (const auto& entry : symbolTable) {
        string linesStr;
        for (size_t i = 0; i < entry.second.lines.size(); i++) {
            if (i > 0) linesStr += ", ";
            linesStr += to_string(entry.second.lines[i]);
        }

        cout << "| " << right << setw(idColWidth) << entry.second.id << " | "
             << left << setw(nameColWidth) << entry.first << " | "
             << setw(typeColWidth) << entry.second.type << " | "
             << setw(valueColWidth) << entry.second.value << " | "
             << setw(linesColWidth) << linesStr << " |" << endl;
    }

    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
         << "-+-" << string(typeColWidth, '-') << "-+-" << string(valueColWidth, '-')
         << "-+-" << string(linesColWidth, '-') << "-+" << endl;
    cout << "Total identifiers: " << symbolTable.size() << endl << endl;
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

    return 0;
}
