#include "loginWindow.h"
#include "./ui_loginWindow.h"
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPaintEvent>


//Constuctor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Login)
    , mainApp(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("Login - Mesh Monitor");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QMap<QString, QString> dict = { {"dennis", "1923"} };

    QString username = ui->userName->text();
    QString password = ui->Password->text();

    if(dict.contains(username) && dict[username] == password) {
        mainApp = new MainApp();
        mainApp->show();
        this->hide();


    } else {
        QMessageBox::warning(this,"Login Failed!", "Bad Credentials!");
    }

}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

    QPainter painter(this);
    QPixmap background(":/images/topo-map1.jpeg");

    QPixmap scaledBackground = background.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Paint it
    painter.drawPixmap(0, 0, scaledBackground);
}



