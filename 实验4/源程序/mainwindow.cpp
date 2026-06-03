#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QHeaderView>
#include <QInputDialog>
#include <QRegularExpression>
#include <QFile>
#include <QGroupBox>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("LALR(1) 语法分析生成器");
    resize(1300, 850);

    setStyleSheet(
        "QMainWindow { background-color: #f5f5f5; }"
        "QTextEdit, QLineEdit {"
        "  background-color: #ffffff; border: 1px solid #ccc;"
        "  border-radius: 4px; padding: 4px;"
        "  font-family: 'Consolas','Courier New',monospace; font-size: 13px;"
        "}"
        "QPushButton {"
        "  background-color: #e0e0e0; border: 1px solid #bbb;"
        "  border-radius: 4px; padding: 6px 14px; font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #d0d0d0; }"
        "QPushButton:pressed { background-color: #c0c0c0; }"
        "QPushButton.btn-primary {"
        "  background-color: #d4a056; color: white; font-weight: bold;"
        "}"
        "QPushButton.btn-primary:hover { background-color: #c89040; }"
        "QTabWidget::pane { border: 1px solid #ccc; background: white; }"
        "QTabBar::tab {"
        "  padding: 8px 16px; border: 1px solid #ccc; border-bottom: none;"
        "  background: #e8e8e8; min-width: 64px;"
        "}"
        "QTabBar::tab:selected { background: white; font-weight: bold; }"
        "QTabBar::tab:hover { background: #ddd; }"
        "QTableWidget { gridline-color: #ddd; font-size: 12px; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section {"
        "  background: #e8e8e8; padding: 4px; border: 1px solid #ccc; font-weight: bold;"
        "}"
        "QLabel { font-size: 13px; }"
        "QGroupBox { font-weight: bold; border: 1px solid #ccc; border-radius: 6px; margin-top: 8px; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; }"
    );

    setupUI();
}

MainWindow::~MainWindow() { delete ui; }

static QTableWidget* makeTable(QWidget *parent) {
    QTableWidget *t = new QTableWidget(parent);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->horizontalHeader()->setStretchLastSection(true);
    t->verticalHeader()->setVisible(false);
    t->setAlternatingRowColors(true);
    return t;
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLay = new QVBoxLayout(central);
    mainLay->setContentsMargins(8, 8, 8, 8);
    mainLay->setSpacing(8);

    // 文法输入区
    QGroupBox *inputGroup = new QGroupBox("文法输入", this);
    QVBoxLayout *inputLay = new QVBoxLayout(inputGroup);

    QHBoxLayout *topRow = new QHBoxLayout();
    grammarInput = new QTextEdit(this);
    grammarInput->setPlaceholderText(
        "请输入文法，每行一条产生式。\n"
        "格式示例：\n"
        "  E -> E + T | T\n"
        "  T -> a\n"
        "用 # 表示空串。同一左部的多个候选式可用 | 分隔。"
    );
    grammarInput->setMinimumHeight(130);
    grammarInput->setPlainText("E -> E + T | T\nT -> a");
    topRow->addWidget(grammarInput, 1);

    QVBoxLayout *btnCol = new QVBoxLayout();
    auto makeBtn = [&](const QString &text, void (MainWindow::*slot)()) {
        QPushButton *b = new QPushButton(text, this);
        connect(b, &QPushButton::clicked, this, slot);
        return b;
    };

    QPushButton *btnAnalyze = makeBtn("开始分析", &MainWindow::onAnalyze);
    btnAnalyze->setStyleSheet(
        "QPushButton { background-color: #d4a056; color: white; font-weight: bold; font-size: 14px; padding: 8px 14px; }"
        "QPushButton:hover { background-color: #c89040; }"
    );

    btnCol->addWidget(btnAnalyze);
    btnCol->addWidget(makeBtn("保存文法", &MainWindow::onSaveGrammar));
    btnCol->addWidget(makeBtn("打开文法", &MainWindow::onLoadGrammar));
    btnCol->addWidget(makeBtn("载入用例", &MainWindow::onLoadSampleGrammar));
    btnCol->addWidget(makeBtn("清空", &MainWindow::onClearGrammar));
    btnCol->addStretch();
    topRow->addLayout(btnCol);
    inputLay->addLayout(topRow);

    statusLabel = new QLabel(this);
    statusLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; padding: 4px 0; }");
    inputLay->addWidget(statusLabel);

    mainLay->addWidget(inputGroup);

    tabWidget = new QTabWidget(this);
    setupTab1();   // FIRST/FOLLOW 集
    setupTab2();   // LR(0) DFA
    setupTab3();   // SLR(1) 检查
    setupTab4();   // LR(1) DFA
    setupTab5();   // LALR(1) DFA
    setupTab6();   // LALR(1) 分析表
    setupTab7();   // 句子分析
    mainLay->addWidget(tabWidget, 1);

    setCentralWidget(central);
    onAnalyze();
}

