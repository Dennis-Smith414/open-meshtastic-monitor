#ifndef MAINAPP_H
#define MAINAPP_H

#include <QMainWindow>
#include <QPaintEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainApp : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainApp(QWidget *parent = nullptr);
    ~MainApp();

private:
    Ui::MainWindow *ui;
    void paintEvent(QPaintEvent *event) override;

signals:

};

#endif // MAINAPP_H
