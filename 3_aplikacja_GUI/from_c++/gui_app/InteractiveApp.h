#ifndef INTERACTIVEAPP_H
#define INTERACTIVEAPP_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>

class InteractiveApp : public QWidget {
    Q_OBJECT

public:
    explicit InteractiveApp(QWidget* parent = nullptr);

private slots:
    void showMessage();

private:
    QLabel* name_label;
    QLineEdit* name_input;
    QLabel* color_label;
    QComboBox* color_dropdown;
    QCheckBox* emoji_check;
    QLabel* font_label;
    QSlider* font_slider;
    QPushButton* button;
    QLabel* result_label;

    const QString SAVE_FILE = "user_settings.json";

    void saveSettings(const QString& name, const QString& color, bool emoji, int font_size);
    void loadSettings();
};

#endif // INTERACTIVEAPP_H
