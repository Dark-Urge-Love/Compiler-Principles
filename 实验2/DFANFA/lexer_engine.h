#ifndef LEXER_ENGINE_H
#define LEXER_ENGINE_H

#include <QString>
#include <QMap>
#include "automata_engine.h"

namespace Lexer {

class LexerEngine {
public:
    // 在 GUI 中对 DFA 运行模拟测试
    static int simulateDFA(Automata::DFAGraph* dfa, 
                          const QString& input, 
                          int start, 
                          const QMap<int, QString>& idToAlias, 
                          Automata::Engine& engine);

    // 根据 DFA 生成独立运行的 C++ 源代码
    static QString generateStandaloneCode(Automata::DFAGraph* dfa, 
                                          const QMap<int, QString>& idToAlias, 
                                          Automata::Engine& engine);
};

}

#endif
