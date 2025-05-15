#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>


// Token structure
struct Token {
    std::string type;  // Type of the token (e.g., KEYWORD, IDENTIFIER, etc.)
    std::string value; // Value of the token
    int line;          // Line number where the token appears
};

// Symbol table entry structure
struct SymbolEntry {
    int id;
    std::vector<int> lines;
    std::string type = "unknown";  // Data type
    std::string value = "undefined";  // Assigned value
    std::string scope = "global";    // Scope of the identifier
};

// Custom hash function for std::pair
struct PairHash {
    template <typename T1, typename T2>
    size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1); // Combine the two hashes
    }
};

// Function declarations
std::vector<Token> tokenize(const std::string& source);
void generateSymbolTable(const std::vector<Token>& tokens);
void printTokenTable(const std::vector<Token>& tokens);

bool isKeyword(const std::string& str);
bool isOperator(const std::string& str);
bool isDelimiter(const std::string& str);
bool isIdentifier(const std::string& str);
bool isNumber(const std::string& str);

#endif // LEXICAL_ANALYZER_H