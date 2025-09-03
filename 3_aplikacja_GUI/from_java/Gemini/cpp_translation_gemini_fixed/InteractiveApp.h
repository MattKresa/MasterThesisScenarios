// InteractiveApp.h
#ifndef INTERACTIVEAPP_H
#define INTERACTIVEAPP_H

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>

// Include nlohmann/json for JSON parsing.
// You'll need to download this header-only library and place it in your project or include path.
// Get it from: https://github.com/nlohmann/json
#include "json.hpp" // Assuming json.hpp is in the same directory or accessible via include path

// Use a namespace alias for convenience
using json = nlohmann::json;

class InteractiveApp : public QMainWindow {
    Q_OBJECT // Required for Qt's meta-object system (signals and slots)

public:
    explicit InteractiveApp(QWidget* parent = nullptr); // explicit to prevent unintended conversions

private slots:
    void showMessage();

private:
    const QString SAVE_FILE = "user_settings.json";

    QLineEdit* nameInput;
    QComboBox* colorDropdown;
    QCheckBox* emojiCheck;
    QSlider* fontSlider;
    QLabel* resultLabel;

    void saveSettings(const QString& name, const QString& color, bool emoji, int fontSize);
    void loadSettings();
};

#endif // INTERACTIVEAPP_H