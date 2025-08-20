#include "loginWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <map>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;



    w.show();
    return a.exec();

}
