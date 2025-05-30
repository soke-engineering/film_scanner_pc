#include "mainwindow.h"

#include <QApplication>

#include "knokke_v1.h"

int main(int argc, char *argv[])
{
    KnokkeV1 knokke;

    QApplication a(argc, argv);
    MainWindow   w;
    w.show();
    return a.exec();
}
