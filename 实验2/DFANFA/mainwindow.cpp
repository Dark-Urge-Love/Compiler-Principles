#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Clean UI settings
    ui->nfaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->dfaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->minDfaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->lexResultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Set initial splitter stretches: left (input) smaller, right (results) larger
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 4);

    // Set initial lex splitter stretches: top (code) and bottom (test) equal
    ui->lexSplitter->setStretchFactor(0, 1);
    ui->lexSplitter->setStretchFactor(1, 1);

    initKeywords();
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_tokenDFAs);
    delete ui;
}

void MainWindow::initKeywords()
{
    // C/C++ common keywords
    m_keywords["if"] = 1;
    m_keywords["else"] = 2;
    m_keywords["for"] = 3;
    m_keywords["while"] = 4;
    m_keywords["do"] = 5;
    m_keywords["return"] = 6;
    m_keywords["switch"] = 7;
    m_keywords["case"] = 8;
    m_keywords["break"] = 9;
    m_keywords["default"] = 10;
    m_keywords["int"] = 11;
    m_keywords["float"] = 12;
    m_keywords["char"] = 13;
    m_keywords["double"] = 14;
    m_keywords["void"] = 15;
    m_keywords["using"] = 16;
    m_keywords["namespace"] = 17;
    m_keywords["include"] = 18;
    m_keywords["iostream"] = 19;
    m_keywords["string"] = 20;
    m_keywords["std"] = 21;
    m_keywords["main"] = 22;
    m_keywords["cout"] = 23;
    m_keywords["cin"] = 24;
    m_keywords["endl"] = 25;
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

    // Clear old token DFAs
    qDeleteAll(m_tokenDFAs);
    m_tokenDFAs.clear();
    m_tokenIds.clear();

    // Simple parser for named regexes
    QMap<QString, QString> definitions;
    QStringList lines = input.split('\n');
    QString lastRegex;
    
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith("//")) continue;
        
        if (line.contains('=')) {
            QString namePart = line.section('=', 0, 0).trimmed();
            QString expr = line.section('=', 1).trimmed();
            
            // Substitute previous definitions
            for (auto it = definitions.begin(); it != definitions.end(); ++it) {
                expr.replace(it.key(), "(" + it.value() + ")");
            }
            definitions[namePart] = expr;
            lastRegex = expr;

            // Check for ID suffix (e.g., name_100)
            static QRegularExpression re("(.+)_(\\d+)[A-Z]*$");
            auto match = re.match(namePart);
            if (match.hasMatch()) {
                QString baseName = match.captured(1);
                int id = match.captured(2).toInt();
                
                Automata::NFAGraph* nfa = m_engine.regexToNFA(expr);
                if (nfa) {
                    Automata::DFAGraph* dfa = m_engine.nfaToDFA(nfa);
                    Automata::DFAGraph* minDfa = m_engine.minimizeDFA(dfa);
                    m_tokenDFAs[namePart] = minDfa;
                    m_tokenIds[namePart] = id;
                }
            }
        } else {
            lastRegex = line;
            for (auto it = definitions.begin(); it != definitions.end(); ++it) {
                lastRegex.replace(it.key(), "(" + it.value() + ")");
            }
        }
    }

    if (lastRegex.isEmpty()) return;

    statusBar()->showMessage("正在转换最后一个正则表达式: " + lastRegex);

    // Update tables for the last defined regex
    Automata::NFAGraph* nfa = m_engine.regexToNFA(lastRegex);
    if (!nfa) {
        QMessageBox::critical(this, "错误", "正则表达式解析失败");
        return;
    }

    Automata::DFAGraph* dfa = m_engine.nfaToDFA(nfa);
    Automata::DFAGraph* minDfa = m_engine.minimizeDFA(dfa);

    fillNFATable(nfa);
    fillDFATable(dfa, ui->dfaTable);
    fillDFATable(minDfa, ui->minDfaTable);

    // Generate code for the minimized DFA
    ui->generatedCodeEdit->setPlainText(generateMethod1Code(minDfa));
}

