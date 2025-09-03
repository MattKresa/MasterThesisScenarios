#ifndef INTERACTIVEAPP_H
#define INTERACTIVEAPP_H

#include <QMainWindow>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>

class InteractiveApp : public QMainWindow {
    Q_OBJECT

public:
    explicit InteractiveApp(QWidget* parent = nullptr);

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