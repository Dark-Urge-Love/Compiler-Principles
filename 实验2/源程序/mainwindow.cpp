#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "lexer_engine.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化表格样式
    ui->nfaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->dfaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->minDfaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 设置主分隔条比例
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 4);

    ui->generatedCodeEdit->setReadOnly(true);
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_tokenDFAs);
    delete ui;
}

void MainWindow::on_loadButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择正则表达式文件", "", "Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ui->regexInput->setPlainText(file.readAll());
        }
    }
}

void MainWindow::on_convertButton_clicked()
{
    QString input = ui->regexInput->toPlainText().trimmed();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入正则表达式");
        return;
    }

    // 清理旧状态
    qDeleteAll(m_tokenDFAs);
    m_tokenDFAs.clear();
    m_tokenIds.clear();
    m_idToAlias.clear();
    m_engine.clearAliases();
    m_engine.resetStateCounter();

    QStringList lines = input.split('\n');
    QString lastTerm;
    static QRegularExpression reRule("(.+)_(\\d+)[A-Z]*$");

    // 第一遍处理：注册别名（如 letter=[a-z]）
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith("//")) continue;

        if (line.contains('=')) {
            QString namePart = line.section('=', 0, 0).trimmed();
            QString expr = line.section('=', 1).trimmed();

            bool isRule = reRule.match(namePart).hasMatch() || namePart.startsWith("_");
            if (!isRule) {
                m_engine.setAliasPattern(namePart, expr);
                if (expr.startsWith('[') && expr.endsWith(']')) {
                    int id = 0x1000 + (qHash(namePart) % 0x1000);
                    m_idToAlias[id] = namePart;
                }
            }
            lastTerm = expr;
        } else {
            lastTerm = line;
        }
    }

    // 第二遍处理：为 Token 规则构造 NFA
    QList<Automata::NFAGraph*> nfas;
    QList<int> ids;
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith("//")) continue;

        if (line.contains('=')) {
            QString namePart = line.section('=', 0, 0).trimmed();
            QString expr = line.section('=', 1).trimmed();
            auto match = reRule.match(namePart);
            if (match.hasMatch() || namePart.startsWith("_")) {
                int tokenId = match.hasMatch() ? match.captured(2).toInt() : 100;

                Automata::NFAGraph* nfa = m_engine.regexToNFA(expr);
                if (nfa) {
                    // 为该规则生成独立的 DFA
                    Automata::DFAGraph* dfa = m_engine.nfaToDFA(nfa);
                    Automata::DFAGraph* minDfa = m_engine.minimizeDFA(dfa);
                    m_tokenDFAs[namePart] = minDfa;
                    m_tokenIds[namePart] = tokenId;

                    nfas << nfa;
                    ids << tokenId;
                }
            }
        }
    }

    if (nfas.isEmpty()) {
        if (!lastTerm.isEmpty()) {
            statusBar()->showMessage("正在转换单个正则表达式: " + lastTerm);
            Automata::NFAGraph* nfa = m_engine.regexToNFA(lastTerm);
            if (nfa) {
                Automata::DFAGraph* dfa = m_engine.nfaToDFA(nfa);
                Automata::DFAGraph* minDfa = m_engine.minimizeDFA(dfa);
                fillNFATable(nfa);
                fillDFATable(dfa, ui->dfaTable);
                fillDFATable(minDfa, ui->minDfaTable);
                ui->generatedCodeEdit->setPlainText(Lexer::LexerEngine::generateStandaloneCode(minDfa, m_idToAlias, m_engine));
            }
        }
        return;
    }

    statusBar()->showMessage(QString("正在合并 %1 条规则并生成综合 DFA...").arg(nfas.size()));

    Automata::NFAGraph* displayNfa;
    if (nfas.size() == 1) {
        // 单条规则直接使用
        displayNfa = nfas[0];
    } else {
        // 多条规则合并，建立统一起始状态
        displayNfa = m_engine.combineNFAs(nfas, ids);
    }

    Automata::DFAGraph* combinedDfa = m_engine.nfaToDFA(displayNfa);
    Automata::DFAGraph* minCombinedDfa = m_engine.minimizeDFA(combinedDfa);

    fillNFATable(displayNfa);
    fillDFATable(combinedDfa, ui->dfaTable);
    fillDFATable(minCombinedDfa, ui->minDfaTable);

    ui->generatedCodeEdit->setPlainText(Lexer::LexerEngine::generateStandaloneCode(minCombinedDfa, m_idToAlias, m_engine));
}

