#ifndef PRODUCTION_H
#define PRODUCTION_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSet>

// 产生式结构
struct Production {
    QString lhs;   // 左部（非终结符）
    QStringList rhs;  // 右部符号序列（空 = epsilon）
    int index = 0;  // 编号
};

// LR(1) 项目（LR(0) 项目 = lookahead 为空）
struct LR1Item {
    int prodIndex = 0; // 产生式在 augProductions 中的下标
    int dotPos = 0;  // 圆点位置
    QString lookahead; // 展望符（LR(1) 使用，LR(0) 忽略）
};

// LR 状态（适用于 LR(0) / LR(1) / LALR(1)）
struct LRState {
    int id = 0;
    QVector<LR1Item> items;
    QMap<QString, int> transitions; // symbol -> target state
    QVector<int> originalIds;  // LALR 合并前来自哪些 LR(1) 状态
};

// 分析表
struct ParsingAction {
    QString type;
    int value = -1;
};

#endif
