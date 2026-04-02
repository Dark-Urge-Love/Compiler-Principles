#include "Lexer.h"
#include <cctype>
#include <algorithm>

using namespace std;

Token::Token(string t, TokenType ty) : text(t), type(ty) {
    typeName = getTypeName(ty);
}

// TokenType枚举值
string Token::getTypeName(TokenType ty) {
    switch (ty) {
    case TokenType::Keyword: return "关键字";
    case TokenType::Identifier: return "标识符";
    case TokenType::LiteralInteger: return "字面量（十进制整数）";
    case TokenType::LiteralHexInteger: return "字面量（十六进制整数）";
    case TokenType::LiteralOctInteger: return "字面量（八进制整数）";
    case TokenType::LiteralBinInteger: return "字面量（二进制整数）";
    case TokenType::LiteralFloat: return "字面量（浮点数）";
    case TokenType::LiteralString: return "字符串字面量";
    case TokenType::Comment: return "注释";
    case TokenType::Operator: return "操作符";
    case TokenType::AssignmentOperator: return "赋值操作符";
    case TokenType::Delimiter: return "分隔符";
    case TokenType::MacroCall: return "宏调用名";
    default: return "未知";
    }
}

// Rust的保留关键字
const map<string, TokenType> Lexer::keywords = {
    {"as", TokenType::Keyword}, {"break", TokenType::Keyword}, {"const", TokenType::Keyword},
    {"continue", TokenType::Keyword}, {"crate", TokenType::Keyword}, {"else", TokenType::Keyword},
    {"enum", TokenType::Keyword}, {"extern", TokenType::Keyword}, {"false", TokenType::Keyword},
    {"fn", TokenType::Keyword}, {"for", TokenType::Keyword}, {"if", TokenType::Keyword},
    {"impl", TokenType::Keyword}, {"in", TokenType::Keyword}, {"let", TokenType::Keyword},
    {"loop", TokenType::Keyword}, {"match", TokenType::Keyword}, {"mod", TokenType::Keyword},
    {"move", TokenType::Keyword}, {"mut", TokenType::Keyword}, {"pub", TokenType::Keyword},
    {"ref", TokenType::Keyword}, {"return", TokenType::Keyword}, {"self", TokenType::Keyword},
    {"Self", TokenType::Keyword}, {"static", TokenType::Keyword}, {"struct", TokenType::Keyword},
    {"super", TokenType::Keyword}, {"trait", TokenType::Keyword}, {"true", TokenType::Keyword},
    {"type", TokenType::Keyword}, {"unsafe", TokenType::Keyword}, {"use", TokenType::Keyword},
    {"where", TokenType::Keyword}, {"while", TokenType::Keyword}, {"async", TokenType::Keyword},
    {"await", TokenType::Keyword}, {"dyn", TokenType::Keyword}, {"abstract", TokenType::Keyword},
    {"become", TokenType::Keyword}, {"box", TokenType::Keyword}, {"do", TokenType::Keyword},
    {"final", TokenType::Keyword}, {"macro", TokenType::Keyword}, {"override", TokenType::Keyword},
    {"priv", TokenType::Keyword}, {"typeof", TokenType::Keyword}, {"unsized", TokenType::Keyword},
    {"virtual", TokenType::Keyword}, {"yield", TokenType::Keyword}, {"try", TokenType::Keyword}
};

// src: 待解析的源代码字符串；pos: 解析指针（初始为0，指向源代码第一个字符）
Lexer::Lexer(const string& src) : source(src), pos(0) {}

// 查看当前位置偏移offset后的字符
char Lexer::peek(size_t offset) const {
    // 检查偏移后是否超出源代码长度，超出则返回空字符
    if (pos + offset >= source.length())
        return '\0';
    // 返回偏移后的字符
    return source[pos + offset];
}

// 获取当前位置的字符，并将解析指针pos后移一位
char Lexer::get() {
    // 检查是否已到源代码末尾
    if (pos >= source.length())
        return '\0';
    // 返回当前字符，并将pos+1
    return source[pos++];
}

// 跳过所有空白字符
void Lexer::skipWhitespace() {
    while (isspace(peek())) pos++;
}

