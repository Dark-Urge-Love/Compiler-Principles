#include "graph_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent), m_dfa(nullptr) {
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void GraphWidget::setDFA(Automata::DFAGraph* dfa) {
    m_dfa = dfa;
    updateLayout();
    update();
}

void GraphWidget::resizeEvent(QResizeEvent *) {
    updateLayout();
}

void GraphWidget::updateLayout() {
    m_nodes.clear();
    if (!m_dfa || m_dfa->states.isEmpty()) return;

    int n = m_dfa->states.size();
    // Use 80% of the smaller dimension for the circle radius
    qreal radius = qMin(width(), height()) * 0.35;
    if (radius < 50) radius = 100; // Minimum usable radius

    QPointF center(width() / 2.0, height() / 2.0);

    for (int i = 0; i < n; ++i) {
        qreal angle = 2 * M_PI * i / n - M_PI / 2.0; // Start from top
        QPointF pos(center.x() + radius * qCos(angle), center.y() + radius * qSin(angle));
        m_nodes.push_back({m_dfa->states[i]->id, pos, m_dfa->states[i]->isAccept});
    }
}

void GraphWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_dfa) return;

    painter.setPen(Qt::black);
    qreal nodeRadius = 25;

    // Group transitions by (source, target)
    QMap<QPair<int, int>, QStringList> groupedTransitions;
    for (auto s : m_dfa->states) {
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            groupedTransitions[{s->id, it.value()}].append(QString(char(it.key())));
        }
    }

    // Draw transitions
    for (auto it = groupedTransitions.begin(); it != groupedTransitions.end(); ++it) {
        int srcId = it.key().first;
        int dstId = it.key().second;
        QString label = it.value().join(",");

        QPointF p1, p2;
        bool p1F = false, p2F = false;
        for (const auto& node : m_nodes) {
            if (node.id == srcId) { p1 = node.pos; p1F = true; }
            if (node.id == dstId) { p2 = node.pos; p2F = true; }
        }
        if (!p1F || !p2F) continue;

        QLineF line(p1, p2);
        qreal angle = qAtan2(-line.dy(), line.dx());
        QPointF edgeP1 = p1 + QPointF(nodeRadius * qCos(angle), -nodeRadius * qSin(angle));
        QPointF edgeP2 = p2 - QPointF(nodeRadius * qCos(angle), -nodeRadius * qSin(angle));

        if (srcId == dstId) {
            // Self-loop
            painter.drawArc(p1.x() - nodeRadius, p1.y() - nodeRadius * 2, nodeRadius * 2, nodeRadius * 2, 0 * 16, 180 * 16);
            painter.drawText(p1.x(), p1.y() - nodeRadius * 2, label);
        } else {
            painter.drawLine(edgeP1, edgeP2);
            // Arrow head
            QPointF arrowP1 = edgeP2 + QPointF(10 * qCos(angle + M_PI * 0.8), -10 * qSin(angle + M_PI * 0.8));
            QPointF arrowP2 = edgeP2 + QPointF(10 * qCos(angle - M_PI * 0.8), -10 * qSin(angle - M_PI * 0.8));
            painter.drawPolygon(QPolygonF() << edgeP2 << arrowP1 << arrowP2);
            // Label
            painter.drawText((edgeP1 + edgeP2) / 2 + QPointF(0, -5), label);
        }
    }

    // Draw nodes
    for (const auto& node : m_nodes) {
        painter.setBrush(node.id == m_dfa->startState ? Qt::lightGray : Qt::white);
        painter.drawEllipse(node.pos, nodeRadius, nodeRadius);
        if (node.isAccept) {
            painter.drawEllipse(node.pos, nodeRadius - 4, nodeRadius - 4);
        }
        painter.drawText(QRectF(node.pos.x() - nodeRadius, node.pos.y() - nodeRadius, nodeRadius * 2, nodeRadius * 2), 
                         Qt::AlignCenter, QString::number(node.id));
    }
}
