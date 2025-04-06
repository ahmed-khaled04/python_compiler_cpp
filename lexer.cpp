#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cctype>

using namespace std;

// Python keywords
const unordered_set<string> KEYWORDS = {
    "False", "None", "True", "and", "as", "assert", "async", "await",
    "break", "class", "continue", "def", "del", "elif", "else", "except",
    "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
    "try", "while", "with", "yield","print"
};

// Python operators
const unordered_set<string> OPERATORS = {
    "+", "-", "*", "/", "%", "**", "//", "=", "+=", "-=", "*=", "/=",
    "%=", "**=", "//=", "==", "!=", "<", ">", "<=", ">=", "&", "|",
    "^", "~", "<<", ">>", "and", "or", "not", "is", "in"
};

// Python delimiters
const unordered_set<string> DELIMITERS = {
    "(", ")", "[", "]", "{", "}", ",", ":", ".", ";", "@"
};

struct Token {
    string type;
    string value;
    int line;
};

// Function prototypes
bool isKeyword(const string& str);
bool isOperator(const string& str);
bool isDelimiter(const string& str);
bool isIdentifier(const string& str);
bool isNumber(const string& str);
bool isStringLiteral(const string& str);
vector<Token> tokenize(const string& source);
void printTokens(const vector<Token>& tokens);
void generateSymbolTable(const vector<Token>& tokens);

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
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

bool isNumber(const string& str) {
    bool hasDecimal = false;
    for (char c : str) {
        if (c == '.') {
            if (hasDecimal) return false;
            hasDecimal = true;
        }
        else if (!isdigit(c)) {
            return false;
        }
    }
    return !str.empty();
}

bool isStringLiteral(const string& str) {
    return (str.size() >= 2) &&
        ((str.front() == '\'' && str.back() == '\'') ||
            (str.front() == '"' && str.back() == '"'));
}

