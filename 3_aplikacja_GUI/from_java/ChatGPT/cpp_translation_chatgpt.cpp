#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QMessageBox>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QWidget>
#include <QPalette>
#include <QJsonDocument>
#include <QJsonObject>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class InteractiveApp : public QWidget {
    Q_OBJECT

public:
    InteractiveApp(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Interactive App");
        setFixedSize(400, 300);

        // Layout
        QVBoxLayout *layout = new QVBoxLayout(this);

        // Name input
        QLabel *nameLabel = new QLabel("Enter your name:");
        nameInput = new QLineEdit();
        layout->addWidget(nameLabel);
        layout->addWidget(nameInput);

        // Color dropdown
        QLabel *colorLabel = new QLabel("Choose a color:");
        colorDropdown = new QComboBox();
        colorDropdown->addItems({"Black", "Blue", "Green", "Red", "Purple"});
        colorDropdown->setCurrentText("Black");
        layout->addWidget(colorLabel);
        layout->addWidget(colorDropdown);

        // Emoji checkbox
        emojiCheck = new QCheckBox("Add an emoji 😊");
        layout->addWidget(emojiCheck);

        // Font size slider
        QLabel *fontLabel = new QLabel("Select font size:");
        fontSlider = new QSlider(Qt::Horizontal);
        fontSlider->setRange(10, 30);
        fontSlider->setValue(12);
        layout->addWidget(fontLabel);
        layout->addWidget(fontSlider);

        // Show message button
        QPushButton *button = new QPushButton("Show Message");
        layout->addWidget(button);

        // Result label
        resultLabel = new QLabel("");
        resultLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(resultLabel);

        connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);

        loadSettings();
    }

private slots:
    void showMessage() {
        QString name = nameInput->text().trimmed();
        QString color = colorDropdown->currentText();
        bool emoji = emojiCheck->isChecked();
        int fontSize = fontSlider->value();

        if (name.isEmpty()) {
            QMessageBox::warning(this, "Warning", "Please enter your name!");
            return;
        }

        QString message = "Hello, " + name + "! Your favorite color is " + color + ".";
        if (emoji) {
            message += " 😊";
        }

        resultLabel->setText(message);

        QPalette palette = resultLabel->palette();
        palette.setColor(QPalette::WindowText, QColor(color.toLower()));
        resultLabel->setPalette(palette);

        QFont font = resultLabel->font();
        font.setPointSize(fontSize);
        resultLabel->setFont(font);

        saveSettings(name, color, emoji, fontSize);
    }

private:
    void saveSettings(const QString &name, const QString &color, bool emoji, int fontSize) {
        json data;
        data["name"] = name.toStdString();
        data["color"] = color.toStdString();
        data["emoji"] = emoji;
        data["font_size"] = fontSize;

        std::ofstream file("user_settings.json");
        if (file.is_open()) {
            file << data.dump(4);
        }
    }

    void loadSettings() {
        std::ifstream file("user_settings.json");
        if (!file.is_open()) return;

        json data;
        try {
            file >> data;
        } catch (...) {
            return;
        }

        nameInput->setText(QString::fromStdString(data.value("name", "")));
        colorDropdown->setCurrentText(QString::fromStdString(data.value("color", "Black")));
        emojiCheck->setChecked(data.value("emoji", false));
        fontSlider->setValue(data.value("font_size", 12));
    }

    QLineEdit *nameInput;
    QComboBox *colorDropdown;
    QCheckBox *emojiCheck;
    QSlider *fontSlider;
    QLabel *resultLabel;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    InteractiveApp window;
    window.show();
    return app.exec();
}

#include "main.moc"