void MainWindow::setupTab1()
{
    QWidget *t = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(t);

    QLabel *firstLabel = new QLabel("FIRST 集", t);
    QFont fl = firstLabel->font(); fl.setBold(true); fl.setPointSize(12);
    firstLabel->setFont(fl);
    firstLabel->setStyleSheet("QLabel { padding: 4px 0; }");
    l->addWidget(firstLabel);

    firstTable = makeTable(t);
    l->addWidget(firstTable);

    QLabel *followLabel = new QLabel("FOLLOW 集", t);
    followLabel->setFont(fl);
    followLabel->setStyleSheet("QLabel { padding: 8px 0 4px 0; }");
    l->addWidget(followLabel);

    followTable = makeTable(t);
    l->addWidget(followTable, 1);

    tabWidget->addTab(t, "FIRST/FOLLOW 集");
}

void MainWindow::setupTab2()
{
    QWidget *t = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(t);
    lr0Table = makeTable(t);
    l->addWidget(lr0Table);
    tabWidget->addTab(t, "LR(0) DFA");
}

void MainWindow::setupTab3()
{
    QWidget *t = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(t);
    slr1Table = makeTable(t);
    l->addWidget(slr1Table);
    tabWidget->addTab(t, "SLR(1) 检查");
}

void MainWindow::setupTab4()
{
    QWidget *t = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(t);
    lr1Table = makeTable(t);
    l->addWidget(lr1Table);
    tabWidget->addTab(t, "LR(1) DFA");
}

void MainWindow::setupTab5()
{
    QWidget *t = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(t);
    lalr1Table = makeTable(t);
    l->addWidget(lalr1Table);
    tabWidget->addTab(t, "LALR(1) DFA");
}

void MainWindow::setupTab6()
{
    QWidget *t = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(t);
    lalrTable = makeTable(t);
    l->addWidget(lalrTable);
    tabWidget->addTab(t, "LALR(1) 分析表");
}

void MainWindow::setupTab7()
{
    QWidget *t = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(t);

    QGroupBox *inputGroup = new QGroupBox("输入待分析句子");
    QVBoxLayout *gLay = new QVBoxLayout(inputGroup);
    QHBoxLayout *row = new QHBoxLayout();
    sentenceInput = new QLineEdit(t);
    sentenceInput->setPlaceholderText("符号之间用空格分隔，例如：a + a");
    connect(sentenceInput, &QLineEdit::returnPressed, this, &MainWindow::onParseSentence);

    parseBtn = new QPushButton("开始分析", t);
    parseBtn->setStyleSheet(
        "QPushButton { background-color: #d4a056; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #c89040; }");
    connect(parseBtn, &QPushButton::clicked, this, &MainWindow::onParseSentence);

    row->addWidget(sentenceInput, 1);
    row->addWidget(parseBtn);
    gLay->addLayout(row);

    parseResultLabel = new QLabel("请先输入句子，然后点击分析按钮。", t);
    parseResultLabel->setWordWrap(true);
    parseResultLabel->setStyleSheet("QLabel { padding: 8px; font-size: 14px; }");
    gLay->addWidget(parseResultLabel);

    l->addWidget(inputGroup);

    QGroupBox *stepGroup = new QGroupBox("分析步骤");
    QVBoxLayout *sLay = new QVBoxLayout(stepGroup);
    parseStepsTable = makeTable(t);
    parseStepsTable->setColumnCount(4);
    parseStepsTable->setHorizontalHeaderLabels({"步骤", "状态栈", "输入", "动作"});
    sLay->addWidget(parseStepsTable);
    l->addWidget(stepGroup, 1);

    tabWidget->addTab(t, "句子分析");
}

