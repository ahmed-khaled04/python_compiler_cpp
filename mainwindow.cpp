#include "mainwindow.h"
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), lexer(new Lexer(this))
{
    setWindowTitle("Python Lexical Analyzer");
    resize(1000, 700);
    setupUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // Central widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Splitter for code and results
    QSplitter *splitter = new QSplitter(Qt::Vertical, centralWidget);

    // Code editor
    QWidget *codeWidget = new QWidget(splitter);
    QVBoxLayout *codeLayout = new QVBoxLayout(codeWidget);

    QLabel *codeLabel = new QLabel("Python Code:", codeWidget);
    codeEdit = new QTextEdit(codeWidget);
    codeEdit->setFont(QFont("Consolas", 10));
    codeLayout->addWidget(codeLabel);
    codeLayout->addWidget(codeEdit);

    // Results tabs
    QTabWidget *resultsTab = new QTabWidget(splitter);

    // Tokens tab
    QWidget *tokenTab = new QWidget(resultsTab);
    QVBoxLayout *tokenLayout = new QVBoxLayout(tokenTab);
    tokenTable = new QTableWidget(tokenTab);
    tokenTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tokenTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    tokenTable->setColumnCount(3);
    tokenTable->setHorizontalHeaderLabels({"Token Type", "Value", "Line"});
    tokenTable->horizontalHeader()->setStretchLastSection(true);
    tokenLayout->addWidget(tokenTable);
    resultsTab->addTab(tokenTab, "Tokens");

    // Symbol table tab
    QWidget *symbolTab = new QWidget(resultsTab);
    QVBoxLayout *symbolLayout = new QVBoxLayout(symbolTab);
    symbolTable = new QTableWidget(symbolTab);
    symbolTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    symbolTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    symbolTable->setColumnCount(5);
    symbolTable->setHorizontalHeaderLabels({"ID", "Identifier", "Type", "Value", "Lines"});
    symbolTable->horizontalHeader()->setStretchLastSection(true);
    symbolLayout->addWidget(symbolTable);
    resultsTab->addTab(symbolTab, "Symbol Table");

    splitter->addWidget(codeWidget);
    splitter->addWidget(resultsTab);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    // Button panel
    QWidget *buttonWidget = new QWidget(centralWidget);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonWidget);

    QPushButton *analyzeButton = new QPushButton("Analyze", buttonWidget);
    QPushButton *openButton = new QPushButton("Open File", buttonWidget);
    QPushButton *clearButton = new QPushButton("Clear", buttonWidget);

    buttonLayout->addWidget(analyzeButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();

    // Connect signals
    connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::analyzeCode);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearAll);

    // Add to main layout
    mainLayout->addWidget(splitter);
    mainLayout->addWidget(buttonWidget);

    setCentralWidget(centralWidget);
}

void MainWindow::analyzeCode()
{
    QString code = codeEdit->toPlainText();
    if (code.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter some Python code first.");
        return;
    }

    std::vector<Token> tokens = lexer->tokenize(code);
    auto symbolTable = lexer->generateSymbolTable(tokens);

    displayTokens(tokens);
    displaySymbolTable(symbolTable);
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Python File", "", "Python Files (*.py);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            codeEdit->setPlainText(in.readAll());
            file.close();
        } else {
            QMessageBox::critical(this, "Error", "Could not open file.");
        }
    }
}

void MainWindow::clearAll()
{
    codeEdit->clear();
    tokenTable->clearContents();
    tokenTable->setRowCount(0);
    symbolTable->clearContents();
    symbolTable->setRowCount(0);
}

void MainWindow::displayTokens(const std::vector<Token>& tokens)
{
    tokenTable->clearContents();
    tokenTable->setRowCount(tokens.size());

    for (int i = 0; i < tokens.size(); ++i) {
        const Token& token = tokens[i];

        QTableWidgetItem *typeItem = new QTableWidgetItem(token.type);
        QTableWidgetItem *valueItem = new QTableWidgetItem(token.value);
        QTableWidgetItem *lineItem = new QTableWidgetItem(QString::number(token.line));

        tokenTable->setItem(i, 0, typeItem);
        tokenTable->setItem(i, 1, valueItem);
        tokenTable->setItem(i, 2, lineItem);
    }

    tokenTable->resizeColumnsToContents();
}

void MainWindow::displaySymbolTable(const std::unordered_map<QString, SymbolEntry>& symbolTable)
{
    this->symbolTable->clearContents();
    this->symbolTable->setRowCount(symbolTable.size());

    int row = 0;
    for (const auto& entry : symbolTable) {
        QString linesStr;
        for (size_t i = 0; i < entry.second.lines.size(); ++i) {
            if (i > 0) linesStr += ", ";
            linesStr += QString::number(entry.second.lines[i]);
        }

        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(entry.second.id));
        QTableWidgetItem *nameItem = new QTableWidgetItem(entry.first);
        QTableWidgetItem *typeItem = new QTableWidgetItem(entry.second.type);
        QTableWidgetItem *valueItem = new QTableWidgetItem(entry.second.value);
        QTableWidgetItem *linesItem = new QTableWidgetItem(linesStr);

        this->symbolTable->setItem(row, 0, idItem);
        this->symbolTable->setItem(row, 1, nameItem);
        this->symbolTable->setItem(row, 2, typeItem);
        this->symbolTable->setItem(row, 3, valueItem);
        this->symbolTable->setItem(row, 4, linesItem);

        ++row;
    }

    this->symbolTable->resizeColumnsToContents();
}
