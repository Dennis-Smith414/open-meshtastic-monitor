#include "mainapp.h"
#include "ui_MainApp.h"

MainApp::MainApp(QWidget *parent)
    : QMainWindow{parent}
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Mesh Monitor - Main App");
}

MainApp::~MainApp()
{
    delete ui;
}