void MainWindow::onAnalyze()
{
    QString text = grammarInput->toPlainText();
    if (text.trimmed().isEmpty()) {
        statusLabel->setText("请输入文法。");
        return;
    }
    if (!grammar.parse(text)) {
        statusLabel->setText("错误：未识别到任何产生式，请检查文法格式。");
        return;
    }
    grammar.computeFirst();
    grammar.computeFollow();

    buildResult = LRDFABuilder::buildAll(
        grammar.productions, grammar.nonTerminals, grammar.terminals,
        grammar.startSymbol, grammar.firstSets, grammar.followSets);

    actionTable = LRDFABuilder::buildActionTable(
        buildResult.lalrStates, buildResult.augProductions, grammar.terminals);
    gotoTable = LRDFABuilder::buildGotoTable(
        buildResult.lalrStates, grammar.nonTerminals);

    fillFirstTable();
    fillFollowTable();
    fillLR0();
    fillSLR1();
    fillLR1();
    fillLALR1();
    fillParsingTable();

    parseStepsTable->setRowCount(0);
    parseResultLabel->setText("请先输入句子，然后点击分析按钮。");

    statusLabel->setText(
        QString("分析完成：%1 条产生式，%2 个 LR(0) 状态，%3 个 LR(1) 状态，%4 个 LALR(1) 状态")
            .arg(grammar.productions.size())
            .arg(buildResult.lr0States.size())
            .arg(buildResult.lr1States.size())
            .arg(buildResult.lalrStates.size())
    );
}


void MainWindow::onSaveGrammar()
{
    QString fn = QFileDialog::getSaveFileName(this, "保存文法", "", "文法文件 (*.txt *.grammar);;所有文件 (*)");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法保存文件：" + f.errorString());
        return;
    }
    QTextStream out(&f);
    out << grammarInput->toPlainText();
    f.close();
}

void MainWindow::onLoadGrammar()
{
    QString fn = QFileDialog::getOpenFileName(this, "打开文法", "", "文法文件 (*.txt *.grammar);;所有文件 (*)");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件：" + f.errorString());
        return;
    }
    QTextStream in(&f);
    grammarInput->setPlainText(in.readAll());
    f.close();
    onAnalyze();
}

void MainWindow::onLoadSampleGrammar()
{
    QString fn = QFileDialog::getOpenFileName(this, "选择用例文件", "", "文本文件 (*.txt);;所有文件 (*)");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件：" + f.errorString());
        return;
    }
    QString all = QString::fromUtf8(f.readAll());
    f.close();

    QStringList blocks = all.split("#---", Qt::SkipEmptyParts);
    QVector<QString> samples;
    QStringList labels;

    for (const QString &block : blocks) {
        QString t = block.trimmed();
        if (t.isEmpty()) continue;
        QStringList lines = t.split('\n');
        QString label;
        int start = 0;
        for (int i = 0; i < lines.size(); i++) {
            QString l = lines[i].trimmed();
            if (l.startsWith('#')) { if (label.isEmpty()) label = l.mid(1).trimmed(); }
            else if (!l.isEmpty()) { start = i; break; }
        }
        if (label.isEmpty()) label = QString("用例 %1").arg(samples.size() + 1);
        QStringList cls;
        for (int i = start; i < lines.size(); i++) {
            QString l = lines[i].trimmed();
            if (!l.isEmpty() && !l.startsWith('#')) cls.append(l);
        }
        if (!cls.isEmpty()) { samples.append(cls.join('\n')); labels.append(label); }
    }
    if (samples.isEmpty()) {
        QMessageBox::information(this, "提示", "文件中未找到有效文法用例。");
        return;
    }
    bool ok;
    QString sel = QInputDialog::getItem(this, "选择文法用例", "请选择要载入的用例：", labels, 0, false, &ok);
    if (!ok || sel.isEmpty()) return;
    int idx = labels.indexOf(sel);
    if (idx >= 0) { grammarInput->setPlainText(samples[idx]); onAnalyze(); }
}

void MainWindow::onClearGrammar()
{
    grammarInput->clear();
    firstTable->setRowCount(0); firstTable->setColumnCount(0);
    followTable->setRowCount(0); followTable->setColumnCount(0);
    lr0Table->setRowCount(0); lr0Table->setColumnCount(0);
    slr1Table->setRowCount(0); slr1Table->setColumnCount(0);
    lr1Table->setRowCount(0); lr1Table->setColumnCount(0);
    lalr1Table->setRowCount(0); lalr1Table->setColumnCount(0);
    lalrTable->setRowCount(0); lalrTable->setColumnCount(0);
    parseStepsTable->setRowCount(0);
    parseResultLabel->clear();
    statusLabel->clear();
}

