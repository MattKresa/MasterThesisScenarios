#include "InteractiveApp.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    InteractiveApp window;
    window.show();

    return app.exec();
}