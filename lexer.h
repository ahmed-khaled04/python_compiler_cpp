#ifndef LEXER_H
#define LEXER_H

#include <QObject>
#include <QString>
#include <QSet>
#include <vector>
#include <unordered_map>

struct SymbolEntry {
    int id;
    std::vector<int> lines;
    QString type = "unknown";
    QString value = "undefined";
};

struct Token {
    QString type;
    QString value;
    int line;
};

enum class State {
    START,
    IN_IDENTIFIER,
    IN_NUMBER,
    IN_OPERATOR,
    IN_STRING,
    IN_COMMENT,
    IN_MULTILINE_STRING
};

class Lexer : public QObject
{
    Q_OBJECT

public:
    explicit Lexer(QObject *parent = nullptr);

    std::vector<Token> tokenize(const QString& source);
    std::unordered_map<QString, SymbolEntry> generateSymbolTable(const std::vector<Token>& tokens);

private:
    bool isKeyword(const QString& str);
    bool isOperator(const QString& str);
    bool isDelimiter(const QString& str);
    bool isIdentifier(const QString& str);
    bool isNumber(const QString& str);
    bool isHexDigit(QChar c);
    bool isBinaryDigit(QChar c);
    bool isOctalDigit(QChar c);

    const QSet<QString> KEYWORDS;
    const QSet<QString> OPERATORS;
    const QSet<QString> DELIMITERS;
};

#endif // LEXER_H