QString MainWindow::generateMethod1Code(Automata::DFAGraph* dfa)
{
    if (!dfa || dfa->states.isEmpty()) return "// 请先转换一个正规式以生成代码";

    QString code;
    code += "// 自动生成的词法分析程序 (方法一: Switch-Case 状态迁移)\n";
    code += "#include <iostream>\n#include <string>\n#include <vector>\n#include <algorithm>\n\n";
    code += "using namespace std;\n\n";
    
    code += "// 获取单词函数 (基于转换后的最小化 DFA)\n";
    code += "int GetToken(const string& input, int& pos) {\n";
    code += "    int state = " + QString::number(dfa->startState) + ";\n";
    code += "    int lastAcceptPos = -1;\n";
    code += "    int currentPos = pos;\n\n";
    
    code += "    while (currentPos <= (int)input.length()) {\n";
    code += "        char c = (currentPos < (int)input.length()) ? input[currentPos] : 0;\n";
    code += "        switch (state) {\n";
    
    for (auto s : dfa->states) {
        code += QString("            case %1:\n").arg(s->id);
        if (s->isAccept) {
            code += "                lastAcceptPos = currentPos;\n";
        }
        
        if (s->transitions.isEmpty()) {
            code += "                goto end_loop;\n";
            continue;
        }

        // Group transitions by target state
        QMap<int, QList<int>> targets;
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            targets[it.value()].append(it.key());
        }
        
        bool first = true;
        for (auto it = targets.begin(); it != targets.end(); ++it) {
            QList<int> chars = it.value();
            std::sort(chars.begin(), chars.end());
            
            QStringList conditions;
            for (int i = 0; i < chars.size(); ) {
                int j = i + 1;
                while (j < chars.size() && chars[j] == chars[j-1] + 1) j++;
                
                if (j - i > 2) { // Use range
                    conditions << QString("(c >= %1 && c <= %2)").arg(chars[i]).arg(chars[j-1]);
                    i = j;
                } else {
                    conditions << QString("c == %1").arg(chars[i]);
                    i++;
                }
            }
            
            code += QString("                %1 (%2) state = %3;\n")
                        .arg(first ? "if" : "else if")
                        .arg(conditions.join(" || "))
                        .arg(it.key());
            first = false;
        }
        code += "                else goto end_loop;\n";
        code += "                break;\n";
    }
    
    code += "        }\n";
    code += "        currentPos++;\n";
    code += "    }\n\n";
    
    code += "end_loop:\n";
    code += "    if (lastAcceptPos != -1) {\n";
    code += "        pos = lastAcceptPos + 1;\n";
    code += "        return 1; // 匹配成功\n";
    code += "    }\n";
    code += "    return -1; // 匹配失败\n";
    code += "}\n\n";
    
    code += "int main() {\n";
    code += "    string input;\n";
    code += "    cout << \"请输入待分析字符串: \";\n";
    code += "    getline(cin, input);\n\n";
    code += "    int pos = 0;\n";
    code += "    if (GetToken(input, pos) == 1 && pos == (int)input.length()) {\n";
    code += "        cout << \"识别成功! 结果: \" << input << endl;\n";
    code += "    } else {\n";
    code += "        cout << \"识别失败!\" << endl;\n";
    code += "    }\n";
    code += "    return 0;\n";
    code += "}\n";
    
    return code;
}