// 将源代码解析为Token列表
vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    while (pos < source.length()) {
        skipWhitespace();
        if (pos >= source.length()) break;

        char c = peek();
        // 字母或下划线开头 → 解析标识符/关键字/宏调用
        if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        // 数字开头 → 解析数字（整数/浮点数/进制数）
        else if (isdigit(c)) {
            tokens.push_back(readNumber());
        }
        // 双引号开头 → 解析字符串字面量
        else if (c == '"') {
            tokens.push_back(readString());
        }
        // 斜杠开头且下一个字符是//或/* → 解析注释
        else if (c == '/' && (peek(1) == '/' || peek(1) == '*')) {
            tokens.push_back(readComment());
        }
        // 其他字符 → 解析操作符/分隔符/赋值操作符
        else {
            tokens.push_back(readOperatorOrDelimiter());
        }
    }
    return tokens;
}

// 解析标识符、关键字或宏调用
Token Lexer::readIdentifierOrKeyword() {
    string text;
    while (isalnum(peek()) || peek() == '_' || peek() == '!') {
        text += get();
        // 若读取到!，说明是宏调用，终止读取（!必须是最后一个字符）
        if (text.back() == '!') break;
    }
    // 末尾是! → 宏调用Token
    if (text.back() == '!') {
        return Token(text, TokenType::MacroCall);
    }
    // 在关键字表中 → 关键字Token
    if (keywords.count(text)) {
        return Token(text, TokenType::Keyword);
    }
    // 普通标识符 → 标识符Token
    return Token(text, TokenType::Identifier);
}

// 解析数字
Token Lexer::readNumber() {
    string text;
    // 标记是否为浮点数
    bool isFloat = false;

    // 进制数（0x/0o/0b开头）
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'o' || peek(1) == 'b')) {
        text += get();
        char base = get();
        text += base;
        while (isxdigit(peek()) || peek() == '_') {
            text += get();
        }
        if (base == 'x' || base == 'X') {
            return Token(text, TokenType::LiteralHexInteger);
        } else if (base == 'o' || base == 'O') {
            return Token(text, TokenType::LiteralOctInteger);
        } else if (base == 'b' || base == 'B') {
            return Token(text, TokenType::LiteralBinInteger);
        }
        return Token(text, TokenType::LiteralInteger);
    }

    // 十进制整数/浮点数
    while (isdigit(peek()) || peek() == '.' || peek() == '_') {
        // 检测小数点：若下一个字符也是.，则是范围操作符（..），终止读取
        if (peek() == '.') {
            if (peek(1) == '.') break;
            isFloat = true;
        }
        text += get();
    }
    return Token(text, isFloat ? TokenType::LiteralFloat : TokenType::LiteralInteger);
}

// 解析字符串
Token Lexer::readString() {
    string text;
    // 读取第一个双引号（"）
    text += get();
    while (peek() != '"' && peek() != '\0') {
        // 处理转义字符（如\"、\\）：先读取\，再读取后续字符
        if (peek() == '\\') text += get();
        text += get();
    }
    if (peek() == '"') text += get();
    return Token(text, TokenType::LiteralString);
}

// 解析注释
Token Lexer::readComment() {
    string text;
    // 单行注释（//）
    if (peek() == '/' && peek(1) == '/') {
        while (peek() != '\n' && peek() != '\0') text += get();
    }
    // 多行注释（/* */）
    else if (peek() == '/' && peek(1) == '*') {
        text += get(); text += get();
        while (!(peek() == '*' && peek(1) == '/') && peek() != '\0') text += get();
        if (peek() == '*') { text += get(); text += get(); }
    }
    return Token(text, TokenType::Comment);
}

// 解析操作符、分隔符、赋值操作符
Token Lexer::readOperatorOrDelimiter() {
    string text;
    char c = get();
    text += c;

    char next = peek();
    if ((c == '=' && next == '=') || (c == '!' && next == '=') ||
        (c == '<' && next == '=') || (c == '>' && next == '=') ||
        (c == '+' && next == '=') || (c == '-' && next == '=') ||
        (c == '*' && next == '=') || (c == '/' && next == '=') ||
        (c == '&' && next == '&') || (c == '|' && next == '|') ||
        (c == '.' && next == '.')) {
        text += get();
    }

    // 分隔符（()[]{};, 或..）
    static const string delimiters = "()[]{};,";
    if (delimiters.find(c) != string::npos || text == "..") {
        return Token(text, TokenType::Delimiter);
    }

    // 赋值操作符（=、+=、-=、*=、/=、%=）
    if (text == "=" || text == "+=" || text == "-=" || text == "*=" || text == "/=" || text == "%=") {
        return Token(text, TokenType::AssignmentOperator);
    }

    // 普通操作符（如+、-、*、/、&、|等）
    return Token(text, TokenType::Operator);
}
