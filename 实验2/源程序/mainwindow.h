#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QMap>
#include "automata_engine.h"

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
    void on_convertButton_clicked();
    void on_loadButton_clicked();

private:
    Ui::MainWindow *ui;
    // 自动机引擎实例
    Automata::Engine m_engine;
    
    // 词法分析器数据
    QMap<QString, Automata::DFAGraph*> m_tokenDFAs;
    // 规则名称到 Token ID 的映射
    QMap<QString, int> m_tokenIds;
    // 符号 ID 到别名的映射（用于表格显示）
    QMap<int, QString> m_idToAlias;

    void fillNFATable(Automata::NFAGraph* nfa);
    void fillDFATable(Automata::DFAGraph* dfa, QTableWidget* table);
};
#endif // MAINWINDOW_H
