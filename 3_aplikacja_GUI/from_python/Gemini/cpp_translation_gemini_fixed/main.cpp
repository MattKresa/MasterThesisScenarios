#include <QApplication>
#include "InteractiveApp.h" // Include the header for our application class

/**
 * @brief The main entry point of the application.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return The application's exit code.
 */
int main(int argc, char* argv[])
{
    QApplication app(argc, argv); // Create the QApplication instance

    InteractiveApp window; // Create an instance of our main window
    window.show();         // Show the window

    return app.exec();     // Start the Qt event loop
}
