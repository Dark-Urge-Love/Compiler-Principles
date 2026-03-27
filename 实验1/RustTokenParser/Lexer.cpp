#include "Lexer.h"
#include <cctype>
#include <algorithm>

using namespace std;

// t: 令牌的文本内容（如"let"、"123"）；ty: 令牌的类型枚举值
Token::Token(string t, TokenType ty) : text(t), type(ty) {
    // 将枚举类型转换为可读的类型名称（如"关键字"）
    typeName = getTypeName(ty);
}

// 根据TokenType枚举值，返回对应的中文类型名称（便于调试/输出）
string Token::getTypeName(TokenType ty) {
    switch (ty) {
    case TokenType::Keyword: return "关键字";                // 关键字（如let、fn）
    case TokenType::Identifier: return "标识符";            // 标识符（变量/函数名等）
    case TokenType::LiteralInteger: return "字面量（十进制整数）"; // 十进制整数（如123、45_67）
    case TokenType::LiteralHexInteger: return "字面量（十六进制整数）"; // 十六进制（0x1a、0X2B）
    case TokenType::LiteralOctInteger: return "字面量（八进制整数）";   // 八进制（0o123、0O456）
    case TokenType::LiteralBinInteger: return "字面量（二进制整数）";   // 二进制（0b101、0B110）
    case TokenType::LiteralFloat: return "字面量（浮点数）";         // 浮点数（如3.14、123.45_67）
    case TokenType::LiteralString: return "字符串字面量";          // 字符串（如"hello"、"rust"）
    case TokenType::Comment: return "注释";                  // 注释（// 单行 或 /* 多行 */）
    case TokenType::Operator: return "操作符";               // 操作符（+、-、==、&&等）
    case TokenType::AssignmentOperator: return "赋值操作符";      // 赋值操作符（=、+=、*=等）
    case TokenType::Delimiter: return "分隔符";              // 分隔符（()、[]、{}、;、,、..等）
    case TokenType::MacroCall: return "宏调用名";            // 宏调用（如println!、vec!）
    default: return "未知";                    // 未识别的类型
    }
}

// Lexer类的静态常量：Rust关键字映射表（关键字字符串 -> TokenType::Keyword）
// 包含Rust的保留关键字（如let、fn、if）和实验性关键字（如async、await）
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

// Lexer类的构造函数：初始化源代码字符串和当前解析位置
// src: 待解析的源代码字符串；pos: 解析指针（初始为0，指向源代码第一个字符）
Lexer::Lexer(const string& src) : source(src), pos(0) {}

// 查看当前位置偏移offset后的字符（不移动解析指针pos）
// offset: 偏移量（默认0表示当前字符，1表示下一个字符）；返回值：目标字符（超出长度返回'\0'）
char Lexer::peek(size_t offset) const {
    // 检查偏移后是否超出源代码长度，超出则返回空字符
    if (pos + offset >= source.length()) return '\0';
    // 返回偏移后的字符
    return source[pos + offset];
}

// 获取当前位置的字符，并将解析指针pos后移一位
// 返回值：当前字符（超出长度返回'\0'）
char Lexer::get() {
    // 检查是否已到源代码末尾
    if (pos >= source.length()) return '\0';
    // 返回当前字符，并将pos+1
    return source[pos++];
}

// 跳过所有空白字符（空格、制表符、换行符等），将pos移动到非空白字符位置
void Lexer::skipWhitespace() {
    // 循环检查当前字符是否为空白（isspace返回true），若是则pos后移
    while (isspace(peek())) pos++;
}

// 核心分词方法：将源代码解析为Token列表
// 返回值：包含所有解析出的Token的向量
vector<Token> Lexer::tokenize() {
    // 存储解析出的所有Token
    vector<Token> tokens;
    // 循环解析，直到pos到达源代码末尾
    while (pos < source.length()) {
        // 先跳过空白字符（避免解析空白为Token）
        skipWhitespace();
        // 跳过空白后若已到末尾，退出循环
        if (pos >= source.length()) break;

        // 获取当前待解析的字符
        char c = peek();
        // 分支1：字母或下划线开头 → 解析标识符/关键字/宏调用
        if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        // 分支2：数字开头 → 解析数字（整数/浮点数/进制数）
        else if (isdigit(c)) {
            tokens.push_back(readNumber());
        }
        // 分支3：双引号开头 → 解析字符串字面量
        else if (c == '"') {
            tokens.push_back(readString());
        }
        // 分支4：斜杠开头且下一个字符是//或/* → 解析注释
        else if (c == '/' && (peek(1) == '/' || peek(1) == '*')) {
            tokens.push_back(readComment());
        }
        // 分支5：其他字符 → 解析操作符/分隔符/赋值操作符
        else {
            tokens.push_back(readOperatorOrDelimiter());
        }
    }
    // 返回解析完成的Token列表
    return tokens;
}