void MainWindow::fillNFATable(Automata::NFAGraph* nfa)
{
    ui->nfaTable->clearContents();
    if (!nfa) return;

    QSet<int> alphabet;
    for (auto s : nfa->states) {
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            if (it.key() != Automata::EPSILON) alphabet.insert(it.key());
        }
    }
    QList<int> sortedAlpha = alphabet.values();
    std::sort(sortedAlpha.begin(), sortedAlpha.end());

    ui->nfaTable->setRowCount(nfa->states.size());
    ui->nfaTable->setColumnCount(sortedAlpha.size() + 2);

    QStringList headers;
    headers << "状态" << "ε";
    for (int c : sortedAlpha) {
        if (m_idToAlias.contains(c)) headers << m_idToAlias[c];
        else headers << QString(QChar(c));
    }
    ui->nfaTable->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < nfa->states.size(); ++i) {
        auto s = nfa->states[i];
        QString stateStr = QString::number(s->id);
        if (s->id == nfa->startState) stateStr += " (S)";
        if (s->isAccept) stateStr += " (A)";

        ui->nfaTable->setItem(i, 0, new QTableWidgetItem(stateStr));

        QStringList epsTargets;
        if (s->transitions.contains(Automata::EPSILON)) {
            for (int v : s->transitions[Automata::EPSILON]) epsTargets << QString::number(v);
        }
        ui->nfaTable->setItem(i, 1, new QTableWidgetItem(epsTargets.join(",")));

        for (int j = 0; j < sortedAlpha.size(); ++j) {
            QStringList targets;
            if (s->transitions.contains(sortedAlpha[j])) {
                for (int v : s->transitions[sortedAlpha[j]]) targets << QString::number(v);
            }
            ui->nfaTable->setItem(i, j + 2, new QTableWidgetItem(targets.join(",")));
        }
    }
}

void MainWindow::fillDFATable(Automata::DFAGraph* dfa, QTableWidget* table)
{
    table->clearContents();
    if (!dfa) return;

    QList<int> sortedAlpha = dfa->alphabet.values();
    std::sort(sortedAlpha.begin(), sortedAlpha.end());

    table->setRowCount(dfa->states.size());
    table->setColumnCount(sortedAlpha.size() + 2);

    QStringList headers;
    headers << "状态" << "包含NFA状态";
    for (int c : sortedAlpha) {
        if (m_idToAlias.contains(c)) headers << m_idToAlias[c];
        else headers << QString(QChar(c));
    }
    table->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < dfa->states.size(); ++i) {
        auto s = dfa->states[i];
        QString stateStr = QString::number(s->id);
        if (s->id == dfa->startState) stateStr += " (Start)";
        if (s->isAccept) {
            stateStr += QString(" (Token %1)").arg(s->acceptTokenId);
        }

        table->setItem(i, 0, new QTableWidgetItem(stateStr));

        QStringList nfaList;
        for (int nid : s->nfaStates) nfaList << QString::number(nid);
        table->setItem(i, 1, new QTableWidgetItem("{" + nfaList.join(",") + "}"));

        for (int j = 0; j < sortedAlpha.size(); ++j) {
            if (s->transitions.contains(sortedAlpha[j])) {
                int destId = s->transitions[sortedAlpha[j]];
                QString displayStr = QString::number(destId);

                // For the main DFA table (not minDFA), show NFA state sets in transitions to match reference screenshot
                if (table == ui->dfaTable) {
                    for (auto destS : dfa->states) {
                        if (destS->id == destId) {
                            QStringList targetNfaList;
                            for (int nid : destS->nfaStates) targetNfaList << QString::number(nid);
                            displayStr = "{" + targetNfaList.join(",") + "}";
                            break;
                        }
                    }
                }

                table->setItem(i, j + 2, new QTableWidgetItem(displayStr));
            } else {
                table->setItem(i, j + 2, new QTableWidgetItem("-"));
            }
        }
    }
}
