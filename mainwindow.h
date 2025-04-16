#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include "lexer.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void analyzeCode();
    void openFile();
    void clearAll();

private:
    void setupUI();
    void displayTokens(const std::vector<Token>& tokens);
    void displaySymbolTable(const std::unordered_map<QString, SymbolEntry>& symbolTable);

    QTextEdit *codeEdit;
    QTableWidget *tokenTable;
    QTableWidget *symbolTable;
    Lexer *lexer;
};

#endif // MAINWINDOW_H
