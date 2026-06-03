#include "LRDFA.h"
#include "Grammar.h"
#include <algorithm>

QSet<QString> LRDFABuilder::firstOfString(const QStringList &symbols,
    const QMap<QString, QSet<QString>> &firstSets)
{
    QSet<QString> result;
    bool allEpsilon = true;
    for (const QString &sym : symbols) {
        if (!firstSets.contains(sym)) {
            result.insert(sym);
            allEpsilon = false;
            break;
        }
        const QSet<QString> &first = firstSets[sym];
        bool hasEpsilon = first.contains("");
        for (const QString &f : first) {
            if (f != "") result.insert(f);
        }
        if (!hasEpsilon) { allEpsilon = false; break; }
    }
    if (allEpsilon) result.insert("");
    return result;
}


QString LRDFABuilder::itemToString(const Production &p, int dotPos, const QString &lookahead)
{
    QString rhs;
    if (p.rhs.isEmpty()) {
        rhs = "#";
    } else {
        QStringList parts;
        for (int i = 0; i < p.rhs.size(); i++) {
            if (i == dotPos) parts.append(".");
            parts.append(p.rhs[i]);
        }
        if (dotPos == p.rhs.size()) parts.append(".");
        rhs = parts.join(" ");
    }
    QString result = p.lhs + " -> " + rhs;
    if (!lookahead.isEmpty()) result += ", " + lookahead;
    return result;
}

QString LRDFABuilder::productionToString(const Production &p)
{
    if (p.rhs.isEmpty()) return p.lhs + " -> #";
    return p.lhs + " -> " + p.rhs.join(" ");
}

QString LRDFABuilder::joinIntVector(const QVector<int> &vec)
{
    QStringList strs;
    for (int v : vec) strs.append(QString::number(v));
    return strs.join(" ");
}

bool LRDFABuilder::lr0ItemsEqual(const QVector<LR1Item> &a, const QVector<LR1Item> &b)
{
    if (a.size() != b.size()) return false;
    QSet<QString> setA;
    for (const auto &it : a)
        setA.insert(QString::number(it.prodIndex) + ":" + QString::number(it.dotPos));
    for (const auto &it : b) {
        if (!setA.contains(QString::number(it.prodIndex) + ":" + QString::number(it.dotPos)))
            return false;
    }
    return true;
}

QVector<LR1Item> LRDFABuilder::closureLR0(const QVector<LR1Item> &items,
    const QVector<Production> &augProds,
    const QSet<QString> &nonTerminals)
{
    QVector<LR1Item> result = items;
    QVector<LR1Item> queue = items;
    QSet<QString> visited;
    for (const auto &it : items)
        visited.insert(QString::number(it.prodIndex) + ":" + QString::number(it.dotPos));

    while (!queue.isEmpty()) {
        LR1Item item = queue.takeFirst();
        const Production &prod = augProds[item.prodIndex];

        if (item.dotPos < prod.rhs.size()) {
            const QString &next = prod.rhs[item.dotPos];
            if (nonTerminals.contains(next)) {
                for (int i = 0; i < augProds.size(); i++) {
                    if (augProds[i].lhs == next) {
                        QString key = QString::number(i) + ":0";
                        if (!visited.contains(key)) {
                            visited.insert(key);
                            LR1Item newItem;
                            newItem.prodIndex = i;
                            newItem.dotPos = 0;
                            result.append(newItem);
                            queue.append(newItem);
                        }
                    }
                }
            }
        }
    }
    return result;
}

QVector<LR1Item> LRDFABuilder::gotoLR0(const QVector<LR1Item> &items,
    const QString &symbol,
    const QVector<Production> &augProds,
    const QSet<QString> &nonTerminals)
{
    QVector<LR1Item> nextItems;
    for (const auto &item : items) {
        const Production &prod = augProds[item.prodIndex];
        if (item.dotPos < prod.rhs.size() && prod.rhs[item.dotPos] == symbol) {
            LR1Item newItem;
            newItem.prodIndex = item.prodIndex;
            newItem.dotPos = item.dotPos + 1;
            nextItems.append(newItem);
        }
    }
    if (nextItems.isEmpty()) return {};
    return closureLR0(nextItems, augProds, nonTerminals);
}

