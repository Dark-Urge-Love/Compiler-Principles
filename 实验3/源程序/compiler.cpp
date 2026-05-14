#include "compiler.h"
#include <iostream>
#include <cctype>

using namespace std;

map<string, TokenType> Lexer::reservedWords = {
    {"if", TokenType::IF}, {"then", TokenType::THEN}, {"else", TokenType::ELSE},
    {"end", TokenType::END}, {"repeat", TokenType::REPEAT}, {"until", TokenType::UNTIL},
    {"read", TokenType::READ}, {"write", TokenType::WRITE}, {"while", TokenType::WHILE},
    {"endwhile", TokenType::ENDWHILE}, {"for", TokenType::FOR}, {"endfor", TokenType::ENDFOR},
    {"do", TokenType::DO}
};

Lexer::Lexer(const string& src) : source(src), pos(0), line(1) {}

char Lexer::peek() {
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    if (pos >= source.length()) return '\0';
    char c = source[pos++];
    if (c == '\n') line++;
    return c;
}

void Lexer::skipWhitespace() {
    while (isspace(peek()) || peek() == '{') {
        if (peek() == '{') {
            while (peek() != '\0' && peek() != '}') advance();
            if (peek() == '}') advance();
        } else {
            advance();
        }
    }
}

Token Lexer::getToken() {
    skipWhitespace();
    if (peek() == '\0') return {TokenType::ENDFILE, "", line};

    char c = advance();
    if (isalpha(c)) {
        string s;
        s += c;
        while (isalnum(peek())) s += advance();
        if (reservedWords.count(s)) return {reservedWords[s], s, line};
        return {TokenType::ID, s, line};
    }
    if (isdigit(c)) {
        string s;
        s += c;
        while (isdigit(peek())) s += advance();
        return {TokenType::NUM, s, line};
    }

    switch (c) {
        case '+':
            if (peek() == '+') { advance(); return {TokenType::PLUSPLUS, "++", line}; }
            return {TokenType::PLUS, "+", line};
        case '-':
            if (peek() == '-') { advance(); return {TokenType::MINUSMINUS, "--", line}; }
            return {TokenType::MINUS, "-", line};
        case '*': return {TokenType::TIMES, "*", line};
        case '/': return {TokenType::OVER, "/", line};
        case '%': return {TokenType::MOD, "%", line};
        case '^': return {TokenType::POWER, "^", line};
        case '(': return {TokenType::LPAREN, "(", line};
        case ')': return {TokenType::RPAREN, ")", line};
        case '|': return {TokenType::REG_OR, "|", line};
        case '&': return {TokenType::REG_AND, "&", line};
        case '#': return {TokenType::REG_EMPTY, "#", line};
        case '?': return {TokenType::REG_QUERY, "?", line};
        case ';': return {TokenType::SEMI, ";", line};
        case ':':
            if (peek() == '=') { advance(); return {TokenType::ASSIGN, ":=", line}; }
            if (peek() == ':') {
                advance();
                if (peek() == '=') { advance(); return {TokenType::REG_ASSIGN, "::=", line}; }
                return {TokenType::ERROR, "::", line};
            }
            return {TokenType::ERROR, ":", line};
        case '<':
            if (peek() == '=') { advance(); return {TokenType::LE, "<=", line}; }
            if (peek() == '>') { advance(); return {TokenType::NE, "<>", line}; }
            return {TokenType::LT, "<", line};
        case '>':
            if (peek() == '=') { advance(); return {TokenType::GE, ">=", line}; }
            return {TokenType::GT, ">", line};
        case '=':
            if (peek() == '=') { advance(); return {TokenType::EQ, "==", line}; }
            return {TokenType::EQ, "=", line};
        case '!':
            if (peek() == '=') { advance(); return {TokenType::NE, "!=", line}; }
            return {TokenType::ERROR, "!", line};
        default: return {TokenType::ERROR, string(1, c), line};
    }
}

vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    Token t;
    do {
        t = getToken();
        if (t.type == TokenType::ERROR) {
            errors.push_back({"词法错误: 未知符号 '" + t.value + "'", t.line, 0});
        }
        tokens.push_back(t);
    } while (t.type != TokenType::ENDFILE);
    return tokens;
}

Parser::Parser(const vector<Token>& tks) : tokens(tks), pos(0) {}

Token Parser::peek() { return tokens[pos]; }
Token Parser::advance() { if (pos < tokens.size() - 1) return tokens[pos++]; return tokens[pos]; }
bool Parser::check(TokenType type) { return peek().type == type; }

