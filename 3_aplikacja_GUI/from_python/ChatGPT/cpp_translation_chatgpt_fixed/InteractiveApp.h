#ifndef INTERACTIVEAPP_H
#define INTERACTIVEAPP_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>

class InteractiveApp : public QWidget {
    Q_OBJECT

public:
    explicit InteractiveApp(QWidget *parent = nullptr);

private slots:
    void show_message();
    void save_settings(const QString &name, const QString &color, bool emoji, int font_size);
    void load_settings();

private:
    QLabel *name_label;
    QLineEdit *name_input;
    QLabel *color_label;
    QComboBox *color_dropdown;
    QCheckBox *emoji_check;
    QLabel *font_label;
    QSlider *font_slider;
    QPushButton *button;
    QLabel *result_label;
};

#endif // INTERACTIVEAPP_H
