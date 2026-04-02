#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <map>

using namespace std;

enum class TokenType {
    Keyword,
    Identifier,
    LiteralInteger,
    LiteralHexInteger,
    LiteralOctInteger,
    LiteralBinInteger,
    LiteralFloat,
    LiteralString,
    Comment,
    Operator,
    AssignmentOperator,
    Delimiter,
    MacroCall,
    Unknown
};

struct Token {
    string text; //Token的文本内容
    TokenType type; //Token的类型
    string typeName; //Token类型的中文名称

    Token(string t, TokenType ty);
    static string getTypeName(TokenType ty);
};

class Lexer {
public:
    Lexer(const string& source);
    vector<Token> tokenize();

private:
    string source;
    size_t pos;

    char peek(size_t offset = 0) const;
    char get();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readString();
    Token readComment();
    Token readOperatorOrDelimiter();

    static const map<string, TokenType> keywords;
};

#endif // LEXER_H
