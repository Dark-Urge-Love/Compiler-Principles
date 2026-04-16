#include "automata_engine.h"
#include <QDebug>

namespace Automata {

Engine::Engine() : stateCounter(0) {}

Engine::~Engine() {}

QString Engine::preprocessRegex(QString regex) {
    QString expanded;
    // Handle character classes [0-9] and [a-zA-Z]
    // Step 1: Handle character classes and ESCAPED spaces
    for (int i = 0; i < regex.length(); ++i) {
        if (regex[i].isSpace()) continue; // Ignore unescaped spaces
        
        if (regex[i] == '\\' && i + 1 < regex.length()) {
            expanded += regex[i];
            expanded += regex[++i];
            continue;
        }
        if (regex[i] == '[' && i + 1 < regex.length()) {
            int end = regex.indexOf(']', i);
            if (end != -1) {
                QString content = regex.mid(i + 1, end - i - 1);
                QString sub;
                if (content == "0-9") {
                    sub = "(0|1|2|3|4|5|6|7|8|9)";
                } else if (content == "a-z") {
                    sub = "(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)";
                } else if (content == "A-Z") {
                    sub = "(A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z)";
                } else if (content == "A-Za-z") {
                    sub = "(A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)";
                } else {
                    // Generic char class [abc] -> (a|b|c)
                    sub = "(";
                    for (int j = 0; j < content.length(); ++j) {
                        sub += content[j];
                        if (j < content.length() - 1) sub += "|";
                    }
                    sub += ")";
                }
                expanded += sub;
                i = end;
                continue;
            }
        }
        expanded += regex[i];
    }

    // Insert implicit concatenation operator '.'
    QString withCat;
    for (int i = 0; i < expanded.length(); ++i) {
        QChar c1 = expanded[i];
        withCat += c1;
        if (i + 1 < expanded.length()) {
            QChar c2 = expanded[i+1];
            
            // c1: characters, operators that end a sub-expression
            bool c1CanConcat = (c1 != '(' && c1 != '|' && c1 != '\\');
            // c2: characters, operators that start a sub-expression
            bool c2NeedsConcat = (c2 != ')' && c2 != '|' && c2 != '*' && c2 != '+' && c2 != '?');
            
            if (c1 == '*' || c1 == '+' || c1 == '?' || c1 == ')') c1CanConcat = true;

            if (c1CanConcat && c2NeedsConcat) {
                withCat += '.';
            }
        }
    }
    return withCat;
}

int getPrecedence(QChar c) {
    if (c == '*' || c == '+' || c == '?') return 3;
    if (c == '.') return 2;
    if (c == '|') return 1;
    return 0;
}

QString Engine::infixToPostfix(QString infix) {
    QString postfix;
    QStack<QChar> stack;
    
    for (int i = 0; i < infix.length(); ++i) {
        QChar c = infix[i];
        if (c == '\\' && i + 1 < infix.length()) {
            postfix += c;
            postfix += infix[++i];
            postfix += ' '; // Use space as literal delimiter
        } else if (c == '(') {
            stack.push(c);
        } else if (c == ')') {
            while (!stack.isEmpty() && stack.top() != '(') {
                postfix += stack.pop();
            }
            if (!stack.isEmpty()) stack.pop();
        } else if (c == '|' || c == '*' || c == '+' || c == '?' || c == '.') {
            while (!stack.isEmpty() && getPrecedence(stack.top()) >= getPrecedence(c)) {
                postfix += stack.pop();
            }
            stack.push(c);
        } else {
            postfix += c;
            postfix += ' ';
        }
    }
    while (!stack.isEmpty()) {
        postfix += stack.pop();
    }
    return postfix;
}

NFAGraph* Engine::createBasic(int c) {
    NFAGraph* nfa = new NFAGraph();
    NFAState* s1 = new NFAState(newStateId());
    NFAState* s2 = new NFAState(newStateId());
    s1->transitions[c].push_back(s2->id);
    nfa->states.push_back(s1);
    nfa->states.push_back(s2);
    nfa->startState = s1->id;
    nfa->endState = s2->id;
    s2->isAccept = true;
    return nfa;
}

NFAGraph* Engine::createUnion(NFAGraph* left, NFAGraph* right) {
    NFAGraph* nfa = new NFAGraph();
    NFAState* sStart = new NFAState(newStateId());
    NFAState* sEnd = new NFAState(newStateId());
    
    sStart->transitions[EPSILON].push_back(left->startState);
    sStart->transitions[EPSILON].push_back(right->startState);
    
    // Find accepting states of subgraphs and link to new end
    for (auto s : left->states) if (s->id == left->endState) { s->isAccept = false; s->transitions[EPSILON].push_back(sEnd->id); }
    for (auto s : right->states) if (s->id == right->endState) { s->isAccept = false; s->transitions[EPSILON].push_back(sEnd->id); }
    
    nfa->states.push_back(sStart);
    nfa->states.append(left->states);
    nfa->states.append(right->states);
    nfa->states.push_back(sEnd);
    
    nfa->startState = sStart->id;
    nfa->endState = sEnd->id;
    sEnd->isAccept = true;
    
    delete left; delete right;
    return nfa;
}

NFAGraph* Engine::createConcat(NFAGraph* first, NFAGraph* second) {
    // Link end of first to start of second
    for (auto s : first->states) {
        if (s->id == first->endState) {
            s->isAccept = false;
            s->transitions[EPSILON].push_back(second->startState);
        }
    }
    first->states.append(second->states);
    first->endState = second->endState;
    delete second;
    return first;
}

NFAGraph* Engine::createStar(NFAGraph* inner) {
    NFAGraph* nfa = new NFAGraph();
    NFAState* sStart = new NFAState(newStateId());
    NFAState* sEnd = new NFAState(newStateId());
    
    sStart->transitions[EPSILON].push_back(inner->startState);
    sStart->transitions[EPSILON].push_back(sEnd->id);
    
    for (auto s : inner->states) {
        if (s->id == inner->endState) {
            s->isAccept = false;
            s->transitions[EPSILON].push_back(inner->startState);
            s->transitions[EPSILON].push_back(sEnd->id);
        }
    }
    
    nfa->states.push_back(sStart);
    nfa->states.append(inner->states);
    nfa->states.push_back(sEnd);
    nfa->startState = sStart->id;
    nfa->endState = sEnd->id;
    sEnd->isAccept = true;
    
    delete inner;
    return nfa;
}

NFAGraph* Engine::createPlus(NFAGraph* inner) {
    // a+ = a.a*
    // Or more efficiently linking end back to start
    NFAGraph* nfa = new NFAGraph();
    NFAState* sStart = new NFAState(newStateId());
    NFAState* sEnd = new NFAState(newStateId());
    
    sStart->transitions[EPSILON].push_back(inner->startState);
    
    for (auto s : inner->states) {
        if (s->id == inner->endState) {
            s->isAccept = false;
            s->transitions[EPSILON].push_back(inner->startState);
            s->transitions[EPSILON].push_back(sEnd->id);
        }
    }
    
    nfa->states.push_back(sStart);
    nfa->states.append(inner->states);
    nfa->states.push_back(sEnd);
    nfa->startState = sStart->id;
    nfa->endState = sEnd->id;
    sEnd->isAccept = true;
    
    delete inner;
    return nfa;
}

NFAGraph* Engine::createOptional(NFAGraph* inner) {
    // a? = a | epsilon
    NFAGraph* nfa = new NFAGraph();
    NFAState* sStart = new NFAState(newStateId());
    NFAState* sEnd = new NFAState(newStateId());
    
    sStart->transitions[EPSILON].push_back(inner->startState);
    sStart->transitions[EPSILON].push_back(sEnd->id);
    
    for (auto s : inner->states) {
        if (s->id == inner->endState) {
            s->isAccept = false;
            s->transitions[EPSILON].push_back(sEnd->id);
        }
    }
    
    nfa->states.push_back(sStart);
    nfa->states.append(inner->states);
    nfa->states.push_back(sEnd);
    nfa->startState = sStart->id;
    nfa->endState = sEnd->id;
    sEnd->isAccept = true;
    
    delete inner;
    return nfa;
}

NFAGraph* Engine::regexToNFA(QString regex) {
    stateCounter = 0;
    QString processed = preprocessRegex(regex);
    QString postfix = infixToPostfix(processed);
    
    QStack<NFAGraph*> stack;
    for (int i = 0; i < postfix.length(); ++i) {
        QChar c = postfix[i];
        if (c == ' ') continue;
        
        if (c == '\\' && i + 1 < postfix.length()) {
            stack.push(createBasic(postfix[++i].toLatin1()));
            if (i + 1 < postfix.length() && postfix[i+1] == ' ') i++;
        } else if (c == '|') {
            if (stack.size() >= 2) {
                NFAGraph* r = stack.pop();
                NFAGraph* l = stack.pop();
                stack.push(createUnion(l, r));
            }
        } else if (c == '.') {
            if (stack.size() >= 2) {
                NFAGraph* r = stack.pop();
                NFAGraph* l = stack.pop();
                stack.push(createConcat(l, r));
            }
        } else if (c == '*') {
            if (!stack.isEmpty()) stack.push(createStar(stack.pop()));
        } else if (c == '+') {
            if (!stack.isEmpty()) stack.push(createPlus(stack.pop()));
        } else if (c == '?') {
            if (!stack.isEmpty()) stack.push(createOptional(stack.pop()));
        } else {
            stack.push(createBasic(c.toLatin1()));
            if (i + 1 < postfix.length() && postfix[i+1] == ' ') i++;
        }
    }
    return stack.isEmpty() ? nullptr : stack.pop();
}

QSet<int> Engine::epsilonClosure(NFAGraph* nfa, QSet<int> states) {
    QSet<int> closure = states;
    QStack<int> stack;
    for (int s : states) stack.push(s);
    
    QMap<int, NFAState*> stateMap;
    for (auto s : nfa->states) stateMap[s->id] = s;
    
    while (!stack.isEmpty()) {
        int u = stack.pop();
        if (stateMap.contains(u)) {
            for (int v : stateMap[u]->transitions[EPSILON]) {
                if (!closure.contains(v)) {
                    closure.insert(v);
                    stack.push(v);
                }
            }
        }
    }
    return closure;
}

QSet<int> Engine::move(NFAGraph* nfa, const QSet<int>& states, int symbol) {
    QSet<int> result;
    QMap<int, NFAState*> stateMap;
    for (auto s : nfa->states) stateMap[s->id] = s;
    
    for (int s : states) {
        if (stateMap.contains(s) && stateMap[s]->transitions.contains(symbol)) {
            for (int target : stateMap[s]->transitions[symbol]) {
                result.insert(target);
            }
        }
    }
    return result;
}

DFAGraph* Engine::nfaToDFA(NFAGraph* nfa) {
    if (!nfa) return nullptr;
    DFAGraph* dfa = new DFAGraph();
    QSet<int> alphabet;
    for (auto s : nfa->states) {
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            if (it.key() != EPSILON) alphabet.insert(it.key());
        }
    }
    dfa->alphabet = alphabet;

