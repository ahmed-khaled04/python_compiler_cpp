#include "lexical_analyzer.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <iomanip>
#include <algorithm>

using namespace std;

SymbolEntry* getEntryByScope(const unordered_map<pair<string, string>, SymbolEntry, PairHash>& symbolTable, const string& tokenValue, const string& scope) {
    for (const auto& entry : symbolTable) {
        const auto& key = entry.first;
        const auto& symbol = entry.second;

        if (key.first == tokenValue && key.second == scope) { 
            return const_cast<SymbolEntry*>(&symbol); 
        }
    }

    return nullptr; 
}

SymbolEntry* getFirstEntryForValue(const unordered_map<pair<string, string>, SymbolEntry, PairHash>& symbolTable, 
    const vector<string>& scopeStack, 
    const string& tokenValue) {
    // Iterate through the scopeStack from the last (current scope) to the first (global scope)
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        string currentScope = *it; // Get the current scope
        pair<string, string> key = {tokenValue, currentScope}; // Create the composite key

        // Check if the key exists in the symbol table
        auto entry = symbolTable.find(key);
        if (entry != symbolTable.end()) {
            return const_cast<SymbolEntry*>(&entry->second); // Return the matching entry
        }
    }

    return nullptr; // Return nullptr if no matching entry is found
}

string getHighestScope(const unordered_map<pair<string, string>, SymbolEntry, PairHash>& symbolTable, const string& tokenValue) {
    string highestScope = "global"; // Default to global if no higher scope is found

    for (const auto& entry : symbolTable) {
        const auto& key = entry.first;
        const auto& scope = key.second;


        if (key.first == tokenValue) {
            if (scope != "global") {
                highestScope = scope;
            }
        }
    }

    return highestScope;
}


string getLastScope(const unordered_map<pair<string, string>, SymbolEntry, PairHash>& symbolTable, 
    const vector<pair<string, string>>& symbolOrder, 
    const string& tokenValue) {
    // Iterate through the symbolOrder in reverse to find the last occurrence of the token
    for (auto it = symbolOrder.rbegin(); it != symbolOrder.rend(); ++it) {
        const auto& key = *it;
        if (key.first == tokenValue) {
            return key.second; // Return the scope of the last occurrence
        }
    }
    return "global"; // Default to global if the token is not found
}

bool isIdentifierInSymbolTable(const unordered_map<pair<string, string>, SymbolEntry, PairHash>& symbolTable, const string& identifier) {
    for (const auto& entry : symbolTable) {
        if (entry.first.first == identifier) {
            return true;
        }
    }
    return false;
}

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
    "^", "~", "<<", ">>", "and", "or", "not", "is", ":=","*","**","*="
};

// Python delimiters
const unordered_set<string> DELIMITERS = {
    "(", ")", "[", "]", "{", "}", ",", ":", ".", ";", "@", "..."
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
    IN_MULTILINE_COMMENT
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

bool isOperatorChar(char c) {
    return (c=='+' || c=='-' || c=='*' || c=='/' || c=='%' || c=='=' || c=='<' || c=='>' || c=='!' || c=='&' || c=='|' || c=='^' || c=='~');
}

bool isNumber(const std::string& str) {
    if (str.empty()) {
        return false;
    }

    // Allow leading minus or plus sign
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        if (str.length() == 1) return false; // Just '-' or '+' is not a number
        start = 1;
    }

    // Complex number check
    if (tolower(str.back()) == 'j') {
        if (str.length() == 1 + start) {
            return false;
        }
        std::string realPart = str.substr(0, str.length() - 1);
        return isNumber(realPart);
    }

    // Hexadecimal
    if (str.length() > 2 + start && str[start] == '0' && tolower(str[start+1]) == 'x') {
        for (size_t i = start + 2; i < str.length(); i++) {
            if (!isHexDigit(str[i])) {
                return false;
            }
        }
        return str.length() > 2 + start;
    }

    // Binary
    if (str.length() > 2 + start && str[start] == '0' && tolower(str[start+1]) == 'b') {
        for (size_t i = start + 2; i < str.length(); i++) {
            if (!isBinaryDigit(str[i])) {
                return false;
            }
        }
        return str.length() > 2 + start;
    }

    // Octal
    if (str.length() > 2 + start && str[start] == '0' && tolower(str[start+1]) == 'o') {
        for (size_t i = start + 2; i < str.length(); i++) {
            if (!isOctalDigit(str[i])) {
                return false;
            }
        }
        return str.length() > 2 + start;
    }

    // Decimal / Float / Scientific
    bool hasDecimal = false;
    bool hasExponent = false;
    bool hasDigit = false;
    bool hasDigitAfterExponent = true;

    for (size_t i = start; i < str.length(); i++) {
        char c = str[i];

        if (isdigit(c)) {
            hasDigit = true;
            if (hasExponent) hasDigitAfterExponent = true;
        } else if (c == '.') {
            if (hasDecimal || hasExponent) {
                return false;
            }
            hasDecimal = true;
        } else if (c == 'e' || c == 'E') {
            if (hasExponent || !hasDigit) {
                return false;
            }
            hasExponent = true;
            hasDigitAfterExponent = false;

            // Look ahead for optional sign
            if (i + 1 < str.length() && (str[i + 1] == '+' || str[i + 1] == '-')) {
                i++;
            }

            // If exponent is at the end or next is not a digit
            if (i + 1 >= str.length() || !isdigit(str[i + 1])) {
                return false;
            }
        } else if (c == '+' || c == '-') {
            // Only valid immediately after 'e' or 'E'
            if (i == start || !(str[i - 1] == 'e' || str[i - 1] == 'E')) {
                return false;
            }
        } else {
            return false;
        }
    }

    return hasDigit && (!hasExponent || hasDigitAfterExponent);
}

