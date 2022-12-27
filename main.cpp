#include "mainwindow.h"

#include <QCryptographicHash>
#include <QDebug>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("Simple Blockchain");
    w.show();
    return a.exec();
}