void MainWindow::on_runLexButton_clicked()
{
    if (m_tokenDFAs.isEmpty()) {
        on_convertButton_clicked(); // Try to compile rules first
        if (m_tokenDFAs.isEmpty()) {
            QMessageBox::warning(this, "警告", "没有定义有效的 Token 规则（需使用 `名称_ID=正规式` 格式）");
            return;
        }
    }

    QString source = ui->generatedCodeEdit->toPlainText();
    ui->lexResultTable->setRowCount(0);

    int pos = 0;
    int lineNum = 1;
    int row = 0;

    statusBar()->showMessage("正在运行词法分析...");

    while (pos < source.length()) {
        // Skip whitespace
        if (source[pos].isSpace()) {
            if (source[pos] == '\n') lineNum++;
            pos++;
            continue;
        }

        int longestMatch = 0;
        QString bestTokenName;

        // Try every token DFA
        for (auto it = m_tokenDFAs.begin(); it != m_tokenDFAs.end(); ++it) {
            int len = simulateDFA(it.value(), source, pos);
            if (len > longestMatch) {
                longestMatch = len;
                bestTokenName = it.key();
            }
        }

        if (longestMatch > 0) {
            QString word = source.mid(pos, longestMatch);
            int id = m_tokenIds[bestTokenName];
            QString attr = word;

            // If it's an ID token, check for keywords
            if (bestTokenName.startsWith("ID", Qt::CaseInsensitive)) {
                if (m_keywords.contains(word)) {
                    id = m_keywords[word];
                    attr = "Keyword";
                } else {
                    attr = word;
                }
            } else if (bestTokenName.startsWith("num", Qt::CaseInsensitive)) {
                attr = word;
            }

            // Fill table
            ui->lexResultTable->insertRow(row);
            ui->lexResultTable->setItem(row, 0, new QTableWidgetItem(QString::number(lineNum)));
            ui->lexResultTable->setItem(row, 1, new QTableWidgetItem(word));
            ui->lexResultTable->setItem(row, 2, new QTableWidgetItem(QString::number(id)));
            ui->lexResultTable->setItem(row, 3, new QTableWidgetItem(attr));

            pos += longestMatch;
            row++;
        } else {
            // Handle undefined symbols more gracefully
            QChar c = source[pos];
            QString sym = QString(c);
            
            ui->lexResultTable->insertRow(row);
            ui->lexResultTable->setItem(row, 0, new QTableWidgetItem(QString::number(lineNum)));
            ui->lexResultTable->setItem(row, 1, new QTableWidgetItem(sym));

            // Assign ASCII as ID for common symbols
            if (c.isPunct() || c.isSymbol()) {
                ui->lexResultTable->setItem(row, 2, new QTableWidgetItem(QString::number(c.unicode())));
                ui->lexResultTable->setItem(row, 3, new QTableWidgetItem(sym));
            } else {
                ui->lexResultTable->setItem(row, 2, new QTableWidgetItem("ERROR"));
                ui->lexResultTable->setItem(row, 3, new QTableWidgetItem("识别失败"));
                ui->lexResultTable->item(row, 1)->setForeground(Qt::red);
            }
            
            pos++;
            row++;
        }
    }
    statusBar()->showMessage("词法分析完成");
}

int MainWindow::simulateDFA(Automata::DFAGraph* dfa, const QString& input, int start)
{
    if (!dfa) return 0;

    int currentStateId = dfa->startState;
    int lastAcceptLen = 0;
    int currentLen = 0;

    auto findState = [&](int id) -> Automata::DFAState* {
        for (auto s : dfa->states) if (s->id == id) return s;
        return nullptr;
    };

    while (start + currentLen < input.length()) {
        int c = input[start + currentLen].unicode();
        
        Automata::DFAState* s = findState(currentStateId);
        if (!s || !s->transitions.contains(c)) break;

        currentStateId = s->transitions[c];
        currentLen++;
        
        Automata::DFAState* nextS = findState(currentStateId);
        if (nextS && nextS->isAccept) {
            lastAcceptLen = currentLen;
        }
    }

    return lastAcceptLen;
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
    for (int c : sortedAlpha) headers << QString(char(c));
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
    for (int c : sortedAlpha) headers << QString(char(c));
    table->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < dfa->states.size(); ++i) {
        auto s = dfa->states[i];
        QString stateStr = QString::number(s->id);
        if (s->id == dfa->startState) stateStr += " (Start)";
        if (s->isAccept) stateStr += " (Accept)";
        
        table->setItem(i, 0, new QTableWidgetItem(stateStr));
        
        QStringList nfaList;
        for (int nid : s->nfaStates) nfaList << QString::number(nid);
        table->setItem(i, 1, new QTableWidgetItem("{" + nfaList.join(",") + "}"));
        
        for (int j = 0; j < sortedAlpha.size(); ++j) {
            if (s->transitions.contains(sortedAlpha[j])) {
                table->setItem(i, j + 2, new QTableWidgetItem(QString::number(s->transitions[sortedAlpha[j]])));
            } else {
                table->setItem(i, j + 2, new QTableWidgetItem("-"));
            }
        }
    }
}