// 解析标识符、关键字或宏调用（以字母/下划线开头，后接字母/数字/下划线/!）
// 返回值：对应的Token（区分关键字、宏调用、普通标识符）
Token Lexer::readIdentifierOrKeyword() {
    // 存储标识符/关键字/宏调用的文本内容
    string text;
    // 循环读取字符：字母/数字/下划线/!（!仅允许出现在末尾，作为宏调用标识）
    while (isalnum(peek()) || peek() == '_' || peek() == '!') {
        text += get();
        // 若读取到!，说明是宏调用，终止读取（!必须是最后一个字符）
        if (text.back() == '!') break;
    }
    // 判定1：末尾是! → 宏调用Token
    if (text.back() == '!') {
        return Token(text, TokenType::MacroCall);
    }
    // 判定2：在关键字表中 → 关键字Token
    if (keywords.count(text)) {
        return Token(text, TokenType::Keyword);
    }
    // 判定3：普通标识符 → 标识符Token
    return Token(text, TokenType::Identifier);
}

// 解析数字（十进制/十六进制/八进制/二进制整数、浮点数）
// 返回值：对应的数字类型Token
Token Lexer::readNumber() {
    // 存储数字的文本内容
    string text;
    // 标记是否为浮点数（默认false）
    bool isFloat = false;

    // 分支1：进制数（0x/0o/0b开头）
    if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'o' || peek(1) == 'b')) {
        text += get(); // 读取第一个字符'0'
        char base = get(); // 读取进制标识（x/o/b，大小写均可）
        text += base;
        // 循环读取进制数的有效字符：十六进制（0-9a-fA-F）、下划线（数字分隔符）
        while (isxdigit(peek()) || peek() == '_') {
            text += get();
        }
        // 根据进制标识返回对应Token类型
        if (base == 'x' || base == 'X') {
            return Token(text, TokenType::LiteralHexInteger);
        } else if (base == 'o' || base == 'O') {
            return Token(text, TokenType::LiteralOctInteger);
        } else if (base == 'b' || base == 'B') {
            return Token(text, TokenType::LiteralBinInteger);
        }
        // 兜底：理论上不会走到这里，返回十进制整数
        return Token(text, TokenType::LiteralInteger);
    }

    // 分支2：十进制整数/浮点数
    // 循环读取有效字符：数字、小数点（浮点数标识）、下划线（数字分隔符）
    while (isdigit(peek()) || peek() == '.' || peek() == '_') {
        // 检测小数点：若下一个字符也是.，则是范围操作符（..），终止读取
        if (peek() == '.') {
            if (peek(1) == '.') break; // 避免将..解析为浮点数的一部分
            isFloat = true; // 标记为浮点数
        }
        text += get();
    }
    // 根据是否是浮点数，返回对应Token类型
    return Token(text, isFloat ? TokenType::LiteralFloat : TokenType::LiteralInteger);
}

// 解析字符串字面量（以双引号包裹）
// 返回值：字符串字面量Token
Token Lexer::readString() {
    string text;
    // 读取第一个双引号（"）
    text += get();
    // 循环读取直到闭合双引号或源代码末尾
    while (peek() != '"' && peek() != '\0') {
        // 处理转义字符（如\"、\\）：先读取\，再读取后续字符
        if (peek() == '\\') text += get();
        text += get();
    }
    // 读取闭合双引号（若存在）
    if (peek() == '"') text += get();
    // 返回字符串字面量Token
    return Token(text, TokenType::LiteralString);
}

// 解析注释（单行// 或 多行/* */）
// 返回值：注释Token
Token Lexer::readComment() {
    string text;
    // 分支1：单行注释（//）
    if (peek() == '/' && peek(1) == '/') {
        // 循环读取直到换行符或末尾（单行注释到行尾结束）
        while (peek() != '\n' && peek() != '\0') text += get();
    }
    // 分支2：多行注释（/* */）
    else if (peek() == '/' && peek(1) == '*') {
        // 读取/*
        text += get(); text += get();
        // 循环读取直到*/或末尾
        while (!(peek() == '*' && peek(1) == '/') && peek() != '\0') text += get();
        // 读取*/（若存在）
        if (peek() == '*') { text += get(); text += get(); }
    }
    // 返回注释Token
    return Token(text, TokenType::Comment);
}

// 解析操作符、分隔符、赋值操作符
// 返回值：对应的Token（区分Delimiter/AssignmentOperator/Operator）
Token Lexer::readOperatorOrDelimiter() {
    string text;
    // 读取第一个字符
    char c = get();
    text += c;

    // 查看下一个字符，判断是否是双字符符号（如==、&&、..）
    char next = peek();
    if ((c == '=' && next == '=') || (c == '!' && next == '=') ||  // ==、!=
        (c == '<' && next == '=') || (c == '>' && next == '=') ||  // <=、>=
        (c == '+' && next == '=') || (c == '-' && next == '=') ||  // +=、-=
        (c == '*' && next == '=') || (c == '/' && next == '=') ||  // *=、/=
        (c == '&' && next == '&') || (c == '|' && next == '|') ||  // &&、||
        (c == '.' && next == '.')) {                               // ..（范围操作符）
        // 读取第二个字符，组成双字符符号
        text += get();
    }

    // 判定1：分隔符（()[]{};, 或..）
    static const string delimiters = "()[]{};,";
    if (delimiters.find(c) != string::npos || text == "..") {
        return Token(text, TokenType::Delimiter);
    }

    // 判定2：赋值操作符（=、+=、-=、*=、/=、%=）
    if (text == "=" || text == "+=" || text == "-=" || text == "*=" || text == "/=" || text == "%=") {
        return Token(text, TokenType::AssignmentOperator);
    }

    // 判定3：普通操作符（如+、-、*、/、&、|等）
    return Token(text, TokenType::Operator);
}
