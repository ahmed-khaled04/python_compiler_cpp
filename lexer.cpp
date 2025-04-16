#include "lexer.h"
#include <QChar>
#include <algorithm>

Lexer::Lexer(QObject *parent) : QObject(parent),
    KEYWORDS({
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield"
    }),
    OPERATORS({
        "+", "-", "*", "/", "%", "**", "//", "=", "+=", "-=", "*=", "/=",
        "%=", "**=", "//=", "==", "!=", "<", ">", "<=", ">=", "&", "|",
        "^", "~", "<<", ">>", "and", "or", "not", "is", ":="
    }),
    DELIMITERS({
        "(", ")", "[", "]", "{", "}", ",", ":", ".", ";", "@", "..."
    })
{
}

bool Lexer::isKeyword(const QString& str) {
    return KEYWORDS.contains(str);
}

bool Lexer::isOperator(const QString& str) {
    return OPERATORS.contains(str);
}

bool Lexer::isDelimiter(const QString& str) {
    return DELIMITERS.contains(str);
}

bool Lexer::isIdentifier(const QString& str) {
    if (str.isEmpty() || str[0].isDigit()) return false;
    for (QChar c : str) {
        if (!c.isLetterOrNumber() && c != '_') return false;
    }
    return true;
}

bool Lexer::isHexDigit(QChar c) {
    return c.isDigit() || (c.toLower() >= 'a' && c.toLower() <= 'f');
}

bool Lexer::isBinaryDigit(QChar c) {
    return c == '0' || c == '1';
}

bool Lexer::isOctalDigit(QChar c) {
    return c >= '0' && c <= '7';
}

bool Lexer::isNumber(const QString& str) {
    if (str.isEmpty()) return false;

    // Check for complex numbers
    if (str.back().toLower() == 'j') {
        if (str.length() == 1) return false;
        QString realPart = str.left(str.length() - 1);
        return isNumber(realPart);
    }

    // Check for hexadecimal
    if (str.length() > 2 && str[0] == '0' && str[1].toLower() == 'x') {
        for (int i = 2; i < str.length(); i++) {
            if (!isHexDigit(str[i])) return false;
        }
        return str.length() > 2;
    }

    // Check for binary
    if (str.length() > 2 && str[0] == '0' && str[1].toLower() == 'b') {
        for (int i = 2; i < str.length(); i++) {
            if (!isBinaryDigit(str[i])) return false;
        }
        return str.length() > 2;
    }

    // Check for octal
    if (str.length() > 2 && str[0] == '0' && str[1].toLower() == 'o') {
        for (int i = 2; i < str.length(); i++) {
            if (!isOctalDigit(str[i])) return false;
        }
        return str.length() > 2;
    }

    // Regular decimal numbers
    bool hasDecimal = false;
    bool hasExponent = false;
    bool needsExponentDigit = false;

    for (int i = 0; i < str.length(); i++) {
        QChar c = str[i];

        if (c.isDigit()) {
            if (needsExponentDigit) needsExponentDigit = false;
            continue;
        }

        if (c == '.') {
            if (hasDecimal || hasExponent) return false;
            hasDecimal = true;
        }
        else if (c.toLower() == 'e') {
            if (hasExponent) return false;
            hasExponent = true;
            needsExponentDigit = true;
            if (i+1 < str.length() && (str[i+1] == '+' || str[i+1] == '-')) {
                i++;
            }
        }
        else {
            return false;
        }
    }

    return !needsExponentDigit;
}

