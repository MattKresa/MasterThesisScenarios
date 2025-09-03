#ifndef INTERACTIVE_APP_H
#define INTERACTIVE_APP_H

#include <QWidget>
#include <QString> // Include QString for string manipulation

// Forward declarations to avoid including full headers in the .h file
// This is good practice for reducing compilation dependencies.
class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QSlider;
class QPushButton;
class QVBoxLayout; // Although QVBoxLayout is used in the .cpp, it's good to be consistent
// and forward declare if you were to declare it as a member.
// For now, it's a local variable in the constructor, so not strictly needed here.

/**
 * @brief The InteractiveApp class represents the main window of the application.
 *
 * This class inherits from QWidget and sets up all the UI elements,
 * handles user interactions, and manages saving/loading settings.
 */
class InteractiveApp : public QWidget
{
    Q_OBJECT // Required for Qt's signal/slot mechanism

public:
    /**
     * @brief Constructor for the InteractiveApp.
     * @param parent The parent widget (defaults to nullptr).
     *
     * Initializes the UI, sets up the layout, connects signals and slots,
     * and loads any previously saved settings.
     */
    explicit InteractiveApp(QWidget* parent = nullptr);

private slots:
    /**
     * @brief Slot to display the message based on user input.
     *
     * Gathers data from UI widgets, constructs a message, applies styling,
     * and saves the current settings.
     */
    void showMessage();

private:
    // --- UI Widgets ---
    QLabel* nameLabel;
    QLineEdit* nameInput;
    QLabel* colorLabel;
    QComboBox* colorDropdown;
    QCheckBox* emojiCheck;
    QLabel* fontLabel;
    QSlider* fontSlider;
    QPushButton* button;
    QLabel* resultLabel;

    /**
     * @brief Saves the current user settings to a JSON file.
     * @param name The user's name.
     * @param color The chosen color.
     * @param emoji Whether emoji is enabled.
     * @param fontSize The selected font size.
     */
    void saveSettings(const QString& name, const QString& color, bool emoji, int fontSize);

    /**
     * @brief Loads user settings from a JSON file.
     *
     * If the save file exists, it reads the settings and applies them
     * to the respective UI widgets. Provides default values if keys are missing.
     */
    void loadSettings();
};

#endif // INTERACTIVE_APP_H