void MainWindow::onParseSentence()
{
    QString input = sentenceInput->text().trimmed();
    if (input.isEmpty()) return;

    QStringList tokens;
    if (input.contains(' ')) {
        tokens = input.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    } else {
        QString cur;
        for (const QChar &ch : input) {
            if (ch.isLetterOrNumber() || ch == '_') { cur += ch; }
            else {
                if (!cur.isEmpty()) { tokens.append(cur); cur.clear(); }
                tokens.append(QString(ch));
            }
        }
        if (!cur.isEmpty()) tokens.append(cur);
    }

    // ==== 写入诊断文件 ====
    {
        QFile f("D:/Users/LiaoYiLin/Documents/LALR/debug_log.txt");
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream s(&f);
        s << "tokens: [" << tokens.join(", ") << "]" << Qt::endl;
        s << "terminals: [" << QStringList(grammar.terminals.begin(), grammar.terminals.end()).join(", ") << "]" << Qt::endl;
        s << "nonTerminals: [" << QStringList(grammar.nonTerminals.begin(), grammar.nonTerminals.end()).join(", ") << "]" << Qt::endl;
        s << "lalrStates count: " << buildResult.lalrStates.size() << Qt::endl;
        s << "actionTable keys (nt): " << actionTable.size() << Qt::endl;
        for (auto it = actionTable.begin(); it != actionTable.end(); ++it) {
            s << "  state " << it.key() << " ACTIONS:";
            if (it.value().isEmpty()) s << " (empty)";
            for (auto it2 = it.value().begin(); it2 != it.value().end(); ++it2)
                s << " [" << it2.key() << ":" << it2.value().type << "(" << it2.value().value << ")]";
            s << Qt::endl;
        }
        s << "augProductions (" << buildResult.augProductions.size() << "):" << Qt::endl;
        for (int i = 0; i < buildResult.augProductions.size(); i++)
            s << "  " << LRDFABuilder::productionToString(buildResult.augProductions[i]) << Qt::endl;
        s << "--- LALR state 0 ---" << Qt::endl;
        if (!buildResult.lalrStates.isEmpty()) {
            const auto &s0 = buildResult.lalrStates[0];
            s << "items(" << s0.items.size() << "):" << Qt::endl;
            for (const auto &itm : s0.items)
                s << "  " << LRDFABuilder::itemToString(buildResult.augProductions[itm.prodIndex], itm.dotPos, itm.lookahead) << Qt::endl;
            s << "transitions(" << s0.transitions.size() << "):" << Qt::endl;
            for (auto it = s0.transitions.begin(); it != s0.transitions.end(); ++it)
                s << "  '" << it.key() << "' -> " << it.value() << Qt::endl;
        } else {
            s << "(no states!)" << Qt::endl;
        }
        s << "--- LALR state 1 ---" << Qt::endl;
        if (buildResult.lalrStates.size() > 1) {
            const auto &s1 = buildResult.lalrStates[1];
            s << "transitions(" << s1.transitions.size() << "):" << Qt::endl;
            for (auto it = s1.transitions.begin(); it != s1.transitions.end(); ++it)
                s << "  '" << it.key() << "' -> " << it.value() << Qt::endl;
        }
        f.close();
    }

    auto pr = LRDFABuilder::parse(tokens, actionTable, gotoTable, buildResult.augProductions);
    fillParseResult(pr);
    tabWidget->setCurrentIndex(6);
}


// ---- FIRST 集 ----
static QString setToString(const QSet<QString> &s) {
    QStringList items(s.begin(), s.end());
    for (QString &x : items) { if (x == "") x = "#"; }
    items.sort();
    return "{ " + items.join(", ") + " }";
}

void MainWindow::fillFirstTable()
{
    firstTable->setColumnCount(2);
    firstTable->setHorizontalHeaderLabels({"非终结符", "FIRST 集"});
    firstTable->setRowCount(grammar.sortedNonTerminals.size());

    int r = 0;
    for (const QString &sym : grammar.sortedNonTerminals) {
        QTableWidgetItem *si = new QTableWidgetItem(sym);
        QFont f = si->font(); f.setBold(true); si->setFont(f);
        firstTable->setItem(r, 0, si);
        if (grammar.firstSets.contains(sym))
            firstTable->setItem(r, 1, new QTableWidgetItem(setToString(grammar.firstSets[sym])));
        r++;
    }
    firstTable->resizeColumnsToContents();
    firstTable->resizeRowsToContents();
}

