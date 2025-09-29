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
    , userDatabase(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("Login - Mesh Monitor");

    userDatabase = new userdatabase(this);
    if (!userDatabase->initDatabase()) {
        QMessageBox::critical(this, "Database Error",
                              "Failed to initialize user database!");
        return;
    }

    if (userDatabase->getAllUsers().isEmpty()) {
        qDebug() << "No users found. Creating default users...";

        // Add an admin user
        userDatabase->addUser("admin", "admin");

        QMessageBox::information(this, "First Run",
                                 "Default users created:\n"
                                 "admin / admin\n\n"
                                 "Database initialized successfully!");
    }

   // qDebug() << "Database ready with" << userDatabase->getAllUsers().size() << "users";

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QString username = ui->userName->text();
    QString password = ui->Password->text();

    if (userDatabase && userDatabase->authenticateUser(username, password)) {
        qDebug() << "Login successful for user:" << username;
        mainApp = new MainApp();
        mainApp->show();
        this->hide();
    } else {
        qDebug() << "Login failed for user:" << username;
        QMessageBox::warning(this, "Login Failed!", "Invalid username or password!");

        // Clear the password field for security
        ui->Password->clear();
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



