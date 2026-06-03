#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "Production.h"
#include <QString>
#include <QStringList>
#include <QSet>
#include <QMap>
#include <QVector>

class Grammar {
public:
    QVector<Production> productions;
    QSet<QString> nonTerminals;
    QSet<QString> terminals;
    QString startSymbol;
    static const QString augStart;

    QMap<QString, QSet<QString>> firstSets;
    QMap<QString, QSet<QString>> followSets;

    QStringList sortedTerminals;
    QStringList sortedNonTerminals;

    // 解析文法文本
    bool parse(const QString &text);

    // 计算 FIRST 集
    void computeFirst();

    // 计算 FOLLOW 集
    void computeFollow();

    // 计算符号串的 FIRST 集
    QSet<QString> firstOfString(const QStringList &symbols) const;

private:
    // 将候选式拆分为 token 序列
    void tokenizeRHS(const QString &alt, QStringList &tokens);
};

#endif