vector<Token> tokenize(const string& source) { 
    vector<Token> tokens;
    string currentToken;
    int lineNumber = 1;
    int tokenStartLine = 1; // Will hold the starting line for the current token.
    State state = State::START;
    char stringQuote = '\0';
    bool escapeNext = false;
    bool potentialMultilineComment = true;
    string lastTokenType;
    // Add these new variables for string quote tracking
    string pendingQuote;
    int pendingQuoteLine = 1;
    bool inMultilineComment = false;
    bool inMultilineString = false;
    
    // New variables for indentation tracking
    vector<int> indentStack = {0};  // Starts with 0 indent level
    int currentIndent = 0;
    bool atLineStart = true;

    auto flushCurrentToken = [&]() {
        if (!currentToken.empty()) {
            string type;
            if (state == State::IN_IDENTIFIER) {
                if (isKeyword(currentToken)) {
                    tokens.push_back({"KEYWORD", currentToken, tokenStartLine});
                } else {
                    tokens.push_back({"IDENTIFIER", currentToken, tokenStartLine});
                }
                lastTokenType = "IDENTIFIER"; // or "KEYWORD" - but we handle both cases above
                currentToken.clear();
                return;
            }
            else if (state == State::IN_NUMBER) {
                tokens.push_back({"NUMBER", currentToken, tokenStartLine});
                lastTokenType = "NUMBER";
            }
            else if (state == State::IN_OPERATOR) {
                tokens.push_back({"OPERATOR", currentToken, tokenStartLine});
                lastTokenType = "OPERATOR";
            }    
            
            if (!type.empty()) {
                tokens.push_back({type, currentToken, tokenStartLine});
                lastTokenType = type;
            }
            currentToken.clear();
        }
        state = State::START;
    };

    for (size_t i = 0; i < source.size(); i++) {
        char c = source[i];
        
        // Handle newlines - this should be at the top of the loop
        if (c == '\n' && state == State::START) {
            flushCurrentToken();

            // Add NEWLINE token before incrementing lineNumber
            tokens.push_back({"NEWLINE", "\\n", lineNumber});
        
            lineNumber++;
            atLineStart = true;
            currentIndent = 0;
            
            // Check if the next line is empty or has less indentation
            size_t next_pos = i + 1;
            int next_indent = 0;
            while (next_pos < source.size() && isspace(source[next_pos])) {
                if (source[next_pos] == '\n') break;
                if (source[next_pos] == ' ') next_indent++;
                next_pos++;
            }
            
            // If next line is empty or has less indentation, we might need to dedent
            if (next_pos < source.size() && (source[next_pos] == '\n' || next_indent < indentStack.back())) {
                // We'll handle the actual dedent when we process the next line
            }
            continue;
        }
        
        // Handle indentation at start of line
        if (atLineStart) {
            if (isspace(c) && c != '\n') {
                currentIndent++;
                continue;
            } else {
                // We've reached the first non-space character in the line
                atLineStart = false;
                
                // Handle indentation changes
                if (!inMultilineComment && !inMultilineString) {  // Don't process indents in comments
                    if (currentIndent > indentStack.back()) {
                        // Increased indentation
                        tokens.push_back({"INDENT", "", lineNumber});
                        indentStack.push_back(currentIndent);
                    } else if (currentIndent < indentStack.back()) {
                        // Decreased indentation - may need multiple DEDENTs
                        while (currentIndent < indentStack.back()) {
                            tokens.push_back({"DEDENT", "", lineNumber});
                            indentStack.pop_back();
                            
                            if (indentStack.empty()) {
                                std::cerr << "Error: Indentation error at line " << lineNumber 
                                          << " - dedented past initial level" << std::endl;
                                indentStack.push_back(0);
                                break;
                            }
                        }
                        
                        if (currentIndent != indentStack.back()) {
                            std::cerr << "Error: Inconsistent indentation at line " << lineNumber << std::endl;
                        }
                    }
                }
            }
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
                else if (c == '\n') {
                    // First process any current token
                    flushCurrentToken();
                    // Add NEWLINE token with current line number
                    tokens.push_back({"NEWLINE", "\\n", lineNumber});
                    // Then increment line number for next line
                    lineNumber++;
                    atLineStart = true;
                    currentIndent = 0;
                    continue;
                }
                else if (isalpha(c) || c == '_') {
                    tokenStartLine = lineNumber; // start number token
                    currentToken += c;
                    state = State::IN_IDENTIFIER;
                }
                else if (isdigit(c)) {
                    tokenStartLine = lineNumber; // start number token
                    currentToken += c;
                    state = State::IN_NUMBER;
                }
                else if (c == ':') {
                    tokenStartLine = lineNumber; // start operator token
                    currentToken += c;
                    state = State::IN_OPERATOR;
                }
                else if (c == '-' && i + 1 < source.size() && isdigit(source[i + 1])) {
                    // Check if previous token was equals sign
                    if (!tokens.empty() && tokens.back().value == "=") {
                        // This is a negative number after equals
                        tokenStartLine = lineNumber;
                        currentToken += c;
                        state = State::IN_NUMBER;
                    } else {
                        // Handle as normal operator
                        tokenStartLine = lineNumber;
                        currentToken += c;
                        state = State::IN_OPERATOR;
                    }
                }
                else if (isOperator(string(1, c))) {
                    tokenStartLine = lineNumber; // start operator token
                    currentToken += c;
                    state = State::IN_OPERATOR;
                }

                else if (c == '\'' || c == '"') {
                    // Check if this is a triple quote
                    if (i + 2 < source.size() && source[i+1] == c && source[i+2] == c) {
                        // Check if it's an assignment (real string) or comment
                        bool isString = false;
                        for (int j = i - 1; j >= 0; --j) {
                            if (isspace(source[j])) continue;
                            if (source[j] == '=') {
                                isString = true;
                                break;
                            }
                            break;
                        }

                        pendingQuote = string(3, c);
                        pendingQuoteLine = lineNumber;
                        tokenStartLine = lineNumber;

                        if (isString) {
                            // Real string - tokenize opening quotes
                            tokens.push_back({"STRING_QUOTE", pendingQuote, pendingQuoteLine});
                            state = State::IN_MULTILINE_STRING;
                            stringQuote = c;
                            i += 2; // Skip next two quotes
                            currentToken.clear();
                        } else {
                            // Comment - skip until closing quotes
                            state = State::IN_MULTILINE_COMMENT;
                            stringQuote = c;
                            i += 2; // Skip next two quotes
                            inMultilineComment = true;
                        }
                    } else {
                        // Single-line string
                        state = State::IN_STRING;
                        stringQuote = c;
                        pendingQuote = string(1, c);
                        pendingQuoteLine = lineNumber;
                        currentToken.clear();
                    }
                }
                else if (c == '#') {
                    tokenStartLine = lineNumber;
                    state = State::IN_COMMENT;
                }
                else if (c == '-' && i + 1 < source.size() && isdigit(source[i + 1])) {
                    if (lastTokenType.empty() || lastTokenType == "OPERATOR" || lastTokenType == "DELIMITER") {
                        tokenStartLine = lineNumber; // start negative number token
                        state = State::IN_NUMBER;
                        currentToken += c;
                    } else {
                        tokenStartLine = lineNumber; // start operator token
                        state = State::IN_OPERATOR;
                        std::string opStr(1, c);
                        while (i + 1 < source.size() && isOperatorChar(source[i + 1])) {
                            opStr.push_back(source[i + 1]);
                            i++;
                        }
                        if (isOperator(opStr)) {
                            tokens.push_back({"OPERATOR", opStr, tokenStartLine});
                        } else {
                            std::cerr << "Error: Invalid operator at line " << tokenStartLine << ": " << opStr << std::endl;
                        }
                    }
                }
                else if (isOperatorChar(c)) {
                    tokenStartLine = lineNumber;
                    std::string opStr(1, c);
                    while (i + 1 < source.size() && isOperatorChar(source[i + 1])) {
                        opStr.push_back(source[i + 1]);
                        i++;
                    }
                
                    if (isOperator(opStr)) {
                        tokens.push_back({"OPERATOR", opStr, tokenStartLine});
                    } else {
                        std::cerr << "Error: Invalid operator at line " << tokenStartLine << ": " << opStr << std::endl;
                    }
                
                    if (c == '=' || c == '(') {
                        potentialMultilineComment = false;
                    }
                }
                else if (isDelimiter(std::string(1, c))) {
                    if (c == '.' && i + 2 < source.size() && source[i + 1] == '.' && source[i + 2] == '.') {
                        tokens.push_back({"DELIMITER", "...", lineNumber});
                        i += 2;
                    } else {
                        tokens.push_back({"DELIMITER", std::string(1, c), lineNumber});
                    }
                }
                else {
                    std::cerr << "Error: Unrecognized character at line " << tokenStartLine << ": " << c << std::endl;
                }
                break;

            case State::IN_IDENTIFIER:
                if (isalnum(c) || c == '_') {
                    currentToken += c;
                } else {
                    // Flush the current token before processing the next character
                    if (isKeyword(currentToken)) {
                        tokens.push_back({"KEYWORD", currentToken, tokenStartLine});
                    } else {
                        tokens.push_back({"IDENTIFIER", currentToken, tokenStartLine});
                    }
                    lastTokenType = isKeyword(currentToken) ? "KEYWORD" : "IDENTIFIER";
                    currentToken.clear();
                    state = State::START;
                    i--; // Reprocess this character
                }
                break;

                case State::IN_NUMBER:
                if (c == '.' && i + 2 < source.size() && 
                    source[i+1] == '.' && source[i+2] == '.') {
                    if (isNumber(currentToken)) {
                        tokens.push_back({"NUMBER", currentToken, tokenStartLine});
                    } else {
                        std::cerr << "Error [INVALID_NUMBER_FORMAT]: Malformed number before ellipsis at line " 
                                  << tokenStartLine << ": " << currentToken << std::endl;
                    }
                    currentToken.clear();
                    state = State::START;
                    i--; // Reprocess for '...'
                    break;
                }
            
                // Continue building number if valid character
                if (isdigit(c) || 
                    (c == '.' && currentToken.find('.') == string::npos) ||
                    tolower(c) == 'e' || 
                    (tolower(c) == 'x' && currentToken == "0") ||
                    (tolower(c) == 'b' && currentToken == "0") ||
                    (tolower(c) == 'o' && currentToken == "0") ||
                    (tolower(c) == 'j' && !currentToken.empty()) ||
                    (currentToken.size() >= 2 && currentToken[0] == '0' && 
                     tolower(currentToken[1]) == 'x' && isHexDigit(c)) ||
                    (c == '-' && currentToken.empty())) {  // Allow minus sign at start of number
                    
                    // Handle exponent notation
                    if ((c == 'e' || c == 'E') && i + 1 < source.size()) {
                        char nextChar = source[i + 1];
                        currentToken += c;
                        if (nextChar == '+' || nextChar == '-') {
                            currentToken += nextChar;
                            i++;
                            if (i + 1 >= source.size() || !isdigit(source[i + 1])) {
                                std::cerr << "Error [INVALID_EXPONENT]: Incomplete exponent at line " 
                                          << tokenStartLine << ": " << currentToken << std::endl;
                                currentToken.clear();
                                state = State::START;
                                break;
                            }
                        } else if (!isdigit(nextChar)) {
                            std::cerr << "Error [INVALID_EXPONENT]: Missing exponent digits at line " 
                                      << tokenStartLine << ": " << currentToken << std::endl;
                            currentToken.clear();
                            state = State::START;
                            break;
                        }
                    } else {
                        currentToken += c;
                    }
                }
                else {
                    // Number termination checks
                    if (c == '.' && isdigit(source[i+1])) {
                        // Multiple decimal points case (e.g., 3.14.15)
                        while (i < source.size() && (isdigit(source[i]) || source[i] == '.')) {
                            currentToken += source[i];
                            i++;
                        }
                        i--;
                        std::cerr << "Error [MULTIPLE_DECIMALS]: Multiple decimal points at line " 
                                  << tokenStartLine << ": " << currentToken << std::endl;
                        currentToken.clear();
                        state = State::START;
                    }
                    else if (c == '\n') {
                        if (isNumber(currentToken)) {
                            tokens.push_back({"NUMBER", currentToken, tokenStartLine});
                        } else if (currentToken.find('.') != string::npos) {
                            std::cerr << "Error [TRAILING_DECIMAL]: Incomplete decimal at line " 
                                      << tokenStartLine << ": " << currentToken << std::endl;
                        } else {
                            std::cerr << "Error [INVALID_NUMBER_FORMAT]: Malformed number at line " 
                                      << tokenStartLine << ": " << currentToken << std::endl;
                        }
                        // Add NEWLINE token with current line number before incrementing
                        tokens.push_back({"NEWLINE", "\\n", lineNumber});
                        currentToken.clear();
                        lineNumber++;  // Now increment line number
                        atLineStart = true;
                        currentIndent = 0;
                        state = State::START;
                    }
                    else if (isalpha(c)) {
                        string invalidSuffix = currentToken + c;
                        size_t j = i + 1;
                    
                        while (j < source.size()) {
                            char nextChar = source[j];
                            if (nextChar == '\n' || !(isalnum(nextChar) || nextChar == '_')) break;
                            invalidSuffix += nextChar;
                            j++;
                        }
                    
                        i = j - 1;
                        
                        if (tolower(c) == 'e') {
                            std::cerr << "Error [INVALID_EXPONENT]: Malformed exponent at line " 
                                      << tokenStartLine << ": " << invalidSuffix << std::endl;
                        } 
                        else if (tolower(c) == 'x' || tolower(c) == 'b' || tolower(c) == 'o') {
                            std::cerr << "Error [INVALID_NUMBER_PREFIX]: Invalid base prefix at line " 
                                      << tokenStartLine << ": " << invalidSuffix << std::endl;
                        }
                        else if (tolower(c) == 'j') {
                            std::cerr << "Error [INVALID_COMPLEX]: Malformed complex number at line " 
                                      << tokenStartLine << ": " << invalidSuffix << std::endl;
                        }
                        else {
                            std::cerr << "Error [INVALID_SUFFIX]: Illegal characters in number at line " 
                                      << tokenStartLine << ": " << invalidSuffix << std::endl;
                        }
                        currentToken.clear();
                        state = State::START;
                    }
                    else {
                        // Final validation
                        if (count(currentToken.begin(), currentToken.end(), '.') > 1) {
                            std::cerr << "Error [MULTIPLE_DECIMALS]: Multiple decimal points at line " 
                                      << tokenStartLine << ": " << currentToken << std::endl;
                        }
                        else if (currentToken.back() == '.') {
                            std::cerr << "Error [TRAILING_DECIMAL]: Incomplete decimal at line " 
                                      << tokenStartLine << ": " << currentToken << std::endl;
                        }
                        else if (!isNumber(currentToken)) {
                            std::cerr << "Error [INVALID_NUMBER_FORMAT]: Unrecognized number format at line " 
                                      << tokenStartLine << ": " << currentToken << std::endl;
                        }
                        else {
                            tokens.push_back({"NUMBER", currentToken, tokenStartLine});
                        }
                        currentToken.clear();
                        state = State::START;
                        i--;
                    }
                }
                break;

                case State::IN_STRING:
                if (escapeNext) {
                    currentToken.push_back(c);
                    escapeNext = false;
                }
                else if (c == '\\') {
                    currentToken.push_back(c);
                    escapeNext = true;
                }
                else if (c == stringQuote) {
                    // Properly terminated string
                    tokens.push_back({"STRING_QUOTE", pendingQuote, pendingQuoteLine});
                    if (!currentToken.empty()) {
                        tokens.push_back({"STRING_LITERAL", currentToken, tokenStartLine});
                    }
                    tokens.push_back({"STRING_QUOTE", string(1, c), lineNumber});
                    currentToken.clear();
                    state = State::START;
                }
                else if ((c == '\'' || c == '"') && c != stringQuote) {
                    // Mismatched quote
                    std::cerr << "Error [MISMATCHED_QUOTE]: String started with " << stringQuote 
                              << " but encountered closing " << c 
                              << " at line " << lineNumber << std::endl;
                    currentToken.clear();
                    state = State::START;
                }
                else if (c == '\n') {
                // Unterminated string at newline
                tokens.push_back({"NEWLINE", "\\n", lineNumber});  // Add token first
                lineNumber++;  // Then increment
                atLineStart = true;
                currentIndent = 0;
                std::cerr << "Error [UNTERMINATED_STRING]: String started with " << stringQuote 
                        << " was not closed before end of line " << pendingQuoteLine << std::endl;
                currentToken.clear();
                state = State::START;;
                }
                else {
                    currentToken.push_back(c);
                }
                break;
                
            case State::IN_MULTILINE_STRING:
                inMultilineString = true;
                if (c == '\n') {
                    tokens.push_back({"NEWLINE", "\\n", lineNumber});  // Add token first
                    lineNumber++;  // Then increment
                    currentToken.push_back(c);
                }

                // Check for potential closing triple quotes
                if (c == stringQuote && i + 2 < source.size() && 
                    source[i+1] == stringQuote && source[i+2] == stringQuote) {
                    // Proper matching quotes - tokenize content and closing quotes
                    if (!currentToken.empty()) {
                        tokens.push_back({"STRING_LITERAL", currentToken, tokenStartLine});
                    }
                    tokens.push_back({"STRING_QUOTE", string(3, stringQuote), lineNumber});
                    currentToken.clear();
                    state = State::START;
                    inMultilineString = false;
                    i += 2; // Skip next two quotes
                }
                else if ((c == '\'' || c == '"') && c != stringQuote && 
                        i + 2 < source.size() && source[i+1] == c && source[i+2] == c) {
                    // Mismatched triple quotes error
                    std::cerr << "Error [MISMATCHED_TRIPLE_QUOTE]: Multiline string started with " 
                            << string(3, stringQuote) << " at line " << pendingQuoteLine 
                            << " but encountered closing " << string(3, c) 
                            << " at line " << lineNumber << std::endl;
                    
                    // Remove the opening quotes token if it was added
                    if (!tokens.empty() && tokens.back().value == string(3, stringQuote) && 
                        tokens.back().type == "STRING_QUOTE") {
                        tokens.pop_back();
                    }
                    
                    currentToken.clear();
                    state = State::START;
                    inMultilineString = false;
                    i += 2;
                }
                else {
                    currentToken += c;
                    
                    // Check for EOF while in multiline string
                    if (i == source.size() - 1) {
                        std::cerr << "Error [UNTERMINATED_MULTILINE_STRING]: String started with " 
                                << string(3, stringQuote) << " at line " << pendingQuoteLine 
                                << " was not properly closed before end of file" << std::endl;
                        state = State::START;
                        inMultilineString = false;
                    }
                }
                break;

                case State::IN_MULTILINE_COMMENT:
                if (c == '\n') {
                    tokens.push_back({"NEWLINE", "\\n", lineNumber});  // Add token first
                    lineNumber++;  // Then increment
                }
            
                // Check for potential closing triple quotes
                if (c == '\'' || c == '"') {
                    if (i + 2 < source.size() && source[i+1] == c && source[i+2] == c) {
                        // Found triple quotes - check if they match opening
                        if (c != stringQuote) {
                            std::cerr << "Error: Mismatched triple quotes in comment at line " 
                                      << lineNumber << ". Started with " << string(3, stringQuote)
                                      << " but ended with " << string(3, c) << std::endl;
                        }
                        state = State::START;
                        inMultilineComment = false;
                        i += 2; // Skip next two quotes
                        break;
                    }
                }
                
                // Check if we're at the last character of the file
                if (i == source.size() - 1) {
                    std::cerr << "Error: Unterminated multiline comment starting at line " 
                              << tokenStartLine << " with " << string(3, stringQuote) << std::endl;
                    // Special handling needed to continue parsing subsequent lines
                    state = State::START;
                    inMultilineComment = false;
                }
                break;

                case State::IN_COMMENT:
                // Single-line comment handling
                if (c == '\n') {
                    // Add NEWLINE token with current line number first
                    tokens.push_back({"NEWLINE", "\\n", lineNumber});
                    // Then increment the line number
                    lineNumber++;
                    atLineStart = true;
                    currentIndent = 0;
                    state = State::START; // End of single-line comment
                }
                break;

            case State::IN_OPERATOR:
                if (currentToken == ":" && c == '=') {
                    currentToken += c;
                    tokens.push_back({"OPERATOR", currentToken, tokenStartLine});
                    currentToken.clear();
                    state = State::START;
                   
                }
                else if (isOperator(currentToken + c)) {
                    currentToken += c;
                }
                else {
                    tokens.push_back({"OPERATOR", currentToken, tokenStartLine});
                    currentToken.clear();
                    state = State::START;
                 
                    i--; // Reprocess this character
                }
                break;
        }
    }

    // Flush any remaining token
    flushCurrentToken();

    // After the main loop, handle any remaining dedents
    while (indentStack.size() > 1) {
        tokens.push_back({"DEDENT", "", lineNumber});
        indentStack.pop_back();
    }
    if (state == State::IN_STRING) {
        std::cerr << "Error [UNTERMINATED_STRING]: String started with " << stringQuote 
                  << " at line " << pendingQuoteLine 
                  << " was not closed before end of file" << std::endl;
        // Attempt to recover by adding the pending quote token
        tokens.push_back({"STRING_QUOTE", pendingQuote, pendingQuoteLine});
        if (!currentToken.empty()) {
            tokens.push_back({"STRING_LITERAL", currentToken, tokenStartLine});
        }
        state = State::START;
    }
    else if (state == State::IN_MULTILINE_STRING) {
        std::cerr << "Error [UNTERMINATED_MULTILINE_STRING]: String started with " 
                  << string(3, stringQuote) << " at line " << pendingQuoteLine 
                  << " was not closed before end of file" << std::endl;
        // Attempt to recover by adding the pending quote token
        tokens.push_back({"STRING_QUOTE", string(3, stringQuote), pendingQuoteLine});
        if (!currentToken.empty()) {
            tokens.push_back({"STRING_LITERAL", currentToken, tokenStartLine});
        }
        state = State::START;
    }

    // Check for unterminated multiline string
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
    unordered_map<pair<string, string>, SymbolEntry, PairHash> symbolTable;
    vector<pair<string, string>> symbolOrder; // To maintain the order of occurrence
    int currentId = 1;

    vector<string> scopeStack = {"global"};  // Stack to track current scope

    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];

        // Handle entering a new scope (function or class)
        if (token.value == "def" && i + 1 < tokens.size() && tokens[i + 1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i + 1].value);  // Push function name as scope
        } else if (token.value == "class" && i + 1 < tokens.size() && tokens[i + 1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i + 1].value);  // Push class name as scope
        }
        else if(token.value == "for" && i + 1 < tokens.size() && tokens[i + 1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i].value);  //For as scope name
        }
        else if(token.value == "if" && i + 1 < tokens.size() && tokens[i + 1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i].value);  //if as scope name
        }
        else if(token.value == "while" && i + 1 < tokens.size() && tokens[i + 1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i].value);  //while as scope name
        }
        else if(token.value =="elif" && i + 1 < tokens.size() && tokens[i + 1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i].value);  //elif as scope name
        }
        else if(token.value == "else" && i + 1 < tokens.size() && tokens[i + 1].type == "IDENTIFIER") {
            scopeStack.push_back(tokens[i].value);  //else as scope name
        }



        // Handle exiting a scope (based on DEDENT tokens)
        if (token.type == "DEDENT" && scopeStack.size() > 1) {
            scopeStack.pop_back();  // Pop the current scope
        }

        // Process identifiers
        if (token.type == "IDENTIFIER") {
            string currentScope = scopeStack.back();  // Get the current scope
            pair<string, string> key = {token.value, currentScope};  // Composite key

            if (!isIdentifierInSymbolTable(symbolTable , token.value) && token.value != currentScope) {
                // Add new identifier to the symbol table
                symbolTable[key] = {currentId++, {token.line}, "unknown", "undefined", currentScope};
                symbolOrder.push_back(key); // Record the order of occurrence
            }
            else if(currentScope == "if" || currentScope == "for" || currentScope == "while" || currentScope == "elif" || currentScope == "else"){
                SymbolEntry* entry = getEntryByScope(symbolTable , token.value , getHighestScope(symbolTable , token.value));
                if(entry != nullptr) {
                    if(find(entry->lines.begin(), entry->lines.end(), token.line) == entry->lines.end()) {
                        entry->lines.push_back(token.line);
                    }
                }
            }
            else if(token.value == currentScope && tokens[i - 1].value == "def") {
                // Function or class definition - add scope

                string previous_scope = "global";
                if(scopeStack.size() > 1) {
                    previous_scope = scopeStack[scopeStack.size() - 2]; // Get the previous scope
                }

                symbolTable[key] = {currentId++, {token.line}, "function", "undefined", previous_scope};
                symbolOrder.push_back(key); // Record the order of occurrence
            }
            else if(token.value == currentScope && tokens[i - 1].value == "class") {
                // Class definition - add scope

                string previous_scope = "global";
                if(scopeStack.size() > 1) {
                    previous_scope = scopeStack[scopeStack.size() - 2]; // Get the previous scope
                }

                symbolTable[key] = {currentId++, {token.line}, "class", "undefined", previous_scope};
                symbolOrder.push_back(key); // Record the order of occurrence

            }
            else{
                // Update existing identifier
                // SymbolEntry& entry = symbolTable[key];
                // if (find(entry.lines.begin(), entry.lines.end(), token.line) == entry.lines.end()) {
                    //     entry.lines.push_back(token.line);
                    // }
                SymbolEntry* entry = getEntryByScope(symbolTable , token.value , currentScope);
                string highest_scope = getHighestScope(symbolTable , token.value);
                if(entry != nullptr) {
                    if(find(entry->lines.begin(), entry->lines.end(), token.line) == entry->lines.end()) {
                        entry->lines.push_back(token.line);
                    }
                }
                else {
                    // If the identifier is not found in the current scope, add it
                    symbolTable[key] = {currentId++, {token.line}, "unknown", "undefined", currentScope};
                    symbolOrder.push_back(key); // Record the order of occurrence
                }

            }
        }

        // Handle assignments to infer types and values
        if (token.type == "IDENTIFIER" && i + 1 < tokens.size() && tokens[i + 1].value == "=") {
            string identifier = token.value;
            string currentScope = scopeStack.back();
            pair<string, string> key = {identifier, currentScope};

            const Token& valueToken = tokens[i + 2];
            if (valueToken.type == "NUMBER") {
                symbolTable[key].type = "numeric";
                symbolTable[key].value = valueToken.value;
            } else if (valueToken.type == "STRING_LITERAL") {
                symbolTable[key].type = "string";
                symbolTable[key].value = valueToken.value;
            } else if (valueToken.value == "True" || valueToken.value == "False") {
                symbolTable[key].type = "boolean";
                symbolTable[key].value = valueToken.value;
            }
        }

        // Handle built-in functions
        if (token.type == "IDENTIFIER" && (token.value == "print" || token.value == "format")) {
            string currentScope = scopeStack.back();
            pair<string, string> key = {token.value, currentScope};

            if (symbolTable.find(key) == symbolTable.end()) {
                symbolTable[key] = {currentId++, {token.line}, "builtin_function", "undefined", currentScope};
                symbolOrder.push_back(key); // Record the order of occurrence
            }
        }
    }

    // Print the symbol table
    cout << "\nSYMBOL TABLE (With Scope)\n";
    cout << "---------------------------------------------\n";

    if (symbolTable.empty()) {
        cout << "No identifiers found in the code.\n";
        return;
    }

    // Calculate column widths
    int idColWidth = 5;
    int nameColWidth = 20;
    int typeColWidth = 15;
    int valueColWidth = 20;
    int scopeColWidth = 15;
    int linesColWidth = 30;

    for (const auto& key : symbolOrder) {
        const auto& entry = symbolTable[key];
        if (key.first.length() > nameColWidth) nameColWidth = key.first.length();
        if (entry.type.length() > typeColWidth) typeColWidth = entry.type.length();
        if (entry.value.length() > valueColWidth) valueColWidth = entry.value.length();
        if (entry.scope.length() > scopeColWidth) scopeColWidth = entry.scope.length();
    }

    // Print table header
    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
         << "-+-" << string(typeColWidth, '-') << "-+-" << string(valueColWidth, '-')
         << "-+-" << string(scopeColWidth, '-') << "-+-" << string(linesColWidth, '-') << "-+" << endl;
    cout << "| " << left << setw(idColWidth) << "ID" << " | "
         << setw(nameColWidth) << "IDENTIFIER" << " | "
         << setw(typeColWidth) << "TYPE" << " | "
         << setw(valueColWidth) << "VALUE" << " | "
         << setw(scopeColWidth) << "SCOPE" << " | "
         << setw(linesColWidth) << "LINES" << " |" << endl;
    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
         << "-+-" << string(typeColWidth, '-') << "-+-" << string(valueColWidth, '-')
         << "-+-" << string(scopeColWidth, '-') << "-+-" << string(linesColWidth, '-') << "-+" << endl;

    // Print table rows in order of occurrence
    for (const auto& key : symbolOrder) {
        const auto& entry = symbolTable[key];
        string linesStr;
        for (size_t i = 0; i < entry.lines.size(); i++) {
            if (i > 0) linesStr += ", ";
            linesStr += to_string(entry.lines[i]);
        }

        cout << "| " << right << setw(idColWidth) << entry.id << " | "
             << left << setw(nameColWidth) << key.first << " | "
             << setw(typeColWidth) << entry.type << " | "
             << setw(valueColWidth) << entry.value << " | "
             << setw(scopeColWidth) << entry.scope << " | "
             << setw(linesColWidth) << linesStr << " |" << endl;
    }

    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
         << "-+-" << string(typeColWidth, '-') << "-+-" << string(valueColWidth, '-')
         << "-+-" << string(scopeColWidth, '-') << "-+-" << string(linesColWidth, '-') << "-+" << endl;
    cout << "Total identifiers: " << symbolTable.size() << endl << endl;
}

// int main() {
//     string input;
//     cout << "PYTHON LEXICAL ANALYZER\n";
//     cout << "=======================\n";
//     cout << "1. Enter Python code manually\n";
//     cout << "2. Read from file\n";
//     cout << "Choose option (1/2): ";

//     int option;
//     cin >> option;
//     cin.ignore();

//     if (option == 1) {
//         cout << "\nEnter Python code (end with empty line):\n";
//         string line;
//         while (true) {
//             getline(cin, line);
//             if (line.empty()) break;
//             input += line + "\n";
//         }
//     }
//     else if (option == 2) {
//         string filename;
//         cout << "\nEnter filename: ";
//         getline(cin, filename);

//         ifstream file(filename);
//         if (!file.is_open()) {
//             cerr << "Error opening file!\n";
//             return 1;
//         }

//         input = string((istreambuf_iterator<char>(file)),
//             istreambuf_iterator<char>());
//         file.close();
//     }
//     else {
//         cerr << "Invalid option!\n";
//         return 1;
//     }

//     vector<Token> tokens = tokenize(input);

//     cout << "\nTOKENS FOUND\n";
//     cout << "============\n";
//     printTokenTable(tokens);

//     generateSymbolTable(tokens);

//     return 0;
// }