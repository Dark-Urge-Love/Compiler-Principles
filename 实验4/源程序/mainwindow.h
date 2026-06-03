#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>

#include "Grammar.h"
#include "LRDFA.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyze();
    void onSaveGrammar();
    void onLoadGrammar();
    void onLoadSampleGrammar();
    void onClearGrammar();
    void onParseSentence();

private:
    Ui::MainWindow *ui;

    // UI
    QTabWidget *tabWidget;
    QTextEdit *grammarInput;
    QLabel *statusLabel;
    QLineEdit *sentenceInput;
    QPushButton *parseBtn;
    QLabel *parseResultLabel;

    // 各选项卡用 QTableWidget 表格
    QTableWidget *firstTable;           // FIRST 集
    QTableWidget *followTable;          // FOLLOW 集
    QTableWidget *lr0Table;             // LR(0) DFA
    QTableWidget *slr1Table;            // SLR(1) 检查结果
    QTableWidget *lr1Table;             // LR(1) DFA
    QTableWidget *lalr1Table;           // LALR(1) DFA
    QTableWidget *lalrTable;            // LALR(1) 分析表
    QTableWidget *parseStepsTable;      // 句子分析步骤

    // 数据
    Grammar grammar;
    LRDFABuilder::BuildResult buildResult;
    QMap<int, QMap<QString, ParsingAction>> actionTable;
    QMap<int, QMap<QString, int>> gotoTable;

    // UI 构建
    void setupUI();
    void setupTab1();  // FIRST/FOLLOW 集
    void setupTab2();  // LR(0) DFA
    void setupTab3();  // SLR(1) 检查
    void setupTab4();  // LR(1) DFA
    void setupTab5();  // LALR(1) DFA
    void setupTab6();  // LALR(1) 分析表
    void setupTab7();  // 句子分析

    // 表格填充
    void fillFirstTable();
    void fillFollowTable();
    void fillLR0();
    void fillSLR1();
    void fillLR1();
    void fillLALR1();
    void fillParsingTable();
    void fillParseResult(const LRDFABuilder::ParseResult &pr);
};

#endif // MAINWINDOW_H
