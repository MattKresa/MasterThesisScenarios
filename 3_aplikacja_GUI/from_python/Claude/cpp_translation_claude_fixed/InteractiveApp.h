
// InteractiveApp.h
#ifndef INTERACTIVEAPP_H
#define INTERACTIVEAPP_H

#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QSlider;
class QPushButton;
QT_END_NAMESPACE

class InteractiveApp : public QWidget
{
    Q_OBJECT

public:
    InteractiveApp(QWidget* parent = nullptr);

private slots:
    void showMessage();

private:
    void saveSettings(const QString& name, const QString& color, bool emoji, int fontSize);
    void loadSettings();

    // UI Elements
    QLabel* nameLabel;
    QLineEdit* nameInput;
    QLabel* colorLabel;
    QComboBox* colorDropdown;
    QCheckBox* emojiCheck;
    QLabel* fontLabel;
    QSlider* fontSlider;
    QPushButton* button;
    QLabel* resultLabel;
};

#endif // INTERACTIVEAPP_H