// ---- FOLLOW 集 ----
void MainWindow::fillFollowTable()
{
    followTable->setColumnCount(2);
    followTable->setHorizontalHeaderLabels({"非终结符", "FOLLOW 集"});
    followTable->setRowCount(grammar.sortedNonTerminals.size());

    int r = 0;
    for (const QString &sym : grammar.sortedNonTerminals) {
        QTableWidgetItem *si = new QTableWidgetItem(sym);
        QFont f = si->font(); f.setBold(true); si->setFont(f);
        followTable->setItem(r, 0, si);
        if (grammar.followSets.contains(sym))
            followTable->setItem(r, 1, new QTableWidgetItem(setToString(grammar.followSets[sym])));
        r++;
    }
    followTable->resizeColumnsToContents();
    followTable->resizeRowsToContents();
}

// ---- LR(0) DFA ----
void MainWindow::fillLR0()
{
    int n = buildResult.lr0States.size();
    lr0Table->setColumnCount(3);
    lr0Table->setHorizontalHeaderLabels({"状态", "项目集", "转移"});

    int rowCount = 0;
    for (const auto &st : buildResult.lr0States) {
        rowCount += 1 + st.items.size(); // 状态标题行 + 每个项目一行
    }
    lr0Table->setRowCount(rowCount);

    int r = 0;
    for (const auto &st : buildResult.lr0States) {
        // 状态标题行
        QString stateLabel = QString("I%1").arg(st.id);
        if (st.id == 0) stateLabel += " (初始)";
        QTableWidgetItem *titleItem = new QTableWidgetItem(stateLabel);
        QFont bf = titleItem->font(); bf.setBold(true); titleItem->setFont(bf);
        for (int c = 0; c < 3; c++) {
            if (c == 0) lr0Table->setItem(r, c, titleItem);
            else { QTableWidgetItem *sep = new QTableWidgetItem(""); sep->setBackground(QColor("#f0f0f0")); lr0Table->setItem(r, c, sep); }
        }
        r++;

        for (const auto &item : st.items) {
            QString itemStr = LRDFABuilder::itemToString(buildResult.augProductions[item.prodIndex], item.dotPos);
            lr0Table->setItem(r, 0, new QTableWidgetItem(""));
            lr0Table->setItem(r, 1, new QTableWidgetItem(itemStr));
            QStringList trans;
            for (auto it = st.transitions.begin(); it != st.transitions.end(); ++it)
                trans.append(QString("%1 -> I%2").arg(it.key()).arg(it.value()));
            lr0Table->setItem(r, 2, new QTableWidgetItem(trans.join("; ")));
            r++;
        }
    }

    lr0Table->resizeColumnsToContents();
    lr0Table->resizeRowsToContents();
}