vector<Token> tokenize(const string& source) {
    vector<Token> tokens;
    string currentToken;
    int lineNumber = 1;
    bool inString = false;
    char stringQuote = '\0';
    bool inComment = false;
    bool inMultiLineComment = false;
    char multiLineQuote = '\0';

    for (size_t i = 0; i < source.size(); i++) {
        char c = source[i];

        // Handle multi-line comments (''' or """)
        if (!inString && !inComment && !inMultiLineComment &&
            (c == '\'' || c == '"') && i + 2 < source.size() &&
            source[i + 1] == c && source[i + 2] == c) {

            inMultiLineComment = true;
            multiLineQuote = c;
            i += 2; // Skip the next two quotes
            continue;
        }

        // Check for end of multi-line comment
        if (inMultiLineComment && c == multiLineQuote &&
            i + 2 < source.size() && source[i + 1] == multiLineQuote &&
            source[i + 2] == multiLineQuote) {
            inMultiLineComment = false;
            i += 2; // Skip the next two quotes
            continue;
        }

        if (inMultiLineComment) {
            if (c == '\n') lineNumber++;
            continue;
        }

        // Handle single-line comments
        if (c == '#' && !inString && !inComment) {
            inComment = true;
            continue;
        }

        if (inComment) {
            if (c == '\n') {
                inComment = false;
                lineNumber++;
            }
            continue;
        }

        // Handle string literals
        if ((c == '\'' || c == '"') && !inString) {
            inString = true;
            stringQuote = c;
            currentToken += c;
            continue;
        }
        else if (inString && c == stringQuote) {
            // Check if it's an escaped quote
            if (i > 0 && source[i - 1] == '\\') {
                currentToken += c;
                continue;
            }

            inString = false;
            currentToken += c;
            tokens.push_back({ "STRING_LITERAL", currentToken, lineNumber });
            currentToken.clear();
            continue;
        }
        else if (inString) {
            currentToken += c;
            if (c == '\n') lineNumber++;
            continue;
        }

        // Handle whitespace
        if (isspace(c)) {
            if (c == '\n') lineNumber++;

            if (!currentToken.empty()) {
                if (isKeyword(currentToken)) {
                    tokens.push_back({ "KEYWORD", currentToken, lineNumber });
                }
                else if (isOperator(currentToken)) {
                    tokens.push_back({ "OPERATOR", currentToken, lineNumber });
                }
                else if (isDelimiter(currentToken)) {
                    tokens.push_back({ "DELIMITER", currentToken, lineNumber });
                }
                else if (isNumber(currentToken)) {
                    tokens.push_back({ "NUMBER", currentToken, lineNumber });
                }
                else if (isIdentifier(currentToken)) {
                    tokens.push_back({ "IDENTIFIER", currentToken, lineNumber });
                }
                else {
                    tokens.push_back({ "UNKNOWN", currentToken, lineNumber });
                }
                currentToken.clear();
            }
            continue;
        }

        // Handle operators and delimiters
        if (isDelimiter(string(1, c))) {
            if (!currentToken.empty()) {
                if (isKeyword(currentToken)) {
                    tokens.push_back({ "KEYWORD", currentToken, lineNumber });
                }
                else if (isOperator(currentToken)) {
                    tokens.push_back({ "OPERATOR", currentToken, lineNumber });
                }
                else if (isNumber(currentToken)) {
                    tokens.push_back({ "NUMBER", currentToken, lineNumber });
                }
                else if (isIdentifier(currentToken)) {
                    tokens.push_back({ "IDENTIFIER", currentToken, lineNumber });
                }
                else {
                    tokens.push_back({ "UNKNOWN", currentToken, lineNumber });
                }
                currentToken.clear();
            }
            tokens.push_back({ "DELIMITER", string(1, c), lineNumber });
            continue;
        }

        if (isOperator(string(1, c))) {
            if (!currentToken.empty() && !isOperator(currentToken)) {
                if (isKeyword(currentToken)) {
                    tokens.push_back({ "KEYWORD", currentToken, lineNumber });
                }
                else if (isNumber(currentToken)) {
                    tokens.push_back({ "NUMBER", currentToken, lineNumber });
                }
                else if (isIdentifier(currentToken)) {
                    tokens.push_back({ "IDENTIFIER", currentToken, lineNumber });
                }
                else {
                    tokens.push_back({ "UNKNOWN", currentToken, lineNumber });
                }
                currentToken.clear();
            }
            currentToken += c;
            continue;
        }

        currentToken += c;
    }

    // Add the last token if exists
    if (!currentToken.empty()) {
        if (isKeyword(currentToken)) {
            tokens.push_back({ "KEYWORD", currentToken, lineNumber });
        }
        else if (isOperator(currentToken)) {
            tokens.push_back({ "OPERATOR", currentToken, lineNumber });
        }
        else if (isDelimiter(currentToken)) {
            tokens.push_back({ "DELIMITER", currentToken, lineNumber });
        }
        else if (isNumber(currentToken)) {
            tokens.push_back({ "NUMBER", currentToken, lineNumber });
        }
        else if (isIdentifier(currentToken)) {
            tokens.push_back({ "IDENTIFIER", currentToken, lineNumber });
        }
        else {
            tokens.push_back({ "UNKNOWN", currentToken, lineNumber });
        }
    }

    return tokens;
}

void generateSymbolTable(const vector<Token>& tokens) {
    unordered_map<string, vector<int>> symbolTable;

    for (const Token& token : tokens) {
        if (token.type == "IDENTIFIER") {
            symbolTable[token.value].push_back(token.line);
        }
    }

    cout << "\nSymbol Table:\n";
    cout << "-------------\n";
    cout << "Identifier\tLines\n";
    cout << "-------------\n";

    for (const auto& entry : symbolTable) {
        cout << entry.first << "\t\t";
        for (size_t i = 0; i < entry.second.size(); i++) {
            if (i > 0) cout << ", ";
            cout << entry.second[i];
        }
        cout << "\n";
    }
}

int main() {
    string input;
    cout << "Python Lexical Analyzer\n";
    cout << "1. Enter Python code manually\n";
    cout << "2. Read from file\n";
    cout << "Choose option (1/2): ";

    int option;
    cin >> option;
    cin.ignore(); // Clear newline

    if (option == 1) {
        cout << "Enter Python code (end with empty line):\n";
        string line;
        input = "";
        while (true) {
            getline(cin, line);
            if (line.empty()) break;
            input += line + "\n";
        }
    }
    else if (option == 2) {
        string filename;
        cout << "Enter filename: ";
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

    cout << "\nTokens:\n";
    cout << "-------\n";
    cout << "Type\t\tValue\t\tLine\n";
    cout << "-------\n";

    for (const Token& token : tokens) {
        cout << token.type << "\t\t" << token.value << "\t\t" << token.line << "\n";
    }

    generateSymbolTable(tokens);

    return 0;
}