std::vector<Token> Lexer::tokenize(const QString& source) {
    std::vector<Token> tokens;
    QString currentToken;
    int lineNumber = 1;
    State state = State::START;
    QChar stringQuote;
    bool escapeNext = false;
    QString lastTokenType;
    bool inFunctionDef = false;
    bool inClassDef = false;

    auto flushCurrentToken = [&]() {
        if (!currentToken.isEmpty()) {
            QString type;
            if (state == State::IN_IDENTIFIER) {
                type = isKeyword(currentToken) ? "KEYWORD" : "IDENTIFIER";
                if (currentToken == "def") inFunctionDef = true;
                if (currentToken == "class") inClassDef = true;
            }
            else if (state == State::IN_NUMBER) {
                type = "NUMBER";
            }
            else if (state == State::IN_OPERATOR) {
                type = "OPERATOR";
                inFunctionDef = false;
                inClassDef = false;
            }

            if (!type.isEmpty()) {
                tokens.push_back({type, currentToken, lineNumber});
                lastTokenType = type;
            }
            currentToken.clear();
        }
    };

    for (int i = 0; i < source.length(); i++) {
        QChar c = source[i];

        if (c == '\n') {
            lineNumber++;
            inFunctionDef = false;
            inClassDef = false;
        }

        switch (state) {
        case State::START:
            if (c.isSpace()) {
                continue;
            }
            else if (c == '.' && i + 2 < source.length() &&
                     source[i+1] == '.' && source[i+2] == '.') {
                tokens.push_back({"ELLIPSIS", "...", lineNumber});
                i += 2;
            }
            else if (c.isLetter() || c == '_') {
                state = State::IN_IDENTIFIER;
                currentToken += c;
            }
            else if (c.isDigit()) {
                state = State::IN_NUMBER;
                currentToken += c;
            }
            else if (c == ':') {
                state = State::IN_OPERATOR;
                currentToken += c;
            }
            else if (isOperator(QString(c))) {
                state = State::IN_OPERATOR;
                currentToken += c;
            }
            else if (c == '\'' || c == '"') {
                // Check for triple-quoted string/comment
                if (i + 2 < source.length() && source[i+1] == c && source[i+2] == c) {
                    // Check if this is a docstring (after def/class)
                    if (inFunctionDef || inClassDef) {
                        state = State::IN_MULTILINE_STRING;
                        stringQuote = c;
                        i += 2;
                        currentToken = QString(3, c);
                    } else {
                        // Treat as comment - skip until closing quotes
                        state = State::IN_COMMENT;
                        stringQuote = c;
                        i += 2;
                        while (i < source.length() - 2 &&
                               !(source[i] == stringQuote &&
                                 source[i+1] == stringQuote &&
                                 source[i+2] == stringQuote)) {
                            if (source[i] == '\n') lineNumber++;
                            i++;
                        }
                        i += 2; // Skip closing quotes
                        state = State::START;
                    }
                } else {
                    // Single-quoted string
                    state = State::IN_STRING;
                    stringQuote = c;
                    currentToken += c;
                }
            }
            else if (c == '#') {
                state = State::IN_COMMENT;
            }
            else if (c == '-' && i + 1 < source.length() && source[i+1].isDigit()) {
                if (lastTokenType.isEmpty() || lastTokenType == "OPERATOR") {
                    state = State::IN_NUMBER;
                    currentToken += c;
                } else {
                    state = State::IN_OPERATOR;
                    currentToken += c;
                }
            }
            else if (isDelimiter(QString(c))) {
                tokens.push_back({"DELIMITER", QString(c), lineNumber});
                inFunctionDef = false;
                inClassDef = false;
            }
            break;

        case State::IN_IDENTIFIER:
            if (c.isLetterOrNumber() || c == '_') {
                currentToken += c;
            } else {
                flushCurrentToken();
                state = State::START;
                i--;
            }
            break;

        case State::IN_NUMBER:
            if (c == '.' && i + 2 < source.length() &&
                source[i+1] == '.' && source[i+2] == '.') {
                if (isNumber(currentToken)) {
                    tokens.push_back({"NUMBER", currentToken, lineNumber});
                }
                currentToken.clear();
                state = State::START;
                i--;
                break;
            }

            if (c.isDigit() || c == '.' || c.toLower() == 'e' ||
                (c.toLower() == 'x' && currentToken == "0") ||
                (c.toLower() == 'b' && currentToken == "0") ||
                (c.toLower() == 'o' && currentToken == "0") ||
                (c.toLower() == 'j' && !currentToken.isEmpty()) ||
                (currentToken.length() >= 2 &&
                 currentToken[0] == '0' &&
                 currentToken[1].toLower() == 'x' &&
                 isHexDigit(c))) {

                if (c.toLower() == 'e' && i + 1 < source.length() &&
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
                if (isNumber(currentToken)) {
                    tokens.push_back({"NUMBER", currentToken, lineNumber});
                    currentToken.clear();
                    state = State::START;
                    i--;
                }
                else {
                    bool found = false;
                    for (int len = currentToken.length(); len > 0; len--) {
                        QString prefix = currentToken.left(len);
                        if (isNumber(prefix)) {
                            tokens.push_back({"NUMBER", prefix, lineNumber});
                            currentToken = currentToken.mid(len);
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        state = State::IN_IDENTIFIER;
                    }
                    i--;
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
            if (c == stringQuote && i + 2 < source.length() &&
                source[i+1] == stringQuote && source[i+2] == stringQuote) {
                currentToken += QString(3, stringQuote);
                tokens.push_back({"STRING_LITERAL", currentToken, lineNumber});
                currentToken.clear();
                state = State::START;
                i += 2;
                inFunctionDef = false;
                inClassDef = false;
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
            if (currentToken == ":" && c == '=') {
                currentToken += c;
                tokens.push_back({"OPERATOR", currentToken, lineNumber});
                currentToken.clear();
                state = State::START;
            }
            else if (isOperator(currentToken + c)) {
                currentToken += c;
            }
            else {
                tokens.push_back({"OPERATOR", currentToken, lineNumber});
                currentToken.clear();
                state = State::START;
                i--;
            }
            break;
        }
    }

    flushCurrentToken();
    return tokens;
}

std::unordered_map<QString, SymbolEntry> Lexer::generateSymbolTable(const std::vector<Token>& tokens) {
    std::unordered_map<QString, SymbolEntry> symbolTable;
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

        if (token.type == "IDENTIFIER" && i + 1 < tokens.size() && tokens[i+1].value == "=") {
            QString identifier = token.value;
            const Token& valueToken = tokens[i+2];

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
                if (symbolTable.find(valueToken.value) != symbolTable.end()) {
                    symbolTable[identifier].type = symbolTable[valueToken.value].type;
                    symbolTable[identifier].value = symbolTable[valueToken.value].value;
                }
            }
        }

        if (token.value == "def" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            symbolTable[tokens[i+1].value].type = "function";
            symbolTable[tokens[i+1].value].value = "function";
        }

        if (token.value == "class" && i + 1 < tokens.size() && tokens[i+1].type == "IDENTIFIER") {
            symbolTable[tokens[i+1].value].type = "class";
            symbolTable[tokens[i+1].value].value = "class";
        }
    }

    return symbolTable;
}
