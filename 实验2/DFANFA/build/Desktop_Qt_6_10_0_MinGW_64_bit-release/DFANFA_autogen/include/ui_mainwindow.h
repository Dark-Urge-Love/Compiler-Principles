/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QSplitter *splitter;
    QWidget *leftContainer;
    QVBoxLayout *leftVerticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpacerItem *horizontalSpacer;
    QPushButton *loadButton;
    QPushButton *convertButton;
    QPlainTextEdit *regexInput;
    QTabWidget *tabWidget;
    QWidget *nfaTab;
    QVBoxLayout *verticalLayout_2;
    QTableWidget *nfaTable;
    QWidget *dfaTab;
    QVBoxLayout *verticalLayout_3;
    QTableWidget *dfaTable;
    QWidget *minDfaTab;
    QVBoxLayout *verticalLayout_4;
    QTableWidget *minDfaTable;
    QWidget *lexTab;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_3;
    QPlainTextEdit *generatedCodeEdit;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1000, 800);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        splitter = new QSplitter(centralwidget);
        splitter->setObjectName("splitter");
        splitter->setOrientation(Qt::Orientation::Horizontal);
        leftContainer = new QWidget(splitter);
        leftContainer->setObjectName("leftContainer");
        leftVerticalLayout = new QVBoxLayout(leftContainer);
        leftVerticalLayout->setObjectName("leftVerticalLayout");
        leftVerticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(leftContainer);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        loadButton = new QPushButton(leftContainer);
        loadButton->setObjectName("loadButton");

        horizontalLayout->addWidget(loadButton);

        convertButton = new QPushButton(leftContainer);
        convertButton->setObjectName("convertButton");

        horizontalLayout->addWidget(convertButton);


        leftVerticalLayout->addLayout(horizontalLayout);

        regexInput = new QPlainTextEdit(leftContainer);
        regexInput->setObjectName("regexInput");

        leftVerticalLayout->addWidget(regexInput);

        splitter->addWidget(leftContainer);
        tabWidget = new QTabWidget(splitter);
        tabWidget->setObjectName("tabWidget");
        nfaTab = new QWidget();
        nfaTab->setObjectName("nfaTab");
        verticalLayout_2 = new QVBoxLayout(nfaTab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        nfaTable = new QTableWidget(nfaTab);
        nfaTable->setObjectName("nfaTable");

        verticalLayout_2->addWidget(nfaTable);

        tabWidget->addTab(nfaTab, QString());
        dfaTab = new QWidget();
        dfaTab->setObjectName("dfaTab");
        verticalLayout_3 = new QVBoxLayout(dfaTab);
        verticalLayout_3->setObjectName("verticalLayout_3");
        dfaTable = new QTableWidget(dfaTab);
        dfaTable->setObjectName("dfaTable");

        verticalLayout_3->addWidget(dfaTable);

        tabWidget->addTab(dfaTab, QString());
        minDfaTab = new QWidget();
        minDfaTab->setObjectName("minDfaTab");
        verticalLayout_4 = new QVBoxLayout(minDfaTab);
        verticalLayout_4->setObjectName("verticalLayout_4");
        minDfaTable = new QTableWidget(minDfaTab);
        minDfaTable->setObjectName("minDfaTable");

        verticalLayout_4->addWidget(minDfaTable);

        tabWidget->addTab(minDfaTab, QString());
        lexTab = new QWidget();
        lexTab->setObjectName("lexTab");
        verticalLayout_6 = new QVBoxLayout(lexTab);
        verticalLayout_6->setObjectName("verticalLayout_6");
        label_3 = new QLabel(lexTab);
        label_3->setObjectName("label_3");

        verticalLayout_6->addWidget(label_3);

        generatedCodeEdit = new QPlainTextEdit(lexTab);
        generatedCodeEdit->setObjectName("generatedCodeEdit");
        generatedCodeEdit->setReadOnly(true);

        verticalLayout_6->addWidget(generatedCodeEdit);

        tabWidget->addTab(lexTab, QString());
        splitter->addWidget(tabWidget);

        verticalLayout->addWidget(splitter);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1000, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\346\255\243\345\210\231\350\241\250\350\276\276\345\274\217\350\275\254\346\215\242", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\346\255\243\350\247\204\345\274\217\345\256\232\344\271\211:", nullptr));
        loadButton->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275\346\226\207\344\273\266", nullptr));
        convertButton->setText(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213\350\275\254\346\215\242", nullptr));
        regexInput->setPlaceholderText(QCoreApplication::translate("MainWindow", "\344\276\213\345\246\202: digit=[0-9]\\nnum_100=(\\+|-)digit+", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(nfaTab), QCoreApplication::translate("MainWindow", "NFA \350\275\254\346\215\242\350\241\250", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(dfaTab), QCoreApplication::translate("MainWindow", "DFA \350\275\254\346\215\242\350\241\250", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(minDfaTab), QCoreApplication::translate("MainWindow", "\346\234\200\345\260\217\345\214\226 DFA \350\241\250", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "\347\224\237\346\210\220\347\232\204\350\257\215\346\263\225\345\210\206\346\236\220\347\250\213\345\272\217 (C++):", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(lexTab), QCoreApplication::translate("MainWindow", "\350\257\215\346\263\225\345\210\206\346\236\220 \346\272\220\347\240\201\346\237\245\347\234\213", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
