#ifndef LRDFA_H
#define LRDFA_H

#include "Production.h"
#include <QVector>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

class LRDFABuilder {
public:
    // 一次构建 LR(0) / LR(1) / LALR(1) DFA
    struct BuildResult {
        QVector<LRState> lr0States;
        QVector<LRState> lr1States;
        QVector<LRState> lalrStates;
        QVector<Production> augProductions;
        QStringList slr1Conflicts;
    };

    static BuildResult buildAll(
        const QVector<Production> &productions,
        const QSet<QString> &nonTerminals,
        const QSet<QString> &terminals,
        const QString &startSymbol,
        const QMap<QString, QSet<QString>> &firstSets,
        const QMap<QString, QSet<QString>> &followSets);

    // 构建 ACTION 表
    static QMap<int, QMap<QString, ParsingAction>> buildActionTable(
        const QVector<LRState> &lalrStates,
        const QVector<Production> &augProductions,
        const QSet<QString> &terminals);

    // 构建 GOTO 表
    static QMap<int, QMap<QString, int>> buildGotoTable(
        const QVector<LRState> &lalrStates,
        const QSet<QString> &nonTerminals);

    // LALR(1) 分析结果
    struct ParseResult {
        QVector<QStringList> steps;
        bool accepted = false;
    };

    // 执行 LALR(1) 分析
    static ParseResult parse(
        const QStringList &tokens,
        QMap<int, QMap<QString, ParsingAction>> &actionTable,
        QMap<int, QMap<QString, int>> &gotoTable,
        const QVector<Production> &augProductions);

    // 格式化
    static QString itemToString(const Production &p, int dotPos, const QString &lookahead = "");
    static QString productionToString(const Production &p);
    static QString joinIntVector(const QVector<int> &vec);

private:
    // LR(0)
    static QVector<LR1Item> closureLR0(const QVector<LR1Item> &items,
        const QVector<Production> &augProds,
        const QSet<QString> &nonTerminals);
    static QVector<LR1Item> gotoLR0(const QVector<LR1Item> &items, const QString &symbol,
        const QVector<Production> &augProds,
        const QSet<QString> &nonTerminals);
    static bool lr0ItemsEqual(const QVector<LR1Item> &a, const QVector<LR1Item> &b);

    // SLR(1)
    static QStringList checkSLR1(const QVector<LRState> &lr0States,
        const QVector<Production> &augProds,
        const QSet<QString> &terminals,
        const QMap<QString, QSet<QString>> &followSets);

    // LR(1)
    static QVector<LR1Item> closureLR1(const QVector<LR1Item> &items,
        const QVector<Production> &augProds,
        const QSet<QString> &nonTerminals,
        const QMap<QString, QSet<QString>> &firstSets);
    static QVector<LR1Item> gotoLR1(const QVector<LR1Item> &items, const QString &symbol,
        const QVector<Production> &augProds,
        const QSet<QString> &nonTerminals,
        const QMap<QString, QSet<QString>> &firstSets);
    static bool lr1ItemsEqual(const QVector<LR1Item> &a, const QVector<LR1Item> &b);

    // LALR(1)
    static QVector<LRState> mergeToLALR(const QVector<LRState> &lr1States);
    static QVector<LR1Item> lr0Core(const QVector<LR1Item> &items);

    // 顶层构建
    static QVector<LRState> buildLR0States(const QVector<Production> &augProds,
        const QSet<QString> &nonTerminals);
    static QVector<LRState> buildLR1States(const QVector<Production> &augProds,
        const QSet<QString> &nonTerminals,
        const QMap<QString, QSet<QString>> &firstSets);

    // 符号串FIRST
    static QSet<QString> firstOfString(const QStringList &symbols,
        const QMap<QString, QSet<QString>> &firstSets);
};

#endif