    QVector<QSet<int>> dstates;
    QSet<int> startClosure = epsilonClosure(nfa, {nfa->startState});
    dstates.push_back(startClosure);
    
    DFAState* dStart = new DFAState(0);
    dStart->nfaStates = startClosure;
    for (int s : startClosure) if (s == nfa->endState) dStart->isAccept = true;
    dfa->states.push_back(dStart);
    dfa->startState = 0;

    int i = 0;
    while (i < dstates.size()) {
        QSet<int> T = dstates[i];
        for (int symbol : alphabet) {
            QSet<int> U = epsilonClosure(nfa, move(nfa, T, symbol));
            if (U.isEmpty()) continue;
            
            int targetIdx = -1;
            for (int k = 0; k < dstates.size(); ++k) {
                if (dstates[k] == U) { targetIdx = k; break; }
            }
            
            if (targetIdx == -1) {
                targetIdx = dstates.size();
                dstates.push_back(U);
                DFAState* dNew = new DFAState(targetIdx);
                dNew->nfaStates = U;
                for (int s : U) if (s == nfa->endState) dNew->isAccept = true;
                dfa->states.push_back(dNew);
            }
            dfa->states[i]->transitions[symbol] = targetIdx;
        }
        i++;
    }
    return dfa;
}

DFAGraph* Engine::minimizeDFA(DFAGraph* dfa) {
    if (!dfa || dfa->states.isEmpty()) return dfa;

    // Partition refinement (Hopcroft's logic simplified)
    QVector<QSet<int>> partitions;
    QSet<int> acceptStates, nonAcceptStates;
    for (auto s : dfa->states) {
        if (s->isAccept) acceptStates.insert(s->id);
        else nonAcceptStates.insert(s->id);
    }
    if (!acceptStates.isEmpty()) partitions.push_back(acceptStates);
    if (!nonAcceptStates.isEmpty()) partitions.push_back(nonAcceptStates);

    bool changed = true;
    while (changed) {
        changed = false;
        QVector<QSet<int>> newPartitions;
        for (const auto& group : partitions) {
            if (group.size() <= 1) {
                newPartitions.push_back(group);
                continue;
            }
            
            QMap<int, QSet<int>> splitMap;
            for (int sId : group) {
                DFAState* s = nullptr;
                for (auto state : dfa->states) if (state->id == sId) { s = state; break; }
                
                // Construct a signature based on transitions to current partitions
                QString signature;
                for (int symbol : dfa->alphabet) {
                    int target = s->transitions.value(symbol, -1);
                    int partIdx = -1;
                    if (target != -1) {
                        for (int k = 0; k < partitions.size(); ++k) {
                            if (partitions[k].contains(target)) { partIdx = k; break; }
                        }
                    }
                    signature += QString::number(partIdx) + ",";
                }
                // Use hash of signature as key
                int sigHash = qHash(signature);
                splitMap[sigHash].insert(sId);
            }
            
            if (splitMap.size() > 1) changed = true;
            for (auto it = splitMap.begin(); it != splitMap.end(); ++it) {
                newPartitions.push_back(it.value());
            }
        }
        partitions = newPartitions;
    }

    // Build new DFA from partitions
    DFAGraph* minDfa = new DFAGraph();
    minDfa->alphabet = dfa->alphabet;
    QMap<int, int> stateToPart;
    for (int i = 0; i < partitions.size(); ++i) {
        for (int sId : partitions[i]) stateToPart[sId] = i;
    }

    for (int i = 0; i < partitions.size(); ++i) {
        DFAState* dNew = new DFAState(i);
        int repId = *partitions[i].begin();
        DFAState* repState = nullptr;
        for (auto s : dfa->states) if (s->id == repId) { repState = s; break; }
        
        dNew->isAccept = repState->isAccept;
        for (int symbol : dfa->alphabet) {
            if (repState->transitions.contains(symbol)) {
                dNew->transitions[symbol] = stateToPart[repState->transitions[symbol]];
            }
        }
        minDfa->states.push_back(dNew);
        if (partitions[i].contains(dfa->startState)) minDfa->startState = i;
    }

    return minDfa;
}

} // namespace Automata
