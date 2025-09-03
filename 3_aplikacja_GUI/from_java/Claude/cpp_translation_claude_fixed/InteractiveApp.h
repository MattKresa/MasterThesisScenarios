
// InteractiveApp.h
#ifndef INTERACTIVEAPP_H
#define INTERACTIVEAPP_H

#include <QtWidgets/QMainWindow>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSlider;
class QLabel;

class InteractiveApp : public QMainWindow
{
    Q_OBJECT

private:
    static const QString SAVE_FILE;

    QLineEdit* nameInput;
    QComboBox* colorDropdown;
    QCheckBox* emojiCheck;
    QSlider* fontSlider;
    QLabel* resultLabel;
    QLabel* fontSizeLabel;

public:
    explicit InteractiveApp(QWidget* parent = nullptr);

private slots:
    void showMessage();
    void updateFontSizeLabel();

private:
    void setupUI();
    void saveSettings(const QString& name, const QString& color, bool emoji, int fontSize);
    void loadSettings();
};

#endif // INTERACTIVEAPP_H