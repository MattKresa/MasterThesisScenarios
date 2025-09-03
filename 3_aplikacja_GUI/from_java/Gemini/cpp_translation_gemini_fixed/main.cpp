// main.cpp
#include <QApplication>
#include "InteractiveApp.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv); // Create the Qt application object
    InteractiveApp appWindow; // Instantiate your main window
    appWindow.show(); // Show the window
    return a.exec(); // Start the Qt event loop
}