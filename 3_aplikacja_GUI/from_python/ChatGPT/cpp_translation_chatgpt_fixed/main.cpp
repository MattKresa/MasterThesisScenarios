#include <QApplication>
#include "InteractiveApp.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    InteractiveApp window;
    window.show();
    return app.exec();
}
