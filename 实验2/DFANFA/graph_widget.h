#ifndef GRAPH_WIDGET_H
#define GRAPH_WIDGET_H

#include <QWidget>
#include <QVector>
#include "automata_engine.h"

class GraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);
    void setDFA(Automata::DFAGraph* dfa);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateLayout();
    Automata::DFAGraph* m_dfa;
    struct NodePos {
        int id;
        QPointF pos;
        bool isAccept;
    };
    QVector<NodePos> m_nodes;
};

#endif // GRAPH_WIDGET_H
