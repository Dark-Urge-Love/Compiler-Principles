#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <cmath>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene(this))
    , currentRoot(nullptr)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(scene);

    ui->splitter_h->setStretchFactor(0, 1);
    ui->splitter_h->setStretchFactor(1, 3);

    ui->tableErrors->horizontalHeader()->setStretchLastSection(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnOpen_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开TINY源程序", "", "TINY Files (*.tny);;Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            ui->textEditSource->setPlainText(in.readAll());
        }
    }
}

void MainWindow::on_btnParse_clicked()
{
    ui->treeWidget->clear();
    scene->clear();
    ui->tableErrors->setRowCount(0);
    currentRoot = nullptr;

    string source = ui->textEditSource->toPlainText().toStdString();
    if (source.empty()) return;

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    auto lexErrors = lexer.getErrors();

    if (!lexErrors.empty()) {
        for (const auto& err : lexErrors) {
            int row = ui->tableErrors->rowCount();
            ui->tableErrors->insertRow(row);
            ui->tableErrors->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(err.message)));
            ui->tableErrors->setItem(row, 1, new QTableWidgetItem("Source"));
            ui->tableErrors->setItem(row, 2, new QTableWidgetItem(QString::number(err.line)));
            ui->tableErrors->setItem(row, 3, new QTableWidgetItem(QString::number(err.column)));
        }
        return;
    }

    Parser parser(tokens);
    currentRoot = parser.parse();
    auto parseErrors = parser.getErrors();

    if (!parseErrors.empty()) {
        for (const auto& err : parseErrors) {
            int row = ui->tableErrors->rowCount();
            ui->tableErrors->insertRow(row);
            ui->tableErrors->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(err.message)));
            ui->tableErrors->setItem(row, 1, new QTableWidgetItem("Source"));
            ui->tableErrors->setItem(row, 2, new QTableWidgetItem(QString::number(err.line)));
            ui->tableErrors->setItem(row, 3, new QTableWidgetItem(QString::number(err.column)));
        }
        return;
    }

    if (currentRoot) {
        buildTreeList(currentRoot, nullptr);
        ui->treeWidget->expandAll();
        buildTreeGraph();
    }
}

void MainWindow::on_btnClear_clicked()
{
    ui->textEditSource->clear();
    ui->treeWidget->clear();
    ui->tableErrors->setRowCount(0);
    scene->clear();
    currentRoot = nullptr;
}

void MainWindow::on_btnSwitchView_clicked()
{
    if (ui->stackedWidget->currentIndex() == 0) {
        ui->stackedWidget->setCurrentIndex(1);
        ui->btnSwitchView->setText("切换到列表视图");
    } else {
        ui->stackedWidget->setCurrentIndex(0);
        ui->btnSwitchView->setText("切换到图形树状图");
    }
}

void MainWindow::buildTreeList(shared_ptr<ASTNode> node, QTreeWidgetItem* parentItem)
{
    while (node) {
        auto info = getNodeInfo(node);
        QString text = info.first;
        if (!info.second.isEmpty()) text += info.second;
        
        QTreeWidgetItem* item;
        if (parentItem) {
            item = new QTreeWidgetItem(parentItem);
        } else {
            item = new QTreeWidgetItem(ui->treeWidget);
        }
        item->setText(0, text);

        for (const auto& child : node->children) {
            buildTreeList(child, item);
        }
        node = node->sibling;
    }
}

void MainWindow::calculateLayout(shared_ptr<ASTNode> node, qreal& totalWidth)
{
    if (!node) {
        totalWidth = 0;
        return;
    }

    const qreal nodeWidth = 80;
    const qreal horizontalSpacing = 20;

    qreal visualAreaWidth = 0;
    if (node->children.empty()) {
        visualAreaWidth = nodeWidth;
    } else {
        qreal childrenWidth = 0;
        for (const auto& child : node->children) {
            qreal cw = 0;
            calculateLayout(child, cw);
            childrenWidth += cw;
        }
        childrenWidth += (node->children.size() - 1) * horizontalSpacing;
        visualAreaWidth = max(nodeWidth, childrenWidth);
    }

    if (node->sibling) {
        qreal sw = 0;
        calculateLayout(node->sibling, sw);
        totalWidth = visualAreaWidth + horizontalSpacing + sw;
    } else {
        totalWidth = visualAreaWidth;
    }
}

void MainWindow::buildTreeGraph()
{
    scene->clear();
    if (!currentRoot) return;

    qreal totalWidth = 0;
    calculateLayout(currentRoot, totalWidth);

    drawNode(currentRoot, 100, 100);
}

