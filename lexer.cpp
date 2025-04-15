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
    "+", "-", "", "/", "%", "", "//", "=", "+=", "-=", "=", "/=",
    "%=", "=", "//=", "==", "!=", "<", ">", "<=", ">=", "&", "|",
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

vector<Token> tokenize(const string& source) {
    vector<Token> tokens;
    string currentToken;
    int lineNumber = 1;
    State state = State::START;
    char stringQuote = '\0';
    bool escapeNext = false;

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
                else if (isalpha(c) || c == '_') {
                    state = State::IN_IDENTIFIER;
                    currentToken += c;
                }
                else if (isdigit(c)) {
                    state = State::IN_NUMBER;
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
                else if (isOperator(string(1, c))) {
                    // Check for multi-character operators
                    state = State::IN_OPERATOR;
                    currentToken += c;
                }
                else if (isDelimiter(string(1, c))) {
                    tokens.push_back({"DELIMITER", string(1, c), lineNumber});
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
                if (isdigit(c) || c == '.') {
                    currentToken += c;
                } else {
                    flushCurrentToken();
                    state = State::START;
                    i--; // Reprocess this character
                }
                break;

            case State::IN_OPERATOR:
                if (isOperator(currentToken + c)) {
                    currentToken += c;
                } else {
                    flushCurrentToken();
                    state = State::START;
                    i--; // Reprocess this character
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
                    tokens.push_back({"STRING_LITERAL", currentToken, lineNumber});
                    currentToken.clear();
                    state = State::START;
                    i += 2; // Skip the closing quotes
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
