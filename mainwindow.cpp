#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , blockchain(Blockchain(this))
{
    ui->setupUi(this);
    connect(&blockchain, SIGNAL(blockMined(const Block &, QColor)), this, SLOT(onBlockMined(const Block &, QColor)));
    connect(&blockchain, SIGNAL(messageSent(QString, QColor)), this, SLOT(onMessageSent(QString, QColor)));
    connect(&blockchain, SIGNAL(updateAverage(qint64)), this, SLOT(onUpdateAverage(qint64)));
    connect(&blockchain, SIGNAL(update10Average(qint64)), this, SLOT(onUpdate10Average(qint64)));
    connect(&blockchain, SIGNAL(difficultyChanged(qint64)), this, SLOT(onDifficultyChnaged(qint64)));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_connectButton_clicked()
{
    auto port = blockchain.startServer();
    if (port == 0) {
        printLedger("Server could not start on port " + QByteArray::number(port));
    } else if (port == -1) {
        return;
    } else {
        printLedger("Server started on port " + QByteArray::number(port));
        changeStatus("Online 127.0.0.1:" + QString::number(port));
    }
}


void MainWindow::on_mineButton_clicked()
{
    auto thread = QThread::create([this](){
        blockchain.startMining();
    });
    thread->start();
}


void MainWindow::on_connectPortButton_clicked()
{
    bool ok;
    auto port = ui->connectPortLineEdit->text().toInt(&ok, 10);
    if (!ok) {
        QMessageBox msgBox;
        msgBox.setText("Error when converting port to number!");
        msgBox.exec();
        return;
    }
    auto res = blockchain.connectToPort(port);
    if (!res) {
        QMessageBox msgBox;
        msgBox.setText("Error when connecting to port: " + QString::number(port));
        msgBox.exec();
        return;
    }
}


void MainWindow::onBlockMined(const Block &block, QColor color)
{
    printLedger(block.toQString().toUtf8(), color);
}


void MainWindow::onMessageSent(QString message, QColor color)
{
    printLedger(message.toUtf8(), color);
}

void MainWindow::changeStatus(QString message)
{
    ui->statusInfoLabel->setText(message);
}

void MainWindow::onUpdateAverage(qint64 milis)
{
    ui->avgBlockTimeInfoLabel->setText(QString::number(milis) + " ms");
}

void MainWindow::onUpdate10Average(qint64 milis)
{
    ui->avg10BlockInfoLabel->setText(QString::number(milis) + " ms");
}

void MainWindow::onDifficultyChnaged(qint64 diff)
{
    ui->diffInfoLabel->setText(QString::number(diff));
}


void MainWindow::printLedger(QByteArray message, QColor color)
{
    auto item = new QListWidgetItem();
    item->setText(message);
    item->setForeground(color);
    ui->ledgerList->addItem(item);
    ui->ledgerList->scrollToBottom();
}


void MainWindow::printHash(QByteArray hash, QColor color)
{
    auto item = new QListWidgetItem();
    item->setText(hash);
    item->setForeground(color);
    ui->hashesList->addItem(item);
    ui->hashesList->scrollToBottom();
}