QVector<LRState> LRDFABuilder::buildLR0States(
    const QVector<Production> &augProds,
    const QSet<QString> &nonTerminals)
{
    QVector<LRState> states;

    LR1Item startItem;
    startItem.prodIndex = 0;
    startItem.dotPos = 0;
    QVector<LR1Item> startItems = LRDFABuilder::closureLR0({startItem}, augProds, nonTerminals);

    LRState s0;
    s0.id = 0;
    s0.items = startItems;
    states.append(s0);

    QVector<int> queue;
    queue.append(0);

    while (!queue.isEmpty()) {
        int sid = queue.takeFirst();
        const QVector<LR1Item> currentItems = states[sid].items;

        QSet<QString> symbols;
        for (const auto &item : currentItems) {
            const Production &prod = augProds[item.prodIndex];
            if (item.dotPos < prod.rhs.size())
                symbols.insert(prod.rhs[item.dotPos]);
        }

        for (const QString &sym : symbols) {
            QVector<LR1Item> newItems = LRDFABuilder::gotoLR0(currentItems, sym, augProds, nonTerminals);
            if (newItems.isEmpty()) continue;

            int existing = -1;
            for (int i = 0; i < states.size(); i++) {
                if (LRDFABuilder::lr0ItemsEqual(states[i].items, newItems)) {
                    existing = i;
                    break;
                }
            }

            if (existing >= 0) {
                states[sid].transitions[sym] = existing;
            } else {
                LRState newState;
                newState.id = states.size();
                newState.items = newItems;
                states.append(newState);
                queue.append(newState.id);
                states[sid].transitions[sym] = newState.id;
            }
        }
    }
    return states;
}

QStringList LRDFABuilder::checkSLR1(const QVector<LRState> &lr0States,
    const QVector<Production> &augProds,
    const QSet<QString> &terminals,
    const QMap<QString, QSet<QString>> &followSets)
{
    QStringList conflicts;

    for (const auto &state : lr0States) {
        QVector<LR1Item> reduceItems;
        QSet<QString> shiftTerms;

        for (const auto &item : state.items) {
            const Production &prod = augProds[item.prodIndex];
            if (item.dotPos == prod.rhs.size()) {
                if (item.prodIndex > 0) reduceItems.append(item);
            } else {
                if (terminals.contains(prod.rhs[item.dotPos]))
                    shiftTerms.insert(prod.rhs[item.dotPos]);
            }
        }

        // shift-reduce
        if (!reduceItems.isEmpty() && !shiftTerms.isEmpty()) {
            for (const auto &rItem : reduceItems) {
                const auto &follow = followSets[augProds[rItem.prodIndex].lhs];
                for (const QString &sym : shiftTerms) {
                    if (follow.contains(sym))
                        conflicts.append(QString("状态 %1：在 '%2' 上 shift 与 reduce 冲突 (%3)")
                            .arg(state.id).arg(sym).arg(productionToString(augProds[rItem.prodIndex])));
                }
            }
        }

        // reduce-reduce
        if (reduceItems.size() >= 2) {
            for (int i = 0; i < reduceItems.size(); i++) {
                for (int j = i + 1; j < reduceItems.size(); j++) {
                    const auto &f1 = followSets[augProds[reduceItems[i].prodIndex].lhs];
                    const auto &f2 = followSets[augProds[reduceItems[j].prodIndex].lhs];
                    for (const QString &sym : f1) {
                        if (f2.contains(sym))
                            conflicts.append(QString("状态 %1：在 '%2' 上 reduce-reduce 冲突 (%3 vs %4)")
                                .arg(state.id).arg(sym)
                                .arg(productionToString(augProds[reduceItems[i].prodIndex]))
                                .arg(productionToString(augProds[reduceItems[j].prodIndex])));
                    }
                }
            }
        }
    }
    return conflicts;
}
bool LRDFABuilder::lr1ItemsEqual(const QVector<LR1Item> &a, const QVector<LR1Item> &b)
{
    if (a.size() != b.size()) return false;
    QSet<QString> setA;
    for (const auto &it : a)
        setA.insert(QString::number(it.prodIndex) + ":" +
                    QString::number(it.dotPos) + ":" + it.lookahead);
    for (const auto &it : b) {
        if (!setA.contains(QString::number(it.prodIndex) + ":" +
                           QString::number(it.dotPos) + ":" + it.lookahead))
            return false;
    }
    return true;
}

