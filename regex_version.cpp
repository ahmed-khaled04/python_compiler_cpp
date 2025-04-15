#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <iomanip>

using namespace std;

// Token types
enum class TokenType {
    KEYWORD, IDENTIFIER, NUMBER, OPERATOR, DELIMITER,
    STRING_LITERAL, COMMENT, WHITESPACE, NEWLINE, UNKNOWN
};

// Mapping of token types to strings
const unordered_map<TokenType, string> TokenTypeStrings = {
    {TokenType::KEYWORD, "KEYWORD"},
    {TokenType::IDENTIFIER, "IDENTIFIER"},
    {TokenType::NUMBER, "NUMBER"},
    {TokenType::OPERATOR, "OPERATOR"},
    {TokenType::DELIMITER, "DELIMITER"},
    {TokenType::STRING_LITERAL, "STRING_LITERAL"},
    {TokenType::COMMENT, "COMMENT"},
    {TokenType::WHITESPACE, "WHITESPACE"},
    {TokenType::NEWLINE, "NEWLINE"},
    {TokenType::UNKNOWN, "UNKNOWN"}
};

struct Token {
    TokenType type;
    string value;
    int line;
};

// Regular expressions for token patterns with error handling
const vector<pair<regex, TokenType>> tokenPatterns = {
    // Comments (single-line)
    {regex(R"(#[^\n]*)"), TokenType::COMMENT},
    
    // Whitespace (skip these)
    {regex(R"([ \t]+)"), TokenType::WHITESPACE},
    
    // Newlines (track these for line counting)
    {regex(R"(\n)"), TokenType::NEWLINE},
    
    // Keywords (must be checked before identifiers)
    {regex(R"(\b(False|None|True|and|as|assert|async|await|break|class|continue|def|del|elif|else|except|finally|for|from|global|if|import|in|is|lambda|nonlocal|not|or|pass|raise|return|try|while|with|yield)\b)"), TokenType::KEYWORD},
    
    // Identifiers
    {regex(R"([a-zA-Z_][a-zA-Z0-9_]*)"), TokenType::IDENTIFIER},
    
    // Numbers (integers and floats with error checking)
    {regex(R"(\d+\.\d*|\.\d+|\d+)"), TokenType::NUMBER},
    
    // String literals (single, double, and triple quoted with error checking)
    {regex(R"((['\"]{3})(?:[^'"]|['"](?!['"]{2}))*['"]{3})"), TokenType::STRING_LITERAL},  // Triple quotes
    {regex(R"((['\"])(?:(?!\1).)*\1)"), TokenType::STRING_LITERAL},     // Single/double quotes
    {regex(R"(['\"])"), TokenType::UNKNOWN},  // Handle unterminated strings
    
    // Operators (including ** as a single operator)
    {regex(R"(\*\*|\+|\-|\*|\/|\/\/|\%|\=|\=\=|\!\=|\<|\>|\<\=|\>\=|\&|\||\^|\~|\<\<|\>\>|\+\=|\-\=|\*\=|\/\=|\%\=|\/\/\=)"), TokenType::OPERATOR},
    
    // Delimiters
    {regex(R"(\(|\)|\[|\]|\{|\}|\,|\:|\;|\.|\@)"), TokenType::DELIMITER}
};

// Error checking functions
bool isValidNumber(const string& num) {
    // Check for multiple decimal points
    if (count(num.begin(), num.end(), '.') > 1) {
        cerr << "Error: Invalid number format - multiple decimal points" << endl;
        return false;
    }
    
    // Check for invalid characters
    for (char c : num) {
        if (!isdigit(c) && c != '.' && c != 'e' && c != 'E' && c != '+' && c != '-') {
            cerr << "Error: Invalid character in number: " << c << endl;
            return false;
        }
    }
    
    return true;
}

bool isValidString(const string& str, int lineNum) {
    if (str.empty()) return false;
    
    // Handle empty strings
    if (str.size() == 2 && (str[0] == '\'' || str[0] == '"') && str[0] == str[1]) {
        return true;
    }
    
    // Check for triple quotes
    if (str.size() >= 3 && str[0] == str[1] && str[1] == str[2]) {
        char quoteType = str[0];
        
        // Must have at least 6 characters (3 opening + 3 closing)
        if (str.length() < 6) {
            cerr << "Error: Incomplete triple-quoted string at line " << lineNum << endl;
            return false;
        }
        
        // Check closing quotes
        if (str.substr(str.length() - 3) != string(3, quoteType)) {
            cerr << "Error: Mismatched triple quotes at line " << lineNum << endl;
            return false;
        }
        
        return true;
    }
    
    // Check for single/double quotes
    if (str.front() != str.back()) {
        cerr << "Error: Unterminated string literal at line " << lineNum << endl;
        return false;
    }
    
    return true;
}