// ---- SLR(1) 检查 ----
void MainWindow::fillSLR1()
{
    int n = buildResult.lr0States.size();
    slr1Table->setColumnCount(4);
    slr1Table->setHorizontalHeaderLabels({"状态", "移进符号", "归约项目", "FOLLOW 集 / 冲突说明"});

    // 先统计行数
    QVector<QVector<LR1Item>> reducesPerState;
    QVector<QSet<QString>> shiftsPerState;
    for (const auto &st : buildResult.lr0States) {
        QVector<LR1Item> red;
        QSet<QString> shift;
        for (const auto &item : st.items) {
            const auto &p = buildResult.augProductions[item.prodIndex];
            if (item.dotPos == p.rhs.size()) {
                if (item.prodIndex > 0) red.append(item);
            } else if (grammar.terminals.contains(p.rhs[item.dotPos]))
                shift.insert(p.rhs[item.dotPos]);
        }
        reducesPerState.append(red);
        shiftsPerState.append(shift);
    }

    int rowCount = 0;
    for (int i = 0; i < buildResult.lr0States.size(); i++) {
        int redCount = reducesPerState[i].size();
        int shiftCount = shiftsPerState[i].size();
        if (redCount > 0 || shiftCount > 0)
            rowCount += qMax(redCount, (shiftCount > 0 ? 1 : 0));
    }
    rowCount += buildResult.slr1Conflicts.isEmpty() ? 1 : buildResult.slr1Conflicts.size() + 1;

    slr1Table->setRowCount(rowCount);
    int r = 0;

    // 检查结果标题行
    {
        QString result;
        if (buildResult.slr1Conflicts.isEmpty())
            result = "结果：该文法是 SLR(1) 文法，无冲突。";
        else
            result = "结果：该文法不是 SLR(1) 文法。";

        QTableWidgetItem *ti = new QTableWidgetItem(result);
        QFont bf = ti->font(); bf.setBold(true); ti->setFont(bf);
        if (buildResult.slr1Conflicts.isEmpty())
            ti->setBackground(QColor("#e8f5e9"));
        else
            ti->setBackground(QColor("#ffcdd2"));
        slr1Table->setItem(r, 0, ti);
        for (int c = 1; c < 4; c++) {
            QTableWidgetItem *e = new QTableWidgetItem("");
            e->setBackground(ti->background());
            slr1Table->setItem(r, c, e);
        }
        r++;
    }

    // 冲突详情（如果有）
    for (const QString &c : buildResult.slr1Conflicts) {
        if (r >= rowCount) break;
        QTableWidgetItem *ci = new QTableWidgetItem("冲突");
        ci->setBackground(QColor("#ffcdd2")); ci->setForeground(QColor("#c62828"));
        QFont bf = ci->font(); bf.setBold(true); ci->setFont(bf);
        slr1Table->setItem(r, 0, ci);
        slr1Table->setItem(r, 1, new QTableWidgetItem(""));
        slr1Table->setItem(r, 2, new QTableWidgetItem(""));
        QTableWidgetItem *desc = new QTableWidgetItem(c);
        desc->setBackground(QColor("#ffebee"));
        slr1Table->setItem(r, 3, desc);
        r++;
    }

    // 逐状态详情
    for (int i = 0; i < buildResult.lr0States.size(); i++) {
        const auto &red = reducesPerState[i];
        const auto &shift = shiftsPerState[i];
        if (red.isEmpty() && shift.isEmpty()) continue;

        int rows = qMax(red.size(), shift.isEmpty() ? 0 : 1);
        for (int j = 0; j < rows; j++) {
            if (r >= rowCount) break;
            slr1Table->setItem(r, 0, new QTableWidgetItem(QString("I%1").arg(i)));

            if (j < shift.size()) {
                QStringList sl(shift.begin(), shift.end());
                sl.sort();
                slr1Table->setItem(r, 1, new QTableWidgetItem(sl[j]));
            } else {
                slr1Table->setItem(r, 1, new QTableWidgetItem(""));
            }

            if (j < red.size()) {
                const auto &ri = red[j];
                slr1Table->setItem(r, 2, new QTableWidgetItem(
                    LRDFABuilder::productionToString(buildResult.augProductions[ri.prodIndex])));
                QStringList fl(grammar.followSets[buildResult.augProductions[ri.prodIndex].lhs].begin(),
                                grammar.followSets[buildResult.augProductions[ri.prodIndex].lhs].end());
                fl.sort();
                slr1Table->setItem(r, 3, new QTableWidgetItem("{ " + fl.join(", ") + " }"));
            } else {
                if (j == 0) {
                    QStringList sl(shift.begin(), shift.end());
                    sl.sort();
                    slr1Table->setItem(r, 1, new QTableWidgetItem("{ " + sl.join(", ") + " }"));
                }
                slr1Table->setItem(r, 2, new QTableWidgetItem(""));
                slr1Table->setItem(r, 3, new QTableWidgetItem(""));
            }
            r++;
        }
    }

    slr1Table->setRowCount(r);
    slr1Table->resizeColumnsToContents();
    slr1Table->resizeRowsToContents();
}

// ---- LR(1) DFA ----
void MainWindow::fillLR1()
{
    int rowCount = 0;
    for (const auto &st : buildResult.lr1States)
        rowCount += 1 + st.items.size();

    lr1Table->setRowCount(rowCount);
    lr1Table->setColumnCount(3);
    lr1Table->setHorizontalHeaderLabels({"状态", "项目集（带展望符）", "转移"});

    int r = 0;
    for (const auto &st : buildResult.lr1States) {
        QString stateLabel = QString("I%1").arg(st.id);
        if (st.id == 0) stateLabel += " (初始)";
        QTableWidgetItem *ti = new QTableWidgetItem(stateLabel);
        QFont bf = ti->font(); bf.setBold(true); ti->setFont(bf);
        ti->setBackground(QColor("#f3e5f5"));
        for (int c = 0; c < 3; c++) {
            if (c == 0) lr1Table->setItem(r, c, ti);
            else { QTableWidgetItem *e = new QTableWidgetItem(""); e->setBackground(QColor("#f3e5f5")); lr1Table->setItem(r, c, e); }
        }
        r++;
        for (const auto &item : st.items) {
            QString itemStr = LRDFABuilder::itemToString(
                buildResult.augProductions[item.prodIndex], item.dotPos, item.lookahead);
            lr1Table->setItem(r, 0, new QTableWidgetItem(""));
            lr1Table->setItem(r, 1, new QTableWidgetItem(itemStr));
            QStringList trans;
            for (auto it = st.transitions.begin(); it != st.transitions.end(); ++it)
                trans.append(QString("%1 -> I%2").arg(it.key()).arg(it.value()));
            lr1Table->setItem(r, 2, new QTableWidgetItem(trans.join("; ")));
            r++;
        }
    }

    lr1Table->resizeColumnsToContents();
    lr1Table->resizeRowsToContents();
}

