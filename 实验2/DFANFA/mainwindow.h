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
    void on_runLexButton_clicked();

private:
    Ui::MainWindow *ui;
    Automata::Engine m_engine;
    
    // Lexical analyzer data
    QMap<QString, Automata::DFAGraph*> m_tokenDFAs;
    QMap<QString, int> m_tokenIds;
    QMap<QString, int> m_keywords;

    void fillNFATable(Automata::NFAGraph* nfa);
    void fillDFATable(Automata::DFAGraph* dfa, QTableWidget* table);
    void initKeywords();
    int simulateDFA(Automata::DFAGraph* dfa, const QString& input, int start);
    QString generateMethod1Code(Automata::DFAGraph* dfa);
};
#endif // MAINWINDOW_H
