#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Lexer.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QHeaderView>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_btnOpen_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "打开 Rust 文件", "", "Rust Files (*.rs *.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }

    QTextStream in(&file);
    QString source = in.readAll();
    file.close();

    ui->textSource->setPlainText(source);
    processSource(source);
}

void MainWindow::processSource(const QString& source) {
    ui->textResult->clear();
    
    QStringList lines = source.split('\n');
    QString output;
    
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            output += "\n";
            continue;
        }
        
        output += line.trimmed() + "\n";
        
        QString indent = "";
        for (int i = 0; i < line.length(); ++i) {
            if (line[i] == ' ' || line[i] == '\t') indent += line[i];
            else break;
        }
        
        Lexer lexer(line.toStdString());
        vector<Token> tokens = lexer.tokenize();
        
        for (const auto& token : tokens) {
            output += indent + QString::fromStdString(token.text) + ": " + QString::fromStdString(token.typeName) + "\n";
        }
        output += "\n";
    }
    
    ui->textResult->setPlainText(output);
}