// ---- LALR(1) DFA ----
void MainWindow::fillLALR1()
{
    lalr1Table->setColumnCount(4);
    lalr1Table->setHorizontalHeaderLabels({"状态", "项目集（合并展望符）", "合并来源", "转移"});

    struct RowData { QString state, items, source, trans; QColor bg; };
    QVector<RowData> rows;

    // 冲突检测
    bool hasConflict = false;
    QStringList conflictLines;
    for (const auto &st : buildResult.lalrStates) {
        QMap<QString, QSet<QString>> acts;
        for (const auto &item : st.items) {
            const auto &p = buildResult.augProductions[item.prodIndex];
            if (item.dotPos < p.rhs.size()) {
                if (grammar.terminals.contains(p.rhs[item.dotPos]))
                    acts[p.rhs[item.dotPos]].insert("移进");
            } else if (item.prodIndex == 0) { acts["$"].insert("接受"); }
            else { acts[item.lookahead].insert("归约"); }
        }
        for (auto it = acts.begin(); it != acts.end(); ++it) {
            if (it.value().size() >= 2) {
                hasConflict = true;
                conflictLines.append(QString("状态 I%1 在 '%2' 上：%3")
                    .arg(st.id).arg(it.key())
                    .arg(QStringList(it.value().begin(), it.value().end()).join(" vs ")));
            }
        }
    }

    // 冲突检测结果行
    if (hasConflict) {
        rows.append({"合并后冲突", "", "", conflictLines.join("; "), QColor("#ffcdd2")});
    } else {
        rows.append({"合并后无冲突", "LALR(1) 文法有效", "", "", QColor("#e8f5e9")});
    }

    for (const auto &st : buildResult.lalrStates) {
        // 状态标题行
        QString stateLabel = QString("I%1").arg(st.id);
        QString orig;
        if (!st.originalIds.isEmpty()) {
            QStringList ids;
            for (int id : st.originalIds) ids.append(QString::number(id));
            orig = "合并自 LR(1) 状态 " + ids.join(", ");
        }
        rows.append({stateLabel, "", orig, "", QColor("#e8f5e9")});

        // 项目行
        QMap<QString, QSet<QString>> grouped;
        for (const auto &item : st.items)
            grouped[QString::number(item.prodIndex) + ":" + QString::number(item.dotPos)].insert(item.lookahead);

        int itemIdx = 0;
        int total = grouped.size();
        for (auto it = grouped.begin(); it != grouped.end(); ++it) {
            QStringList p = it.key().split(":");
            QStringList las(it.value().begin(), it.value().end()); las.sort();
            QString itemStr = LRDFABuilder::itemToString(
                buildResult.augProductions[p[0].toInt()], p[1].toInt())
                + ", {" + las.join(", ") + "}";

            // 转移信息放在最后一个项目行
            QString transStr;
            if (itemIdx == total - 1) {
                QStringList tl;
                for (auto ti = st.transitions.begin(); ti != st.transitions.end(); ++ti)
                    tl.append(QString("%1 -> I%2").arg(ti.key()).arg(ti.value()));
                transStr = tl.join("; ");
            }
            rows.append({"", itemStr, "", transStr, QColor("#f1f8e9")});
            itemIdx++;
        }
    }

    lalr1Table->setRowCount(rows.size());

    auto mkItem = [](const QString &text, bool bold, const QColor &bg) {
        QTableWidgetItem *it = new QTableWidgetItem(text);
        if (bold) { QFont f = it->font(); f.setBold(true); it->setFont(f); }
        if (bg.isValid()) it->setBackground(bg);
        return it;
    };

    for (int i = 0; i < rows.size(); i++) {
        bool isStateHeader = (i == 0) || (!rows[i].state.isEmpty() && rows[i].items.isEmpty());
        lalr1Table->setItem(i, 0, mkItem(rows[i].state, isStateHeader, rows[i].bg));
        lalr1Table->setItem(i, 1, mkItem(rows[i].items, false, rows[i].bg));
        lalr1Table->setItem(i, 2, mkItem(rows[i].source, false, rows[i].bg));
        lalr1Table->setItem(i, 3, mkItem(rows[i].trans, false, rows[i].bg));
    }
    lalr1Table->resizeColumnsToContents();
    lalr1Table->resizeRowsToContents();
}

