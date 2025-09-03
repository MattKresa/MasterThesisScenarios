#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

class InteractiveApp : public QWidget
{
    Q_OBJECT

public:
    explicit InteractiveApp(QWidget* parent = nullptr);
    ~InteractiveApp() = default;

private slots:
    void showMessage();

private:
    void setupUI();
    void saveSettings();
    void loadSettings();

    // UI elements
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
};