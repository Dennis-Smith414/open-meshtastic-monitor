#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QPaintEvent>
#include "mainapp.h"
#include "userdatabase.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Login; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

//    void on_userName_returnPressed();

private:
    Ui::Login *ui;
    void paintEvent(QPaintEvent *event) override;
    MainApp *mainApp;
    userdatabase *userDatabase;
};
#endif // LOGINWINDOW_H