// ---- LALR(1) 分析表 ----
void MainWindow::fillParsingTable()
{
    QStringList actLabels;
    for (const QString &t : grammar.sortedTerminals)
        if (t != "$") actLabels.append(t);
    if (!actLabels.contains("$")) actLabels.append("$");

    QStringList gotoLabels;
    for (const QString &nt : grammar.sortedNonTerminals)
        if (nt != Grammar::augStart) gotoLabels.append(nt);

    int R = buildResult.lalrStates.size();
    int C = 1 + actLabels.size() + gotoLabels.size();
    lalrTable->setRowCount(R);
    lalrTable->setColumnCount(C);

    QStringList hdr;
    hdr << "状态";
    for (const auto &l : actLabels) hdr << l;
    for (const auto &l : gotoLabels) hdr << l;
    lalrTable->setHorizontalHeaderLabels(hdr);

    for (int r = 0; r < R; r++) {
        QTableWidgetItem *si = new QTableWidgetItem(QString::number(r));
        si->setTextAlignment(Qt::AlignCenter);
        QFont bf = si->font(); bf.setBold(true); si->setFont(bf);
        lalrTable->setItem(r, 0, si);

        int col = 1;
        for (const QString &t : actLabels) {
            QTableWidgetItem *it = new QTableWidgetItem("");
            it->setTextAlignment(Qt::AlignCenter);
            if (actionTable.contains(r) && actionTable[r].contains(t)) {
                const auto &a = actionTable[r][t];
                if (a.type == "shift") {
                    it->setText("s" + QString::number(a.value));
                    it->setBackground(QColor("#fce4d6"));
                } else if (a.type == "reduce") {
                    it->setText("r" + QString::number(a.value - 1));
                    it->setBackground(QColor("#e8f5e9"));
                } else if (a.type == "accept") {
                    it->setText("acc");
                    it->setBackground(QColor("#fff3e0"));
                } else if (a.type == "conflict") {
                    it->setText("冲突");
                    it->setBackground(QColor("#ffcdd2"));
                }
            }
            lalrTable->setItem(r, col++, it);
        }
        for (const QString &nt : gotoLabels) {
            QTableWidgetItem *it = new QTableWidgetItem("");
            it->setTextAlignment(Qt::AlignCenter);
            if (gotoTable.contains(r) && gotoTable[r].contains(nt)) {
                it->setText(QString::number(gotoTable[r][nt]));
                it->setBackground(QColor("#f3e5f5"));
            }
            lalrTable->setItem(r, col++, it);
        }
    }
    lalrTable->resizeColumnsToContents();
    lalrTable->resizeRowsToContents();
}

void MainWindow::fillParseResult(const LRDFABuilder::ParseResult &pr)
{
    parseStepsTable->setRowCount(pr.steps.size());
    parseStepsTable->setColumnCount(4);
    parseStepsTable->setHorizontalHeaderLabels({"步骤", "状态栈", "输入", "动作"});

    for (int i = 0; i < pr.steps.size(); i++) {
        for (int j = 0; j < 4; j++) {
            QTableWidgetItem *it = new QTableWidgetItem(pr.steps[i][j]);
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            if (j == 3) {
                QString a = pr.steps[i][3];
                if (a.startsWith("移进"))         it->setForeground(QColor("#bf6b23"));
                else if (a.startsWith("归约"))    it->setForeground(QColor("#2e7d32"));
                else if (a == "接受") {
                    it->setForeground(QColor("#e65100"));
                    QFont f = it->font(); f.setBold(true); it->setFont(f);
                } else if (a.startsWith("错误") || a.startsWith("遇到冲突"))
                    it->setForeground(QColor("#c62828"));
            }
            parseStepsTable->setItem(i, j, it);
        }
    }
    parseStepsTable->resizeColumnsToContents();
    parseStepsTable->resizeRowsToContents();

    if (pr.accepted)
        parseResultLabel->setText("<span style='color: #2e7d32; font-weight: bold; font-size: 16px;'>句子符合文法，分析成功。</span>");
    else
        parseResultLabel->setText("<span style='color: #c62828; font-weight: bold; font-size: 16px;'>句子存在语法错误，分析失败。</span>");
}
