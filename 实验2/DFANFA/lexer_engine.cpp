#include "lexer_engine.h"

namespace Lexer {

int LexerEngine::simulateDFA(Automata::DFAGraph* dfa, const QString& input, int start, const QMap<int, QString>& idToAlias, Automata::Engine& engine) {
    if (!dfa) return 0;

    int currentStateId = dfa->startState;
    int lastAcceptLen = 0;
    int currentLen = 0;

    auto findState = [&](int id) -> Automata::DFAState* {
        for (auto s : dfa->states) if (s->id == id) return s;
        return nullptr;
    };

    // 遍历输入字符串进行DFA状态转移
    while (start + currentLen < input.length()) {
        int c = input[start + currentLen].unicode();
        
        Automata::DFAState* s = findState(currentStateId);
        if (!s) break;

        bool found = false;
        if (s->transitions.contains(c)) {
            currentStateId = s->transitions[c];
            found = true;
        } else {
            // 检查别名转换
            for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
                int sym = it.key();
                if (idToAlias.contains(sym)) {
                    QString pattern = engine.getAliasPattern(idToAlias[sym]);
                    if (pattern == "[A-Za-z]" && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
                        currentStateId = it.value();
                        found = true;
                        break;
                    } else if (pattern == "[0-9]" && (c >= '0' && c <= '9')) {
                        currentStateId = it.value();
                        found = true;
                        break;
                    }
                }
            }
        }

        if (!found) break;
        currentLen++;
        
        Automata::DFAState* nextS = findState(currentStateId);
        if (nextS && nextS->isAccept) {
            lastAcceptLen = currentLen;
        }
    }

    return lastAcceptLen;
}

QString LexerEngine::generateStandaloneCode(Automata::DFAGraph* dfa, const QMap<int, QString>& idToAlias, Automata::Engine& engine) {
    if (!dfa || dfa->states.isEmpty()) return "// 请先转换正则表达式以生成代码";

    QString code;
    code += "#include <iostream>\n";
    code += "#include <string.h>\n";
    code += "using namespace std;\n\n";

    code += "int Judgeletter(char ch) {\n";
    code += "    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))\n";
    code += "        return 1;\n";
    code += "    else\n";
    code += "        return 0;\n";
    code += "}\n\n";

    code += "int Judgedigit(char ch) {\n";
    code += "    if (ch >= '0' && ch <= '9')\n";
    code += "        return 1;\n";
    code += "    else\n";
    code += "        return 0;\n";
    code += "}\n\n";

    // 全局变量
    code += "int pos = 0;\n";
    code += "char ch;\n";
    code += "char buffer[1024];\n\n";

    code += "char GetNext() {\n";
    code += "    if (pos < (int)strlen(buffer))\n";
    code += "        return buffer[pos++];\n";
    code += "    return 0;\n";
    code += "}\n\n";

    // 词法分析核心函数
    code += "void GetToken() {\n";
    code += "    int state = " + QString::number(dfa->startState) + ";\n";
    code += "    int lastAcceptId = -1;\n";
    code += "    int lastAcceptPos = -1;\n";
    code += "    int startPos = pos - 1;\n\n";
    
    code += "    while (true) {\n";
    code += "        switch (state) {\n";

    for (auto s : dfa->states) {
        code += QString("            case %1:\n").arg(s->id);
        if (s->isAccept) {
            code += "                lastAcceptId = " + QString::number(s->acceptTokenId) + ";\n";
            code += "                lastAcceptPos = pos;\n";
        }

        if (s->transitions.isEmpty()) {
            code += "                goto end_loop;\n";
            continue;
        }

        QMap<int, QList<int>> targets;
        for (auto it = s->transitions.begin(); it != s->transitions.end(); ++it) {
            targets[it.value()].append(it.key());
        }

        bool first = true;
        for (auto it = targets.begin(); it != targets.end(); ++it) {
            QList<int> chars = it.value();
            QStringList conditions;
            for (int c : chars) {
                if (idToAlias.contains(c)) {
                    QString pattern = engine.getAliasPattern(idToAlias[c]);
                    if (pattern == "[A-Za-z]") conditions << "Judgeletter(ch)";
                    else if (pattern == "[0-9]") conditions << "Judgedigit(ch)";
                    else conditions << QString("(ch == %1)").arg(c);
                } else {
                    conditions << QString("(ch == %1)").arg(c);
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
    code += "        ch = GetNext();\n";
    code += "        if (ch == 0) break;\n";
    code += "    }\n\n";

    code += "end_loop:\n";
    code += "    if (lastAcceptId != -1) {\n";
    code += "        // 回溯到最后一个接受位置\n";
    code += "        pos = lastAcceptPos;\n";
    code += "        cout << lastAcceptId;\n";
    code += "    } else {\n";
    code += "        cout << \"Error\";\n";
    code += "    }\n";
    code += "}\n\n";

    code += "int main() {\n";
    code += "    cout << \"Enter string to analyze: \";\n";
    code += "    cin.getline(buffer, 1024);\n";
    code += "    pos = 0;\n";
    code += "    ch = GetNext();\n";
    code += "    GetToken();\n";
    code += "    cout << endl;\n";
    code += "    return 0;\n";
    code += "}\n";

    return code;
}

}
