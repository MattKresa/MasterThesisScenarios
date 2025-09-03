#include "InteractiveApp.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

InteractiveApp::InteractiveApp(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Interactive App");
    setGeometry(300, 300, 400, 300);

    name_label = new QLabel("Enter your name:");
    name_input = new QLineEdit();

    color_label = new QLabel("Choose a color:");
    color_dropdown = new QComboBox();
    color_dropdown->addItems({ "Black", "Blue", "Green", "Red", "Purple" });

    emoji_check = new QCheckBox("Add an emoji 😊");

    font_label = new QLabel("Select font size:");
    font_slider = new QSlider(Qt::Horizontal);
    font_slider->setRange(10, 30);
    font_slider->setValue(12);

    button = new QPushButton("Show Message");
    result_label = new QLabel("");
    result_label->setAlignment(Qt::AlignCenter);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(name_label);
    layout->addWidget(name_input);
    layout->addWidget(color_label);
    layout->addWidget(color_dropdown);
    layout->addWidget(emoji_check);
    layout->addWidget(font_label);
    layout->addWidget(font_slider);
    layout->addWidget(button);
    layout->addWidget(result_label);
    setLayout(layout);

    connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);

    loadSettings();
}

void InteractiveApp::showMessage() {
    QString name = name_input->text().trimmed();
    QString color = color_dropdown->currentText();
    bool emoji = emoji_check->isChecked();
    int font_size = font_slider->value();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter your name!");
        return;
    }

    QString message = QString("Hello, %1! Your favorite color is %2.").arg(name, color);
    if (emoji) {
        message += " 😊";
    }

    result_label->setText(message);
    result_label->setStyleSheet(QString("color: %1; font-size: %2px;")
        .arg(color.toLower()).arg(font_size));

    saveSettings(name, color, emoji, font_size);
}

void InteractiveApp::saveSettings(const QString& name, const QString& color, bool emoji, int font_size) {
    QJsonObject obj;
    obj["name"] = name;
    obj["color"] = color;
    obj["emoji"] = emoji;
    obj["font_size"] = font_size;

    QFile file(SAVE_FILE);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
}

void InteractiveApp::loadSettings() {
    QFile file(SAVE_FILE);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            name_input->setText(obj.value("name").toString());
            color_dropdown->setCurrentText(obj.value("color").toString("Black"));
            emoji_check->setChecked(obj.value("emoji").toBool(false));
            font_slider->setValue(obj.value("font_size").toInt(12));
        }
    }
}
