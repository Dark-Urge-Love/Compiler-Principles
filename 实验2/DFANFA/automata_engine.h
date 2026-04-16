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

struct NFAState {
    int id;
    bool isAccept;
    QMap<int, QVector<int>> transitions; // char -> next states (EPSILON for epsilon)

    NFAState(int id) : id(id), isAccept(false) {}
};

struct NFAGraph {
    int startState;
    int endState;
    QVector<NFAState*> states;

    NFAGraph() : startState(-1), endState(-1) {}
};

struct DFAState {
    int id;
    bool isAccept;
    QSet<int> nfaStates;
    QMap<int, int> transitions; // char -> next DFA state id
    QString label; // For named regexes if needed

    DFAState(int id) : id(id), isAccept(false) {}
};

struct DFAGraph {
    int startState;
    QVector<DFAState*> states;
    QSet<int> alphabet;

    DFAGraph() : startState(-1) {}
    ~DFAGraph() {
        for (auto s : states) delete s;
    }
};

class Engine {
public:
    Engine();
    ~Engine();

    // Regex to NFA
    NFAGraph* regexToNFA(QString regex);
    
    // NFA to DFA
    DFAGraph* nfaToDFA(NFAGraph* nfa);
    
    // DFA Minimization
    DFAGraph* minimizeDFA(DFAGraph* dfa);

    // Preprocessing helper
    QString preprocessRegex(QString regex);
    QString infixToPostfix(QString infix);

private:
    int stateCounter;
    int newStateId() { return stateCounter++; }

    // Thompson basic blocks
    NFAGraph* createBasic(int c);
    NFAGraph* createUnion(NFAGraph* left, NFAGraph* right);
    NFAGraph* createConcat(NFAGraph* first, NFAGraph* second);
    NFAGraph* createStar(NFAGraph* nfa);
    NFAGraph* createPlus(NFAGraph* nfa);
    NFAGraph* createOptional(NFAGraph* nfa);

    // Subset construction helpers
    QSet<int> epsilonClosure(NFAGraph* nfa, QSet<int> states);
    QSet<int> move(NFAGraph* nfa, const QSet<int>& states, int symbol);
    
    void clearNFA(NFAGraph* nfa);
};

} // namespace Automata

#endif // AUTOMATA_ENGINE_H
