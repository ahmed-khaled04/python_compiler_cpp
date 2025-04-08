#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <iomanip>

using namespace std;

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
    "+", "-", "*", "/", "%", "**", "//", "=", "+=", "-=", "*=", "/=",
    "%=", "**=", "//=", "==", "!=", "<", ">", "<=", ">=", "&", "|",
    "^", "~", "<<", ">>", "and", "or", "not", "is"
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

    auto flushCurrentToken = [&]() {
        if (!currentToken.empty()) {
            string type;
            if (isKeyword(currentToken)) type = "KEYWORD";
            else if (isOperator(currentToken)) type = "OPERATOR";
            else if (isNumber(currentToken)) type = "NUMBER";
            else if (isIdentifier(currentToken)) type = "IDENTIFIER";
            else type = "UNKNOWN";

            tokens.push_back({ type, currentToken, lineNumber });
            currentToken.clear();
        }
        };

    for (size_t i = 0; i < source.size(); i++) {
        char c = source[i];

        // Handle multi-line comments (''' or """)
        if (!inString && !inComment && !inMultiLineComment &&
            (c == '\'' || c == '"') && i + 2 < source.size() &&
            source[i + 1] == c && source[i + 2] == c) {
            inMultiLineComment = true;
            multiLineQuote = c;
            i += 2;
            continue;
        }

        if (inMultiLineComment && c == multiLineQuote &&
            i + 2 < source.size() && source[i + 1] == multiLineQuote &&
            source[i + 2] == multiLineQuote) {
            inMultiLineComment = false;
            i += 2;
            continue;
        }

        if (inMultiLineComment) {
            if (c == '\n') lineNumber++;
            continue;
        }

        if (c == '#' && !inString) {
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
        else if (inString && c == stringQuote && (i == 0 || source[i - 1] != '\\')) {
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

        // Check if we need to split a keyword from following identifier
        if (!currentToken.empty() && (isalpha(c) || c == '_')) {
            if (isKeyword(currentToken)) {
                tokens.push_back({ "KEYWORD", currentToken, lineNumber });
                currentToken.clear();
            }
        }
        // Handle floating-point numbers like 10.5
        if (isdigit(c)) {
            size_t j = i;
            string number;
            bool hasDecimal = false;

            while (j < source.size() && (isdigit(source[j]) || (source[j] == '.' && !hasDecimal))) {
                if (source[j] == '.') hasDecimal = true;
                number += source[j];
                j++;
            }

            if (number.size() > 1 && isNumber(number)) {
                flushCurrentToken();
                tokens.push_back({ "NUMBER", number, lineNumber });
                i = j - 1;
                continue;
            }
        }


        // Handle delimiters
        if (isDelimiter(string(1, c))) {
            if (!currentToken.empty()) {
                if (isKeyword(currentToken))
                    tokens.push_back({ "KEYWORD", currentToken, lineNumber });
                else if (isOperator(currentToken))
                    tokens.push_back({ "OPERATOR", currentToken, lineNumber });
                else if (isNumber(currentToken))
                    tokens.push_back({ "NUMBER", currentToken, lineNumber });
                else if (isIdentifier(currentToken))
                    tokens.push_back({ "IDENTIFIER", currentToken, lineNumber });
                else
                    tokens.push_back({ "UNKNOWN", currentToken, lineNumber });
                currentToken.clear();
            }
            tokens.push_back({ "DELIMITER", string(1, c), lineNumber });
            continue;
        }

        // Helper function to flush currentToken
        

        // Handle operators
        if (i + 2 < source.size()) {  // Check for 3-character operators first
            string threeCharOp = string(1, c) + source[i + 1] + source[i + 2];
            if (isOperator(threeCharOp)) {
                flushCurrentToken();  // Flush any accumulated token
                tokens.push_back({ "OPERATOR", threeCharOp, lineNumber });
                i += 2;  // Skip next two characters
                continue;
            }
        }

        if (i + 1 < source.size()) {  // Then check 2-character operators
            string twoCharOp = string(1, c) + source[i + 1];
            if (isOperator(twoCharOp)) {
                flushCurrentToken();  // Flush any accumulated token
                tokens.push_back({ "OPERATOR", twoCharOp, lineNumber });
                i++;  // Skip next character
                continue;
            }
        }

        // Finally check single-character operators
        string opStr(1, c);
        if (isOperator(opStr)) {
            flushCurrentToken();  // Flush any accumulated token
            tokens.push_back({ "OPERATOR", opStr, lineNumber });
            continue;
        }


        currentToken += c;
    }

    // Final token
    if (!currentToken.empty()) {
        if (isKeyword(currentToken))
            tokens.push_back({ "KEYWORD", currentToken, lineNumber });
        else if (isOperator(currentToken))
            tokens.push_back({ "OPERATOR", currentToken, lineNumber });
        else if (isDelimiter(currentToken))
            tokens.push_back({ "DELIMITER", currentToken, lineNumber });
        else if (isNumber(currentToken))
            tokens.push_back({ "NUMBER", currentToken, lineNumber });
        else if (isIdentifier(currentToken))
            tokens.push_back({ "IDENTIFIER", currentToken, lineNumber });
        else
            tokens.push_back({ "UNKNOWN", currentToken, lineNumber });
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
    struct SymbolEntry {
        int id;
        vector<int> lines;
    };

    unordered_map<string, SymbolEntry> symbolTable;
    int currentId = 1;

    for (const Token& token : tokens) {
        if (token.type == "IDENTIFIER") {
            if (symbolTable.find(token.value) == symbolTable.end()) {
                symbolTable[token.value] = { currentId++, {token.line} };
            }
            else {
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

    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
        << "-+-" << string(linesColWidth, '-') << "-+" << endl;
    cout << "| " << left << setw(idColWidth) << "ID" << " | "
        << setw(nameColWidth) << "IDENTIFIER" << " | "
        << setw(linesColWidth) << "LINES" << " |" << endl;
    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
        << "-+-" << string(linesColWidth, '-') << "-+" << endl;

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

    cout << "+-" << string(idColWidth, '-') << "-+-" << string(nameColWidth, '-')
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
