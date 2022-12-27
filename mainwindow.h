#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QThread>
#include <QColor>

#include "blockchain.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();

    void on_mineButton_clicked();

    void on_connectPortButton_clicked();

    void onBlockMined(const Block &block, QColor color);

    void onMessageSent(QString message, QColor color);

    void changeStatus(QString message);

    void onUpdateAverage(qint64 milis);

    void onUpdate10Average(qint64 milis);

    void onDifficultyChnaged(qint64 diff);

private:

    void printLedger(QByteArray message, QColor color = QColor(Qt::black));

    void printHash(QByteArray hash, QColor color = QColor(Qt::black));

private:
    Ui::MainWindow *ui;
    Blockchain blockchain;
};
#endif // MAINWINDOW_H