QVector<LR1Item> LRDFABuilder::closureLR1(const QVector<LR1Item> &items,
    const QVector<Production> &augProds,
    const QSet<QString> &nonTerminals,
    const QMap<QString, QSet<QString>> &firstSets)
{
    QVector<LR1Item> result = items;
    QVector<LR1Item> queue = items;
    QSet<QString> visited;
    for (const auto &it : items)
        visited.insert(QString::number(it.prodIndex) + ":" +
                       QString::number(it.dotPos) + ":" + it.lookahead);

    while (!queue.isEmpty()) {
        LR1Item item = queue.takeFirst();
        const Production &prod = augProds[item.prodIndex];

        if (item.dotPos < prod.rhs.size()) {
            const QString &next = prod.rhs[item.dotPos];
            if (nonTerminals.contains(next)) {
                QStringList beta = prod.rhs.mid(item.dotPos + 1);
                QStringList laSymbols = beta;
                laSymbols.append(item.lookahead);
                QSet<QString> lookaheads = firstOfString(laSymbols, firstSets);

                for (int i = 0; i < augProds.size(); i++) {
                    if (augProds[i].lhs == next) {
                        for (const QString &la : lookaheads) {
                            if (la == "") continue;
                            QString key = QString::number(i) + ":0:" + la;
                            if (!visited.contains(key)) {
                                visited.insert(key);
                                LR1Item newItem;
                                newItem.prodIndex = i;
                                newItem.dotPos = 0;
                                newItem.lookahead = la;
                                result.append(newItem);
                                queue.append(newItem);
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

QVector<LR1Item> LRDFABuilder::gotoLR1(const QVector<LR1Item> &items,
    const QString &symbol,
    const QVector<Production> &augProds,
    const QSet<QString> &nonTerminals,
    const QMap<QString, QSet<QString>> &firstSets)
{
    QVector<LR1Item> nextItems;
    for (const auto &item : items) {
        const Production &prod = augProds[item.prodIndex];
        if (item.dotPos < prod.rhs.size() && prod.rhs[item.dotPos] == symbol) {
            LR1Item newItem;
            newItem.prodIndex = item.prodIndex;
            newItem.dotPos = item.dotPos + 1;
            newItem.lookahead = item.lookahead;
            nextItems.append(newItem);
        }
    }
    if (nextItems.isEmpty()) return {};
    return closureLR1(nextItems, augProds, nonTerminals, firstSets);
}

QVector<LRState> LRDFABuilder::buildLR1States(
    const QVector<Production> &augProds,
    const QSet<QString> &nonTerminals,
    const QMap<QString, QSet<QString>> &firstSets)
{
    QVector<LRState> states;

    LR1Item startItem;
    startItem.prodIndex = 0;
    startItem.dotPos = 0;
    startItem.lookahead = "$";
    QVector<LR1Item> startItems = LRDFABuilder::closureLR1({startItem}, augProds, nonTerminals, firstSets);

    LRState s0;
    s0.id = 0;
    s0.items = startItems;
    states.append(s0);

    QVector<int> queue;
    queue.append(0);

    while (!queue.isEmpty()) {
        int sid = queue.takeFirst();
        const QVector<LR1Item> currentItems = states[sid].items;

        QSet<QString> symbols;
        for (const auto &item : currentItems) {
            const Production &prod = augProds[item.prodIndex];
            if (item.dotPos < prod.rhs.size())
                symbols.insert(prod.rhs[item.dotPos]);
        }

        for (const QString &sym : symbols) {
            QVector<LR1Item> newItems = LRDFABuilder::gotoLR1(currentItems, sym, augProds, nonTerminals, firstSets);
            if (newItems.isEmpty()) continue;

            int existing = -1;
            for (int i = 0; i < states.size(); i++) {
                if (LRDFABuilder::lr1ItemsEqual(states[i].items, newItems)) {
                    existing = i;
                    break;
                }
            }

            if (existing >= 0) {
                states[sid].transitions[sym] = existing;
            } else {
                LRState newState;
                newState.id = states.size();
                newState.items = newItems;
                states.append(newState);
                queue.append(newState.id);
                states[sid].transitions[sym] = newState.id;
            }
        }
    }
    return states;
}

QVector<LR1Item> LRDFABuilder::lr0Core(const QVector<LR1Item> &items)
{
    QVector<LR1Item> core;
    QSet<QString> seen;
    for (const auto &it : items) {
        QString key = QString::number(it.prodIndex) + ":" + QString::number(it.dotPos);
        if (!seen.contains(key)) {
            seen.insert(key);
            LR1Item c;
            c.prodIndex = it.prodIndex;
            c.dotPos = it.dotPos;
            core.append(c);
        }
    }
    std::sort(core.begin(), core.end(), [](const LR1Item &a, const LR1Item &b) {
        if (a.prodIndex != b.prodIndex) return a.prodIndex < b.prodIndex;
        return a.dotPos < b.dotPos;
    });
    return core;
}

QVector<LRState> LRDFABuilder::mergeToLALR(const QVector<LRState> &lr1States)
{
    QVector<LRState> lalrStates;

    QMap<QString, QVector<int>> coreGroups;
    for (const auto &state : lr1States) {
        QVector<LR1Item> core = lr0Core(state.items);
        QStringList keyParts;
        for (const auto &it : core)
            keyParts.append(QString::number(it.prodIndex) + ":" + QString::number(it.dotPos));
        coreGroups[keyParts.join("|")].append(state.id);
    }

    QMap<int, int> oldToNew;

    for (auto it = coreGroups.begin(); it != coreGroups.end(); ++it) {
        const QVector<int> &group = it.value();

        QMap<QString, QSet<QString>> combinedLookaheads;
        for (int sid : group) {
            for (const auto &item : lr1States[sid].items) {
                QString coreKey = QString::number(item.prodIndex) + ":" + QString::number(item.dotPos);
                combinedLookaheads[coreKey].insert(item.lookahead);
            }
        }

        QVector<LR1Item> mergedItems;
        for (auto it2 = combinedLookaheads.begin(); it2 != combinedLookaheads.end(); ++it2) {
            QStringList parts = it2.key().split(":");
            int pIdx = parts[0].toInt();
            int dPos = parts[1].toInt();
            for (const QString &la : it2.value()) {
                LR1Item newItem;
                newItem.prodIndex = pIdx;
                newItem.dotPos = dPos;
                newItem.lookahead = la;
                mergedItems.append(newItem);
            }
        }

        int newId = lalrStates.size();
        LRState newState;
        newState.id = newId;
        newState.items = mergedItems;
        newState.originalIds = group;
        lalrStates.append(newState);

        for (int oldId : group) oldToNew[oldId] = newId;
    }

    for (int i = 0; i < lalrStates.size(); i++) {
        // 合并所有原始 LR(1) 状态的转移
        for (int oldId : lalrStates[i].originalIds) {
            if (oldId < 0 || oldId >= lr1States.size()) continue;
            const LRState &oldState = lr1States[oldId];
            for (auto it = oldState.transitions.begin(); it != oldState.transitions.end(); ++it) {
                if (oldToNew.contains(it.value())) {
                    int newTarget = oldToNew[it.value()];
                    // 只在新状态中尚不存在该符号的转移时才添加
                    if (!lalrStates[i].transitions.contains(it.key()))
                        lalrStates[i].transitions[it.key()] = newTarget;
                }
            }
        }
    }

    return lalrStates;
}


LRDFABuilder::BuildResult LRDFABuilder::buildAll(
    const QVector<Production> &productions,
    const QSet<QString> &nonTerminals,
    const QSet<QString> &terminals,
    const QString &startSymbol,
    const QMap<QString, QSet<QString>> &firstSets,
    const QMap<QString, QSet<QString>> &followSets)
{
    BuildResult result;

    // 增广文法
    result.augProductions.append({Grammar::augStart, {startSymbol}, 0});
    for (const auto &p : productions)
        result.augProductions.append({p.lhs, p.rhs, p.index + 1});

    // LR(0)
    result.lr0States = buildLR0States(result.augProductions, nonTerminals);

    // SLR(1)
    result.slr1Conflicts = checkSLR1(result.lr0States, result.augProductions, terminals, followSets);

    // LR(1)
    result.lr1States = buildLR1States(result.augProductions, nonTerminals, firstSets);

    // LALR(1)
    result.lalrStates = mergeToLALR(result.lr1States);

    return result;
}


QMap<int, QMap<QString, ParsingAction>> LRDFABuilder::buildActionTable(
    const QVector<LRState> &lalrStates,
    const QVector<Production> &augProductions,
    const QSet<QString> &terminals)
{
    QMap<int, QMap<QString, ParsingAction>> table;

    for (const auto &state : lalrStates) {
        QMap<QString, ParsingAction> &actions = table[state.id];

        // 1) 移进动作：直接从转移表取，转移符号是终结符即为移进
        for (auto it = state.transitions.begin(); it != state.transitions.end(); ++it) {
            if (terminals.contains(it.key())) {
                ParsingAction a;
                a.type = "shift";
                a.value = it.value();
                actions[it.key()] = a;
            }
        }

        // 2) 归约/接受动作
        for (const auto &item : state.items) {
            const Production &prod = augProductions[item.prodIndex];
            if (item.dotPos != prod.rhs.size()) continue;

            if (item.prodIndex == 0) {
                // S' -> S .
                if (!actions.contains("$")) {
                    ParsingAction a;
                    a.type = "accept";
                    actions["$"] = a;
                }
            } else {
                const QString &la = item.lookahead;
                if (!actions.contains(la)) {
                    ParsingAction a;
                    a.type = "reduce";
                    a.value = item.prodIndex;
                    actions[la] = a;
                } else if (actions[la].type == "shift") {
                    actions[la].type = "conflict";
                }
            }
        }
    }
    return table;
}

QMap<int, QMap<QString, int>> LRDFABuilder::buildGotoTable(
    const QVector<LRState> &lalrStates,
    const QSet<QString> &nonTerminals)
{
    QMap<int, QMap<QString, int>> table;
    for (const auto &state : lalrStates) {
        QMap<QString, int> gotos;
        for (auto it = state.transitions.begin(); it != state.transitions.end(); ++it) {
            if (nonTerminals.contains(it.key()))
                gotos[it.key()] = it.value();
        }
        table[state.id] = gotos;
    }
    return table;
}


LRDFABuilder::ParseResult LRDFABuilder::parse(
    const QStringList &tokens,
    QMap<int, QMap<QString, ParsingAction>> &actionTable,
    QMap<int, QMap<QString, int>> &gotoTable,
    const QVector<Production> &augProductions)
{
    ParseResult result;

    QVector<int> stateStack;
    stateStack.append(0);
    int pos = 0;
    int stepNum = 0;

    result.steps.append({
        QString::number(stepNum++), "0",
        tokens.join(" "), "初始化"
    });

    while (true) {
        int state = stateStack.last();
        QString lookahead = (pos < tokens.size()) ? tokens[pos] : "$";

        if (!actionTable.contains(state) || !actionTable[state].contains(lookahead)) {
            QString diag;
            if (actionTable.contains(state)) {
                QStringList entries;
                for (auto it = actionTable[state].begin(); it != actionTable[state].end(); ++it)
                    entries.append(QString("%1->%2(%3)").arg(it.key()).arg(it.value().type).arg(it.value().value));
                diag = " [诊断] 状态" + QString::number(state) + "的ACTION: " + entries.join("; ");
            } else {
                diag = " [诊断] 状态" + QString::number(state) + "不在分析表中";
                QStringList avail;
                for (auto it = actionTable.begin(); it != actionTable.end(); ++it)
                    avail.append(QString::number(it.key()));
                diag += " (可用状态: " + avail.join(",") + ")";
            }
            result.steps.append({
                QString::number(stepNum++),
                joinIntVector(stateStack),
                tokens.mid(pos).join(" "),
                QString("错误：状态 %1 遇到 '%2' 无有效动作%3").arg(state).arg(lookahead).arg(diag)
            });
            break;
        }

        const ParsingAction &act = actionTable[state][lookahead];

        if (act.type == "shift") {
            stateStack.append(act.value);
            pos++;
            result.steps.append({
                QString::number(stepNum++),
                joinIntVector(stateStack),
                tokens.mid(pos).join(" "),
                QString("移进到状态 %1").arg(act.value)
            });
        } else if (act.type == "reduce") {
            const Production &prod = augProductions[act.value];
            int popCount = prod.rhs.size();
            for (int i = 0; i < popCount && !stateStack.isEmpty(); i++)
                stateStack.removeLast();

            int topState = stateStack.isEmpty() ? 0 : stateStack.last();
            if (!gotoTable.contains(topState) || !gotoTable[topState].contains(prod.lhs)) {
                result.steps.append({
                    QString::number(stepNum++),
                    joinIntVector(stateStack),
                    tokens.mid(pos).join(" "),
                    QString("错误：状态 %1 在 '%2' 上无 goto 转移").arg(topState).arg(prod.lhs)
                });
                break;
            }
            stateStack.append(gotoTable[topState][prod.lhs]);

            result.steps.append({
                QString::number(stepNum++),
                joinIntVector(stateStack),
                tokens.mid(pos).join(" "),
                QString("归约: %1").arg(productionToString(prod))
            });
        } else if (act.type == "accept") {
            result.steps.append({
                QString::number(stepNum++),
                joinIntVector(stateStack), "", "接受"
            });
            result.accepted = true;
            break;
        } else {
            result.steps.append({
                QString::number(stepNum++),
                joinIntVector(stateStack),
                tokens.mid(pos).join(" "), "遇到冲突"
            });
            break;
        }
    }

    return result;
}
