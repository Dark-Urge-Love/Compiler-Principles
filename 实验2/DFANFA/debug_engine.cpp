#include <iostream>
#include <QString>
#include <QDebug>
#include "automata_engine.h"

int main() {
    Automata::Engine engine;
    QString regex = "s";
    Automata::NFAGraph* nfa = engine.regexToNFA(regex);
    if (!nfa) {
        std::cout << "NFA Null" << std::endl;
        return 1;
    }
    std::cout << "NFA States: " << nfa->states.size() << std::endl;
    for (auto s : nfa->states) {
        std::cout << "State " << s->id << " (Accept: " << s->isAccept << ")" << std::endl;
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            std::cout << "  " << it.key() << " -> ";
            for (int v : it.value()) std::cout << v << " ";
            std::cout << std::endl;
        }
    }

    Automata::DFAGraph* dfa = engine.nfaToDFA(nfa);
    std::cout << "DFA States: " << dfa->states.size() << std::endl;
    for (auto s : dfa->states) {
        std::cout << "State " << s->id << " (Accept: " << s->isAccept << ")" << std::endl;
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            std::cout << "  " << (char)it.key() << " -> " << it.value() << std::endl;
        }
    }

    Automata::DFAGraph* minDfa = engine.minimizeDFA(dfa);
    std::cout << "Min DFA States: " << minDfa->states.size() << std::endl;
    for (auto s : minDfa->states) {
        std::cout << "State " << s->id << " (Accept: " << s->isAccept << ")" << std::endl;
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            std::cout << "  " << (char)it.key() << " -> " << it.value() << std::endl;
        }
    }
    return 0;
}
