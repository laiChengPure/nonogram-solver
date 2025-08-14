#include <QApplication>

#include "nonogramboard.h"
#include "outlook\mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}