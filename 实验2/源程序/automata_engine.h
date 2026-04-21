#ifndef AUTOMATA_ENGINE_H
#define AUTOMATA_ENGINE_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QStack>
#include <algorithm>

namespace Automata {

const int EPSILON = -1;

// NFA 状态结构
struct NFAState {
    int id;
    bool isAccept;
    int acceptTokenId; // 接受的 Token ID，若非接受状态则为 -1
    QMap<int, QVector<int>> transitions; // 转换表：字符 -> 下一个状态集合 (EPSILON 表示 ε 转换)

    NFAState(int id) : id(id), isAccept(false), acceptTokenId(-1) {}
};

// NFA 图结构
struct NFAGraph {
    int startState;
    int endState; // Thompson 构造法的主要终点状态
    QVector<NFAState*> states;

    NFAGraph() : startState(-1), endState(-1) {}
};

// DFA 状态结构
struct DFAState {
    int id;
    bool isAccept;
    int acceptTokenId; // 接受的 Token ID
    QSet<int> nfaStates; // 对应的 NFA 状态集合
    QMap<int, int> transitions; // 转换表：字符 -> 下一个 DFA 状态 ID
    QString label;

    DFAState(int id) : id(id), isAccept(false), acceptTokenId(-1) {}
};

// DFA 图结构
struct DFAGraph {
    int startState;
    QVector<DFAState*> states;
    QSet<int> alphabet; // 字母表

    DFAGraph() : startState(-1) {}
    ~DFAGraph() {
        for (auto s : states) delete s;
    }
};

class Engine {
public:
    Engine();
    ~Engine();

    // 正则表达式转 NFA
    NFAGraph* regexToNFA(QString regex);
    
    // NFA 转 DFA (子集构造法)
    DFAGraph* nfaToDFA(NFAGraph* nfa);
    
    // DFA 最小化 (Hopcroft 算法或类似划分法)
    DFAGraph* minimizeDFA(DFAGraph* dfa);

    // 合并多个 NFA 用于多 Token 文法
    NFAGraph* combineNFAs(const QList<NFAGraph*>& nfas, const QList<int>& ids);

    // 预处理与中缀转后缀
    QString preprocessRegex(QString regex, const QSet<QString>& aliases);
    QString infixToPostfix(QString infix, const QSet<QString>& aliases);

    // 状态计数器管理
    void resetStateCounter() { stateCounter = 1; }

    // 别名 (Alias) 映射管理
    void setAliasPattern(QString name, QString pattern) { m_aliases[name] = pattern; }
    void clearAliases() { m_aliases.clear(); }
    QString getAliasPattern(QString name) const { return m_aliases.value(name); }
    QMap<QString, QString> getAliases() const { return m_aliases; }

private:
    int stateCounter;
    QMap<QString, QString> m_aliases;
    int newStateId() { return stateCounter++; }

    // Thompson 构造法基础块
    NFAGraph* createBasic(int c);
    NFAGraph* createUnion(NFAGraph* left, NFAGraph* right);
    NFAGraph* createConcat(NFAGraph* first, NFAGraph* second);
    NFAGraph* createStar(NFAGraph* nfa);
    NFAGraph* createPlus(NFAGraph* nfa);
    NFAGraph* createOptional(NFAGraph* nfa);

    // 子集构造法辅助函数
    QSet<int> epsilonClosure(NFAGraph* nfa, QSet<int> states);
    QSet<int> move(NFAGraph* nfa, const QSet<int>& states, int symbol);
    
    void clearNFA(NFAGraph* nfa);
};

}

#endif