Token Parser::match(TokenType expected) {
    if (peek().type == expected) return advance();
    
    string msg = "语法错误: 期待 ";
    if (expected == TokenType::ID) msg += "标识符";
    else if (expected == TokenType::NUM) msg += "数字";
    else if (expected == TokenType::ASSIGN) msg += "':='";
    else if (expected == TokenType::THEN) msg += "'then'";
    else if (expected == TokenType::END) msg += "'end'";
    else if (expected == TokenType::UNTIL) msg += "'until'";
    else if (expected == TokenType::RPAREN) msg += "')'";
    else if (expected == TokenType::SEMI) msg += "';'";
    else msg += "符号";
    
    msg += ", 实际为 '" + peek().value + "'";
    reportError(msg);
    return {TokenType::ERROR, "", peek().line};
}

void Parser::reportError(const string& message) {
    errors.push_back({message, peek().line, 0});
}

shared_ptr<ASTNode> Parser::parse() {
    return stmt_sequence();
}

shared_ptr<ASTNode> Parser::stmt_sequence() {
    auto t = statement();
    auto p = t;
    while (peek().type != TokenType::ENDFILE && 
           peek().type != TokenType::END && 
           peek().type != TokenType::ELSE && 
           peek().type != TokenType::UNTIL &&
           peek().type != TokenType::ENDWHILE &&
           peek().type != TokenType::ENDFOR) {
        match(TokenType::SEMI);
        auto q = statement();
        if (q) {
            if (!t) t = p = q;
            else {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}

shared_ptr<ASTNode> Parser::statement() {
    switch (peek().type) {
        case TokenType::IF: return if_stmt();
        case TokenType::REPEAT: return repeat_stmt();
        case TokenType::ID: {
            if (pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::REG_ASSIGN)
                return regex_stmt();
            return assign_stmt();
        }
        case TokenType::READ: return read_stmt();
        case TokenType::WRITE: return write_stmt();
        case TokenType::WHILE: return while_stmt();
        case TokenType::FOR: return for_stmt();
        case TokenType::PLUSPLUS:
        case TokenType::MINUSMINUS: return increment_stmt();
        default: return nullptr;
    }
}

shared_ptr<ASTNode> Parser::if_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::IfK, peek().line);
    match(TokenType::IF);
    node->children.push_back(exp());
    match(TokenType::THEN);
    node->children.push_back(stmt_sequence());
    if (peek().type == TokenType::ELSE) {
        match(TokenType::ELSE);
        node->children.push_back(stmt_sequence());
    }
    match(TokenType::END);
    return node;
}

shared_ptr<ASTNode> Parser::repeat_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::RepeatK, peek().line);
    match(TokenType::REPEAT);
    node->children.push_back(stmt_sequence());
    match(TokenType::UNTIL);
    node->children.push_back(exp());
    return node;
}

shared_ptr<ASTNode> Parser::assign_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::AssignK, peek().line);
    node->name = match(TokenType::ID).value;
    match(TokenType::ASSIGN);
    node->children.push_back(exp());
    return node;
}

shared_ptr<ASTNode> Parser::regex_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::RegexK, peek().line);
    node->name = match(TokenType::ID).value;
    match(TokenType::REG_ASSIGN);
    node->children.push_back(regex_exp());
    return node;
}

shared_ptr<ASTNode> Parser::read_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::ReadK, peek().line);
    match(TokenType::READ);
    node->name = match(TokenType::ID).value;
    return node;
}

shared_ptr<ASTNode> Parser::write_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::WriteK, peek().line);
    match(TokenType::WRITE);
    node->children.push_back(exp());
    return node;
}

shared_ptr<ASTNode> Parser::while_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::WhileK, peek().line);
    match(TokenType::WHILE);
    match(TokenType::LPAREN);
    node->children.push_back(exp());
    match(TokenType::RPAREN);
    node->children.push_back(stmt_sequence());
    match(TokenType::ENDWHILE);
    return node;
}

shared_ptr<ASTNode> Parser::for_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::ForK, peek().line);
    match(TokenType::FOR);
    match(TokenType::LPAREN);
    node->children.push_back(assign_stmt());
    match(TokenType::SEMI);
    node->children.push_back(exp());
    match(TokenType::SEMI);
    node->children.push_back(statement());
    if (peek().type == TokenType::SEMI) match(TokenType::SEMI);
    match(TokenType::RPAREN);
    node->children.push_back(statement());
    if (peek().type == TokenType::ENDFOR) match(TokenType::ENDFOR);
    return node;
}