vector<Token> tokenize(const string& source) {
    vector<Token> tokens;
    int lineNumber = 1;
    size_t pos = 0;
    
    while (pos < source.length()) {
        bool matched = false;
        
        // Try to match each pattern in order
        for (const auto& [pattern, type] : tokenPatterns) {
            smatch match;
            string remaining = source.substr(pos);
            
            if (regex_search(remaining, match, pattern, regex_constants::match_continuous)) {
                string value = match.str();
                
                // Error checking based on token type
                if (type == TokenType::NUMBER && !isValidNumber(value)) {
                    pos += value.length();
                    matched = true;
                    break;
                }
                
                if (type == TokenType::STRING_LITERAL) {
                    if (!isValidString(value, lineNumber)) {
                        // If string is invalid, try to find the next valid token
                        pos++;
                        matched = true;
                        break;
                    }
                    
                    // Count newlines in the string for line tracking
                    int newlines = count(value.begin(), value.end(), '\n');
                    if (newlines > 0) {
                        lineNumber += newlines;
                    }
                }
                
                // Skip whitespace and comments if we're not including them
                if (type == TokenType::WHITESPACE || type == TokenType::COMMENT) {
                    pos += value.length();
                    matched = true;
                    break;
                }
                
                // Count newlines for line tracking
                if (type == TokenType::NEWLINE) {
                    lineNumber++;
                    pos += value.length();
                    matched = true;
                    break;
                }
                
                // Add the token
                tokens.push_back({type, value, lineNumber});
                pos += value.length();
                matched = true;
                break;
            }
        }
        
        // If no pattern matched, it's an unknown character
        if (!matched) {
            cerr << "Error: Unrecognized character at line " << lineNumber 
                 << ": '" << source[pos] << "'" << endl;
            tokens.push_back({TokenType::UNKNOWN, string(1, source[pos]), lineNumber});
            pos++;
        }
    }
    
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
        string typeStr = TokenTypeStrings.at(token.type);
        if (typeStr.length() > tokenColWidth) tokenColWidth = typeStr.length();
        if (token.value.length() > valueColWidth) valueColWidth = token.value.length();
    }

    printHorizontalLine(tokenColWidth, valueColWidth, lineColWidth);
    cout << "| " << left << setw(tokenColWidth) << "TOKEN TYPE" << " | "
         << setw(valueColWidth) << "VALUE" << " | "
         << setw(lineColWidth) << "LINE" << " |" << endl;
    printHorizontalLine(tokenColWidth, valueColWidth, lineColWidth);

    for (const auto& token : tokens) {
        if (token.type == TokenType::WHITESPACE || token.type == TokenType::NEWLINE) {
            continue; // Skip whitespace and newlines in output
        }
        
        string typeStr = TokenTypeStrings.at(token.type);
        cout << "| " << left << setw(tokenColWidth) << typeStr << " | "
             << setw(valueColWidth) << token.value << " | "
             << right << setw(lineColWidth) << token.line << " |" << endl;
    }

    printHorizontalLine(tokenColWidth, valueColWidth, lineColWidth);
    cout << "Total tokens: " << tokens.size() << endl << endl;
}

void generateSymbolTable(const vector<Token>& tokens) {
    struct SymbolEntry {
        int id;
        vector<int> lines;
    };

    unordered_map<string, SymbolEntry> symbolTable;
    int currentId = 1;

    for (const Token& token : tokens) {
        if (token.type == TokenType::IDENTIFIER) {
            if (symbolTable.find(token.value) == symbolTable.end()) {
                symbolTable[token.value] = {currentId++, {token.line}};
            } else {
                auto& lines = symbolTable[token.value].lines;
                if (lines.empty() || lines.back() != token.line) {
                    lines.push_back(token.line);
                }
            }
        }
    }

    cout << "\nSYMBOL TABLE\n";
    cout << "------------\n";

    if (symbolTable.empty()) {
        cout << "No identifiers found in the code.\n";
        return;
    }

    int idColWidth = 5;
    int nameColWidth = 20;
    int linesColWidth = 30;

    for (const auto& entry : symbolTable) {
        if (entry.first.length() > nameColWidth) {
            nameColWidth = entry.first.length();
        }
    }

    printHorizontalLine(idColWidth, nameColWidth, linesColWidth);
    cout << "| " << left << setw(idColWidth) << "ID" << " | "
         << setw(nameColWidth) << "IDENTIFIER" << " | "
         << setw(linesColWidth) << "LINES" << " |" << endl;
    printHorizontalLine(idColWidth, nameColWidth, linesColWidth);

    for (const auto& entry : symbolTable) {
        string linesStr;
        for (size_t i = 0; i < entry.second.lines.size(); i++) {
            if (i > 0) linesStr += ", ";
            linesStr += to_string(entry.second.lines[i]);
        }

        cout << "| " << right << setw(idColWidth) << entry.second.id << " | "
             << left << setw(nameColWidth) << entry.first << " | "
             << setw(linesColWidth) << linesStr << " |" << endl;
    }

    printHorizontalLine(idColWidth, nameColWidth, linesColWidth);
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

    vector<Token> tokens = tokenize(input);

    cout << "\nTOKENS FOUND\n";
    cout << "============\n";
    printTokenTable(tokens);

    generateSymbolTable(tokens);

    return 0;
}
