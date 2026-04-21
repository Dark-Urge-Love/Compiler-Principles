#include "automata_engine.h"
#include <QDebug>
#include <QRegularExpression>

namespace Automata {

Engine::Engine() : stateCounter(1) {}

Engine::~Engine() {}

// 预处理正则表达式：处理转义、字符类、别名，并插入显式的连接符号 '.'
QString Engine::preprocessRegex(QString regex, const QSet<QString>& aliases) {
    QStringList tokens;
    
    for (int i = 0; i < regex.length(); ++i) {
        if (regex[i].isSpace()) continue;
        
        // 处理转义字符
        if (regex[i] == '\\' && i + 1 < regex.length()) {
            tokens << regex.mid(i, 2);
            i++;
            continue;
        }

        // 处理字符类 [A-Za-z] 等
        if (regex[i] == '[') {
            int end = regex.indexOf(']', i);
            if (end != -1) {
                tokens << regex.mid(i, end - i + 1);
                i = end;
                continue;
            }
        }

        // 处理别名（如 letter, digit）
        if (regex[i].isLetter() || regex[i] == '_') {
            QString id;
            while (i < regex.length() && (regex[i].isLetterOrNumber() || regex[i] == '_')) {
                id += regex[i++];
            }
            i--;
            tokens << id;
            continue;
        }

        tokens << QString(regex[i]);
    }

    // 自动插入隐式的连接符 '.' (例如 "ab" 变为 "a.b")
    QString processed;
    for (int i = 0; i < tokens.size(); ++i) {
        QString t1 = tokens[i];
        processed += t1;
        
        if (i + 1 < tokens.size()) {
            QString t2 = tokens[i+1];
            
            bool t1CanConcat = (t1 != "(" && t1 != "|" && t1 != "\\");
            bool t2NeedsConcat = (t2 != ")" && t2 != "|" && t2 != "*" && t2 != "+" && t2 != "?" && t2 != ".");
            
            if (t1 == "*" || t1 == "+" || t1 == "?" || t1 == ")") t1CanConcat = true;

            if (t1CanConcat && t2NeedsConcat) {
                processed += ".";
            }
        }
    }
    return processed;
}

int getPrecedence(QChar c) {
    if (c == '*' || c == '+' || c == '?') return 3;
    if (c == '.') return 2;
    if (c == '|') return 1;
    return 0;
}

// 中缀转后缀
QString Engine::infixToPostfix(QString infix, const QSet<QString>& aliases) {
    QString postfix;
    QStack<QString> stack;
    
    for (int i = 0; i < infix.length(); ++i) {
        QString t;
        QChar c = infix[i];
        
        if (c == '\\' && i + 1 < infix.length()) {
            t = infix.mid(i, 2);
            i++;
            postfix += t + " ";
            continue;
        } 
        
        if (c == '(') {
            stack.push("(");
            continue;
        }
        
        if (c == ')') {
            while (!stack.isEmpty() && stack.top() != "(") {
                postfix += stack.pop() + " ";
            }
            if (!stack.isEmpty()) stack.pop();
            continue;
        }
        
        if (c == '|' || c == '*' || c == '+' || c == '?' || c == '.') {
            while (!stack.isEmpty() && getPrecedence(stack.top()[0]) >= getPrecedence(c)) {
                postfix += stack.pop() + " ";
            }
            stack.push(QString(c));
            continue;
        }

        // 处理别名、字符类或普通字符
        if (c.isLetter() || c == '_') {
            while (i < infix.length() && (infix[i].isLetterOrNumber() || infix[i] == '_')) {
                t += infix[i++];
            }
            i--;
        } else if (c == '[') {
            int end = infix.indexOf(']', i);
            if (end != -1) {
                t = infix.mid(i, end - i + 1);
                i = end;
            } else t = c;
        } else {
            t = c;
        }
        postfix += t + " ";
    }
    
    while (!stack.isEmpty()) {
        postfix += stack.pop() + " ";
    }
    return postfix.trimmed();
}

// Thompson 构造法：创建一个接受单个字符的 NFA
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

// Thompson 构造法：并运算 (a|b)
NFAGraph* Engine::createUnion(NFAGraph* left, NFAGraph* right) {
    NFAGraph* nfa = new NFAGraph();
    NFAState* sStart = new NFAState(newStateId());
    NFAState* sEnd = new NFAState(newStateId());
    
    sStart->transitions[EPSILON].push_back(left->startState);
    sStart->transitions[EPSILON].push_back(right->startState);
    
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

// Thompson 构造法：连接运算 (ab)
NFAGraph* Engine::createConcat(NFAGraph* first, NFAGraph* second) {
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

// Thompson 构造法：闭包运算 (a*)
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

// Thompson 构造法：正闭包 (a+)
NFAGraph* Engine::createPlus(NFAGraph* inner) {
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

// Thompson 构造法：可选运算 (a?)
NFAGraph* Engine::createOptional(NFAGraph* inner) {
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

// 将正则表达式转换为 NFA
NFAGraph* Engine::regexToNFA(QString regex) {
    QSet<QString> aliasSet;
    for (auto it = m_aliases.begin(); it != m_aliases.end(); ++it) aliasSet.insert(it.key());

    // 展开别名（宏替换）
    QStringList sortedKeys = m_aliases.keys();
    std::sort(sortedKeys.begin(), sortedKeys.end(), [](const QString& a, const QString& b){
        return a.length() > b.length();
    });

    bool changed = true;
    int limit = 10;
    while (changed && limit-- > 0) {
        changed = false;
        for (const QString& key : sortedKeys) {
            if (m_aliases[key].startsWith("[")) continue;
            
            QRegularExpression re("\\b" + QRegularExpression::escape(key) + "\\b");
            if (regex.contains(re)) {
                regex.replace(re, "(" + m_aliases[key] + ")");
                changed = true;
            }
        }
    }

    QString processed = preprocessRegex(regex, aliasSet);
    QString postfix = infixToPostfix(processed, aliasSet);
    
    QStringList tokens = postfix.split(' ', Qt::SkipEmptyParts);
    QStack<NFAGraph*> stack;
    
    auto getSymbolId = [&](const QString& t) -> int {
        if (t.length() == 1 && !aliasSet.contains(t)) return t[0].unicode();
        return 0x1000 + (qHash(t) % 0x1000); // 对别名或字符类使用特殊映射值
    };

    // 根据后缀表达式通过栈构造 NFA
    for (const QString& token : tokens) {
        if (token == "|") {
            if (stack.size() >= 2) {
                NFAGraph* r = stack.pop();
                NFAGraph* l = stack.pop();
                stack.push(createUnion(l, r));
            }
        } else if (token == ".") {
            if (stack.size() >= 2) {
                NFAGraph* r = stack.pop();
                NFAGraph* l = stack.pop();
                stack.push(createConcat(l, r));
            }
        } else if (token == "*") {
            if (!stack.isEmpty()) stack.push(createStar(stack.pop()));
        } else if (token == "+") {
            if (!stack.isEmpty()) stack.push(createPlus(stack.pop()));
        } else if (token == "?") {
            if (!stack.isEmpty()) stack.push(createOptional(stack.pop()));
        } else {
            QString cleanToken = token;
            if (token.startsWith("\\") && token.length() == 2) cleanToken = token[1];
            
            int id = getSymbolId(cleanToken);
            stack.push(createBasic(id));
        }
    }
    NFAGraph* nfa = stack.isEmpty() ? nullptr : stack.pop();
    if (nfa) {
        // 标记终点状态为接受状态
        for (auto s : nfa->states) {
            if (s->id == nfa->endState) {
                s->isAccept = true;
                s->acceptTokenId = 0;
            }
        }

        // 重新编号状态为从 1 开始的连续序号
        QMap<int, int> idMap;
        for (int i = 0; i < nfa->states.size(); ++i) {
            idMap[nfa->states[i]->id] = i + 1;
        }
        for (auto s : nfa->states) {
            s->id = idMap[s->id];
            QMap<int, QVector<int>> newTransitions;
            for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
                QVector<int> newTargets;
                for (int t : it.value()) {
                    newTargets.push_back(idMap.value(t, t));
                }
                newTransitions[it.key()] = newTargets;
            }
            s->transitions = newTransitions;
        }
        nfa->startState = idMap[nfa->startState];
        nfa->endState = idMap[nfa->endState];
    }
    return nfa;
}

// 合并多个 NFA：创建一个新的起始状态并用 ε 转换连接到各个 NFA
NFAGraph* Engine::combineNFAs(const QList<NFAGraph*>& nfas, const QList<int>& ids) {
    if (nfas.isEmpty()) return nullptr;

    NFAGraph* combined = new NFAGraph();
    NFAState* combinedStart = new NFAState(stateCounter++);
    combined->states.push_back(combinedStart);
    combined->startState = combinedStart->id;

    for (int i = 0; i < nfas.size(); ++i) {
        NFAGraph* nfa = nfas[i];
        int tokenId = ids[i];
        if (!nfa) continue;

        combined->states.append(nfa->states);
        combinedStart->transitions[EPSILON].push_back(nfa->startState);
        
        for (auto s : nfa->states) {
            if (s->id == nfa->endState) {
                s->isAccept = true;
                s->acceptTokenId = tokenId;
            }
        }
    }

    return combined;
}

// 计算 ε-闭包
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

// 子集构造法：计算从给定状态集合出发，通过某个符号能到达的所有 NFA 状态
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

// NFA 转 DFA (子集构造法)
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
    
    DFAState* dStart = new DFAState(1);
    dStart->nfaStates = startClosure;
    for (int nsId : startClosure) {
        for (auto ns : nfa->states) {
            if (ns->id == nsId && ns->isAccept) {
                dStart->isAccept = true;
                if (dStart->acceptTokenId == -1 || ns->acceptTokenId < dStart->acceptTokenId) {
                    dStart->acceptTokenId = ns->acceptTokenId;
                }
            }
        }
    }
    dfa->states.push_back(dStart);
    dfa->startState = 1;

    int i = 0;
    while (i < dstates.size()) {
        QSet<int> T = dstates[i];
        for (int symbol : alphabet) {
            QSet<int> U = epsilonClosure(nfa, move(nfa, T, symbol));
            if (U.isEmpty()) continue;
            
            int targetIdx = -1;
            for (int k = 0; k < dstates.size(); ++k) {
                if (dstates[k] == U) { targetIdx = k + 1; break; }
            }
            
            if (targetIdx == -1) {
                targetIdx = dstates.size() + 1;
                dstates.push_back(U);
                DFAState* dNew = new DFAState(targetIdx);
                dNew->nfaStates = U;
                for (int nsId : U) {
                    for (auto ns : nfa->states) {
                        if (ns->id == nsId && ns->isAccept) {
                            dNew->isAccept = true;
                            if (dNew->acceptTokenId == -1 || ns->acceptTokenId < dNew->acceptTokenId) {
                                dNew->acceptTokenId = ns->acceptTokenId;
                            }
                        }
                    }
                }
                dfa->states.push_back(dNew);
            }
            dfa->states[i]->transitions[symbol] = targetIdx;
        }
        i++;
    }
    return dfa;
}

// 最小化 DFA (分组划分法)
DFAGraph* Engine::minimizeDFA(DFAGraph* dfa) {
    if (!dfa || dfa->states.isEmpty()) return dfa;

    // 初始划分：根据是否为接受状态以及接受的 Token ID 进行分组
    QMap<int, QSet<int>> initialGroups;
    for (auto s : dfa->states) {
        int key = s->isAccept ? s->acceptTokenId : -1;
        initialGroups[key].insert(s->id);
    }
    QVector<QSet<int>> partitions;
    for (auto group : initialGroups.values()) partitions.push_back(group);

    // 迭代划分直到状态不再变化
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
                
                // 根据在当前划分下的转换目标生成特征指纹
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

    // 根据划分结果重新构建 DFA
    DFAGraph* minDfa = new DFAGraph();
    minDfa->alphabet = dfa->alphabet;
    QMap<int, int> stateToPart;
    for (int i = 0; i < partitions.size(); ++i) {
        for (int sId : partitions[i]) stateToPart[sId] = i + 1;
    }

    for (int i = 0; i < partitions.size(); ++i) {
        DFAState* dNew = new DFAState(i + 1);
        int repId = *partitions[i].begin();
        DFAState* repState = nullptr;
        for (auto s : dfa->states) if (s->id == repId) { repState = s; break; }
        
        dNew->isAccept = repState->isAccept;
        dNew->acceptTokenId = repState->acceptTokenId;
        for (int symbol : dfa->alphabet) {
            if (repState->transitions.contains(symbol)) {
                dNew->transitions[symbol] = stateToPart[repState->transitions[symbol]];
            }
        }
        minDfa->states.push_back(dNew);
        if (partitions[i].contains(dfa->startState)) minDfa->startState = i + 1;
    }

    return minDfa;
}

}