shared_ptr<ASTNode> Parser::increment_stmt() {
    auto node = make_shared<StmtNode>(StmtKind::IncrK, peek().line);
    node->name = advance().value;
    auto id = match(TokenType::ID).value;
    auto idNode = make_shared<ExpNode>(ExpKind::IdK, node->line);
    idNode->name = id;
    node->children.push_back(idNode);
    return node;
}

shared_ptr<ASTNode> Parser::exp() {
    auto t = simple_exp();
    if (peek().type == TokenType::LT || peek().type == TokenType::LE || 
        peek().type == TokenType::GT || peek().type == TokenType::GE || 
        peek().type == TokenType::EQ || peek().type == TokenType::NE) {
        auto node = make_shared<ExpNode>(ExpKind::OpK, peek().line);
        node->op = advance().type;
        node->children.push_back(t);
        node->children.push_back(simple_exp());
        t = node;
    }
    return t;
}

shared_ptr<ASTNode> Parser::simple_exp() {
    auto t = term();
    while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
        auto node = make_shared<ExpNode>(ExpKind::OpK, peek().line);
        node->op = advance().type;
        node->children.push_back(t);
        node->children.push_back(term());
        t = node;
    }
    return t;
}

shared_ptr<ASTNode> Parser::term() {
    auto t = power();
    while (peek().type == TokenType::TIMES || peek().type == TokenType::OVER || peek().type == TokenType::MOD) {
        auto node = make_shared<ExpNode>(ExpKind::OpK, peek().line);
        node->op = advance().type;
        node->children.push_back(t);
        node->children.push_back(power());
        t = node;
    }
    return t;
}

shared_ptr<ASTNode> Parser::power() {
    auto t = factor();
    if (peek().type == TokenType::POWER) {
        auto node = make_shared<ExpNode>(ExpKind::OpK, peek().line);
        node->op = advance().type;
        node->children.push_back(t);
        node->children.push_back(power());
        t = node;
    }
    return t;
}

shared_ptr<ASTNode> Parser::factor() {
    switch (peek().type) {
        case TokenType::NUM: {
            auto node = make_shared<ExpNode>(ExpKind::ConstK, peek().line);
            node->val = stoi(advance().value);
            return node;
        }
        case TokenType::ID: {
            auto node = make_shared<ExpNode>(ExpKind::IdK, peek().line);
            node->name = advance().value;
            return node;
        }
        case TokenType::LPAREN: {
            match(TokenType::LPAREN);
            auto node = exp();
            match(TokenType::RPAREN);
            return node;
        }
        default: return nullptr;
    }
}

shared_ptr<ASTNode> Parser::regex_exp() {
    auto t = regex_term();
    while (peek().type == TokenType::REG_OR) {
        auto node = make_shared<ExpNode>(ExpKind::RegexExpK, peek().line);
        node->op = advance().type;
        node->children.push_back(t);
        node->children.push_back(regex_term());
        t = node;
    }
    return t;
}

shared_ptr<ASTNode> Parser::regex_term() {
    auto t = regex_factor();
    while (peek().type == TokenType::REG_AND) {
        auto node = make_shared<ExpNode>(ExpKind::RegexExpK, peek().line);
        node->op = advance().type;
        node->children.push_back(t);
        node->children.push_back(regex_factor());
        t = node;
    }
    return t;
}

shared_ptr<ASTNode> Parser::regex_factor() {
    shared_ptr<ASTNode> t = nullptr;
    switch (peek().type) {
        case TokenType::ID: {
            auto node = make_shared<ExpNode>(ExpKind::IdK, peek().line);
            node->name = advance().value;
            t = node;
            break;
        }
        case TokenType::REG_EMPTY: {
            auto node = make_shared<ExpNode>(ExpKind::RegexExpK, peek().line);
            node->op = advance().type;
            t = node;
            break;
        }
        case TokenType::LPAREN: {
            match(TokenType::LPAREN);
            t = regex_exp();
            match(TokenType::RPAREN);
            break;
        }
        default: return nullptr;
    }
    
    if (peek().type == TokenType::REG_QUERY) {
        auto node = make_shared<ExpNode>(ExpKind::RegexExpK, peek().line);
        node->op = advance().type;
        node->children.push_back(t);
        t = node;
    }
    return t;
}
