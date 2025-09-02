#include "loginWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <map>
#include <iostream>
#include <QMainWindow>
#include <QScreen>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int width = screenGeometry.width();
        int height = screenGeometry.height();

        // Set the window size to fit the screen
        w.resize(width, height);
    }

    w.show();
    return a.exec();

}
