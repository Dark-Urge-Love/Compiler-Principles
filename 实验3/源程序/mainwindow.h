#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include "compiler.h"

using namespace std;

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
    void on_btnOpen_clicked();
    void on_btnParse_clicked();
    void on_btnClear_clicked();
    void on_btnSwitchView_clicked();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    shared_ptr<ASTNode> currentRoot;

    void buildTreeList(shared_ptr<ASTNode> node, QTreeWidgetItem* parentItem);
    void buildTreeGraph();
    void calculateLayout(shared_ptr<ASTNode> node, qreal& totalWidth);
    void drawNode(shared_ptr<ASTNode> node, qreal x, qreal y);
    pair<QString, QString> getNodeInfo(shared_ptr<ASTNode> node);
    QString getTokenName(TokenType type);
};
#endif
