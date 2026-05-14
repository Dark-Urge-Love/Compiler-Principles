#ifndef COMPILER_H
#define COMPILER_H

#include <string>
#include <vector>
#include <memory>
#include <map>

using namespace std;

enum class TokenType {
    // Reserved Words
    IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE, WHILE, ENDWHILE, FOR, ENDFOR, DO,
    // Multicharacter
    ID, NUM,
    // Special Symbols
    ASSIGN, EQ, LT, LE, GT, GE, NE, PLUS, MINUS, TIMES, OVER, MOD, POWER, LPAREN, RPAREN, SEMI, PLUSPLUS, MINUSMINUS,
    // Regular Expression Symbols
    REG_OR, REG_AND, REG_EMPTY, REG_QUERY, REG_ASSIGN,
    // Book-keeping
    ENDFILE, ERROR
};

struct Token {
    TokenType type;
    string value;
    int line;
};

enum class NodeKind { StmtK, ExpK };
enum class StmtKind { IfK, RepeatK, AssignK, ReadK, WriteK, WhileK, ForK, IncrK, RegexK };
enum class ExpKind { OpK, ConstK, IdK, RegexExpK };

class ASTNode {
public:
    NodeKind nodeKind;
    int line;
    vector<shared_ptr<ASTNode>> children;
    shared_ptr<ASTNode> sibling;

    ASTNode(NodeKind kind, int ln) : nodeKind(kind), line(ln), sibling(nullptr) {}
    virtual ~ASTNode() = default;
};

class StmtNode : public ASTNode {
public:
    StmtKind stmtKind;
    string name;
    StmtNode(StmtKind kind, int ln) : ASTNode(NodeKind::StmtK, ln), stmtKind(kind) {}
};

class ExpNode : public ASTNode {
public:
    ExpKind expKind;
    TokenType op;
    int val;
    string name;
    ExpNode(ExpKind kind, int ln) : ASTNode(NodeKind::ExpK, ln), expKind(kind) {}
};

struct ErrorInfo {
    string message;
    int line;
    int column;
};

class Lexer {
public:
    Lexer(const string& source);
    Token getToken();
    vector<Token> tokenize();
    vector<ErrorInfo> getErrors() const { return errors; }

private:
    string source;
    size_t pos;
    int line;
    vector<ErrorInfo> errors;
    static map<string, TokenType> reservedWords;

    char peek();
    char advance();
    void skipWhitespace();
};

class Parser {
public:
    Parser(const vector<Token>& tokens);
    shared_ptr<ASTNode> parse();
    vector<ErrorInfo> getErrors() const { return errors; }

private:
    vector<Token> tokens;
    size_t pos;
    vector<ErrorInfo> errors;

    Token match(TokenType expected);
    Token peek();
    Token advance();
    bool check(TokenType type);
    void reportError(const string& message);

    shared_ptr<ASTNode> stmt_sequence();
    shared_ptr<ASTNode> statement();
    shared_ptr<ASTNode> if_stmt();
    shared_ptr<ASTNode> repeat_stmt();
    shared_ptr<ASTNode> assign_stmt();
    shared_ptr<ASTNode> regex_stmt();
    shared_ptr<ASTNode> read_stmt();
    shared_ptr<ASTNode> write_stmt();
    shared_ptr<ASTNode> while_stmt();
    shared_ptr<ASTNode> for_stmt();
    shared_ptr<ASTNode> increment_stmt();
    shared_ptr<ASTNode> exp();
    shared_ptr<ASTNode> simple_exp();
    shared_ptr<ASTNode> term();
    shared_ptr<ASTNode> power();
    shared_ptr<ASTNode> factor();

    shared_ptr<ASTNode> regex_exp();
    shared_ptr<ASTNode> regex_term();
    shared_ptr<ASTNode> regex_factor();
};

#endif
