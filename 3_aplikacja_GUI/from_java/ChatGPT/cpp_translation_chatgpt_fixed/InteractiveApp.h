#ifndef INTERACTIVEAPP_H
#define INTERACTIVEAPP_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QString>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class InteractiveApp : public QWidget {
    Q_OBJECT

public:
    explicit InteractiveApp(QWidget *parent = nullptr);

private slots:
    void showMessage();

private:
    void saveSettings(const QString &name, const QString &color, bool emoji, int fontSize);
    void loadSettings();

    QLineEdit *nameInput;
    QComboBox *colorDropdown;
    QCheckBox *emojiCheck;
    QSlider *fontSlider;
    QLabel *resultLabel;
};

#endif // INTERACTIVEAPP_H
