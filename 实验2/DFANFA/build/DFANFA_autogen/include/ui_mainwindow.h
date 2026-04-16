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
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "graph_widget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
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
    QWidget *graphTab;
    QVBoxLayout *verticalLayout_5;
    GraphWidget *graphWidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1000, 700);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(centralwidget);
        label->setObjectName("label");

        horizontalLayout->addWidget(label);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        loadButton = new QPushButton(centralwidget);
        loadButton->setObjectName("loadButton");

        horizontalLayout->addWidget(loadButton);

        convertButton = new QPushButton(centralwidget);
        convertButton->setObjectName("convertButton");

        horizontalLayout->addWidget(convertButton);


        verticalLayout->addLayout(horizontalLayout);

        regexInput = new QPlainTextEdit(centralwidget);
        regexInput->setObjectName("regexInput");

        verticalLayout->addWidget(regexInput);

        tabWidget = new QTabWidget(centralwidget);
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
        graphTab = new QWidget();
        graphTab->setObjectName("graphTab");
        verticalLayout_5 = new QVBoxLayout(graphTab);
        verticalLayout_5->setObjectName("verticalLayout_5");
        graphWidget = new GraphWidget(graphTab);
        graphWidget->setObjectName("graphWidget");

        verticalLayout_5->addWidget(graphWidget);

        tabWidget->addTab(graphTab, QString());

        verticalLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\346\255\243\345\210\231\350\241\250\350\276\276\345\274\217 DFA \350\275\254\346\215\242\345\256\236\351\252\214", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\346\255\243\345\210\231\350\241\250\350\276\276\345\274\217 (\346\240\274\345\274\217: name=regex \346\210\226\347\233\264\346\216\245\350\276\223\345\205\245 regex):", nullptr));
        loadButton->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275\346\226\207\344\273\266", nullptr));
        convertButton->setText(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213\350\275\254\346\215\242", nullptr));
        regexInput->setPlaceholderText(QCoreApplication::translate("MainWindow", "\344\276\213\345\246\202: digit=[0-9]\\nnum_100=(\\+|-)digit+", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(nfaTab), QCoreApplication::translate("MainWindow", "NFA \350\275\254\346\215\242\350\241\250", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(dfaTab), QCoreApplication::translate("MainWindow", "DFA \350\275\254\346\215\242\350\241\250", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(minDfaTab), QCoreApplication::translate("MainWindow", "\346\234\200\345\260\217\345\214\226 DFA \350\241\250", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(graphTab), QCoreApplication::translate("MainWindow", "\345\233\276\345\275\242\345\261\225\347\244\272", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