void MainWindow::drawNode(shared_ptr<ASTNode> node, qreal x, qreal y)
{
    if (!node) return;

    const qreal nodeWidth = 70;
    const qreal nodeHeight = 50;
    const qreal horizontalSpacing = 20;
    const qreal verticalSpacing = 80;

    auto info = getNodeInfo(node);
    QString label = info.first;
    if (!info.second.isEmpty()) label += "\n" + info.second;

    if (node->nodeKind == NodeKind::StmtK) {
        scene->addRect(x - nodeWidth/2, y - nodeHeight/2, nodeWidth, nodeHeight, QPen(Qt::black), QBrush(Qt::white));
    } else {
        scene->addEllipse(x - nodeWidth/2, y - nodeHeight/2, nodeWidth, nodeHeight, QPen(Qt::black), QBrush(Qt::white));
    }

    auto textItem = scene->addText(label);
    textItem->setPos(x - textItem->boundingRect().width()/2, y - textItem->boundingRect().height()/2);

    if (!node->children.empty()) {
        qreal totalChildrenWidth = 0;
        vector<qreal> childWidths;
        for (const auto& child : node->children) {
            qreal cw = 0;
            calculateLayout(child, cw);
            childWidths.push_back(cw);
            totalChildrenWidth += cw;
        }
        totalChildrenWidth += (node->children.size() - 1) * horizontalSpacing;

        qreal currentChildX = x - totalChildrenWidth / 2;
        qreal childY = y + verticalSpacing;

        for (size_t i = 0; i < node->children.size(); ++i) {
            qreal childCenterX = currentChildX + childWidths[i] / 2;
            scene->addLine(x, y + nodeHeight/2, childCenterX, childY - nodeHeight/2);
            drawNode(node->children[i], childCenterX, childY);
            currentChildX += childWidths[i] + horizontalSpacing;
        }
    }

    if (node->sibling) {
        qreal currentVisualAreaWidth = 0;
        if (node->children.empty()) {
            currentVisualAreaWidth = nodeWidth;
        } else {
            for (const auto& child : node->children) {
                qreal cw = 0;
                calculateLayout(child, cw);
                currentVisualAreaWidth = max(currentVisualAreaWidth, cw);
            }
            qreal cw_sum = 0;
            for (const auto& child : node->children) {
                qreal cw = 0;
                calculateLayout(child, cw);
                cw_sum += cw;
            }
            cw_sum += (node->children.size() - 1) * horizontalSpacing;
            currentVisualAreaWidth = max(nodeWidth, cw_sum);
        }

        qreal sibX = x + currentVisualAreaWidth / 2 + horizontalSpacing + nodeWidth / 2;
        qreal nextSibVisualWidth = 0;
        qreal nextSibWidth = 0;
        qreal sibFirstVisualWidth = 0;
        if (node->sibling->children.empty()) {
            sibFirstVisualWidth = nodeWidth;
        } else {
            qreal scw = 0;
            for (const auto& sc : node->sibling->children) {
                qreal scw_val = 0;
                calculateLayout(sc, scw_val);
                scw += scw_val;
            }
            scw += (node->sibling->children.size() - 1) * horizontalSpacing;
            sibFirstVisualWidth = max(nodeWidth, scw);
        }

        qreal offset = currentVisualAreaWidth / 2 + horizontalSpacing + sibFirstVisualWidth / 2;
        qreal nextX = x + offset;
        
        scene->addLine(x + nodeWidth/2, y, nextX - nodeWidth/2, y);
        drawNode(node->sibling, nextX, y);
    }
}

pair<QString, QString> MainWindow::getNodeInfo(shared_ptr<ASTNode> node)
{
    if (node->nodeKind == NodeKind::StmtK) {
        auto stmt = static_pointer_cast<StmtNode>(node);
        switch (stmt->stmtKind) {
            case StmtKind::IfK: return {"if", ""};
            case StmtKind::RepeatK: return {"repeat", ""};
            case StmtKind::AssignK: return {"assign", QString("(%1)").arg(QString::fromStdString(stmt->name))};
            case StmtKind::RegexK: return {"regex", QString("(%1)").arg(QString::fromStdString(stmt->name))};
            case StmtKind::ReadK: return {"read", QString("(%1)").arg(QString::fromStdString(stmt->name))};
            case StmtKind::WriteK: return {"write", ""};
            case StmtKind::WhileK: return {"while", ""};
            case StmtKind::ForK: return {"for", ""};
            case StmtKind::IncrK: return {"incr", QString("(%1)").arg(QString::fromStdString(stmt->name))};
        }
    } else {
        auto exp = static_pointer_cast<ExpNode>(node);
        switch (exp->expKind) {
            case ExpKind::OpK: return {"op", QString("(%1)").arg(getTokenName(exp->op))};
            case ExpKind::RegexExpK: return {"regex", QString("(%1)").arg(getTokenName(exp->op))};
            case ExpKind::ConstK: return {"const", QString("(%1)").arg(exp->val)};
            case ExpKind::IdK: return {"id", QString("(%1)").arg(QString::fromStdString(exp->name))};
        }
    }
    return {"unknown", ""};
}

QString MainWindow::getTokenName(TokenType type) {
    switch (type) {
        case TokenType::IF: return "if";
        case TokenType::THEN: return "then";
        case TokenType::ELSE: return "else";
        case TokenType::END: return "end";
        case TokenType::REPEAT: return "repeat";
        case TokenType::UNTIL: return "until";
        case TokenType::READ: return "read";
        case TokenType::WRITE: return "write";
        case TokenType::WHILE: return "while";
        case TokenType::ENDWHILE: return "endwhile";
        case TokenType::FOR: return "for";
        case TokenType::ENDFOR: return "endfor";
        case TokenType::DO: return "do";
        case TokenType::ID: return "id";
        case TokenType::NUM: return "num";
        case TokenType::ASSIGN: return ":=";
        case TokenType::EQ: return "="; // Image shows = for equality
        case TokenType::LT: return "<";
        case TokenType::LE: return "<=";
        case TokenType::GT: return ">";
        case TokenType::GE: return ">=";
        case TokenType::NE: return "<>";
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::TIMES: return "*";
        case TokenType::OVER: return "/";
        case TokenType::MOD: return "%";
        case TokenType::POWER: return "^";
        case TokenType::REG_OR: return "|";
        case TokenType::REG_AND: return "&";
        case TokenType::REG_EMPTY: return "#";
        case TokenType::REG_QUERY: return "?";
        case TokenType::REG_ASSIGN: return "::=";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::SEMI: return ";";
        case TokenType::PLUSPLUS: return "++";
        case TokenType::MINUSMINUS: return "--";
        case TokenType::ENDFILE: return "eof";
        case TokenType::ERROR: return "error";
        default: return "unknown";
    }
}
