#include "Grammar.h"
#include <QRegularExpression>

const QString Grammar::augStart = "S'";

bool Grammar::parse(const QString &text)
{
    productions.clear();
    nonTerminals.clear();
    terminals.clear();
    startSymbol.clear();

    QStringList lines = text.split('\n');
    int prodIndex = 0;

    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith("//") || line.startsWith("#"))
            continue;

        line.replace("->", "\t");
        line.replace(QChar(0x2192), '\t'); // →
        line.replace(QChar(0x03B5), '#');  // ε

        int arrowPos = line.indexOf('\t');
        if (arrowPos < 0) {
            arrowPos = line.indexOf("->");
            if (arrowPos >= 0) {
                line = line.left(arrowPos) + '\t' + line.mid(arrowPos + 2);
                arrowPos = line.indexOf('\t');
            }
        }
        if (arrowPos < 0) continue;

        QString lhs = line.left(arrowPos).trimmed();
        if (lhs.isEmpty()) continue;

        nonTerminals.insert(lhs);
        if (startSymbol.isEmpty()) startSymbol = lhs;

        QString rhsPart = line.mid(arrowPos + 1).trimmed();
        // 按|分割候选式
        QStringList alternatives;
        int depth = 0, last = 0;
        for (int i = 0; i < rhsPart.length(); i++) {
            QChar c = rhsPart[i];
            if (c == '(' || c == '[') depth++;
            else if (c == ')' || c == ']') depth--;
            else if (c == '|' && depth == 0) {
                alternatives.append(rhsPart.mid(last, i - last).trimmed());
                last = i + 1;
            }
        }
        alternatives.append(rhsPart.mid(last).trimmed());

        for (QString alt : alternatives) {
            alt = alt.trimmed();
            QStringList tokens;
            tokenizeRHS(alt, tokens);
            productions.append({lhs, tokens, prodIndex++});
        }
    }

    if (productions.isEmpty()) return false;

    // 识别终结符
    for (const auto &prod : productions) {
        for (const auto &sym : prod.rhs) {
            if (!sym.isEmpty() && !nonTerminals.contains(sym)) {
                terminals.insert(sym);
            }
        }
    }

    sortedNonTerminals = QStringList(nonTerminals.begin(), nonTerminals.end());
    sortedNonTerminals.sort();
    sortedTerminals = QStringList(terminals.begin(), terminals.end());
    sortedTerminals.sort();

    return true;
}

void Grammar::tokenizeRHS(const QString &alt, QStringList &tokens)
{
    if (alt == "#" || alt.isEmpty()) return;

    bool hasSpace = false;
    for (const QChar &c : alt) {
        if (c.isSpace()) { hasSpace = true; break; }
    }

    if (hasSpace) {
        QStringList parts = alt.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (const QString &p : parts) {
            if (p != "#") tokens.append(p);
        }
        return;
    }

    bool isIdentifier = true;
    for (const QChar &c : alt) {
        if (!c.isLetterOrNumber() && c != '_') { isIdentifier = false; break; }
    }
    if (isIdentifier && alt.length() > 0) {
        tokens.append(alt);
        return;
    }

    QString current;
    for (const QChar &ch : alt) {
        if (ch == '#') continue;
        if (ch.isLetterOrNumber() || ch == '_') {
            current += ch;
        } else {
            if (!current.isEmpty()) { tokens.append(current); current.clear(); }
            tokens.append(QString(ch));
        }
    }
    if (!current.isEmpty()) tokens.append(current);
}

void Grammar::computeFirst()
{
    firstSets.clear();

    for (const QString &t : terminals)
        firstSets[t].insert(t);
    for (const QString &nt : nonTerminals)
        firstSets[nt] = {};

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &prod : productions) {
            QSet<QString> &firstLHS = firstSets[prod.lhs];

            if (prod.rhs.isEmpty()) {
                if (!firstLHS.contains("")) {
                    firstLHS.insert("");
                    changed = true;
                }
            } else {
                bool allEpsilon = true;
                for (const QString &sym : prod.rhs) {
                    if (!firstSets.contains(sym))
                        firstSets[sym].insert(sym);
                    const QSet<QString> &firstSym = firstSets[sym];
                    bool hasEpsilon = firstSym.contains("");

                    for (const QString &f : firstSym) {
                        if (f != "" && !firstLHS.contains(f)) {
                            firstLHS.insert(f);
                            changed = true;
                        }
                    }
                    if (!hasEpsilon) { allEpsilon = false; break; }
                }
                if (allEpsilon && !firstLHS.contains("")) {
                    firstLHS.insert("");
                    changed = true;
                }
            }
        }
    }
}

QSet<QString> Grammar::firstOfString(const QStringList &symbols) const
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

void Grammar::computeFollow()
{
    followSets.clear();
    for (const QString &nt : nonTerminals)
        followSets[nt] = {};
    followSets[startSymbol].insert("$");

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &prod : productions) {
            const QSet<QString> &followLHS = followSets[prod.lhs];
            for (int i = 0; i < prod.rhs.size(); i++) {
                const QString &sym = prod.rhs[i];
                if (!nonTerminals.contains(sym)) continue;

                QSet<QString> &followSym = followSets[sym];
                QStringList beta = prod.rhs.mid(i + 1);
                QSet<QString> firstBeta = firstOfString(beta);

                for (const QString &f : firstBeta) {
                    if (f != "" && !followSym.contains(f)) {
                        followSym.insert(f);
                        changed = true;
                    }
                }
                if (firstBeta.contains("") || beta.isEmpty()) {
                    for (const QString &f : followLHS) {
                        if (!followSym.contains(f)) {
                            followSym.insert(f);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}
