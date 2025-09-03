// InteractiveApp.cpp
#include "InteractiveApp.h"
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSlider>
#include <QtWidgets/QMessageBox>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtCore/Qt>
#include <QDebug>

const QString SAVE_FILE = "user_settings.json";

InteractiveApp::InteractiveApp(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Interactive App");
    setGeometry(300, 300, 400, 300);

    // Create widgets
    nameLabel = new QLabel("Enter your name:");
    nameInput = new QLineEdit();

    colorLabel = new QLabel("Choose a color:");
    colorDropdown = new QComboBox();
    colorDropdown->addItems({ "Black", "Blue", "Green", "Red", "Purple" });

    emojiCheck = new QCheckBox("Add an emoji 😊");

    fontLabel = new QLabel("Select font size:");
    fontSlider = new QSlider(Qt::Horizontal);
    fontSlider->setRange(10, 30);
    fontSlider->setValue(12);

    button = new QPushButton("Show Message");
    connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);

    resultLabel = new QLabel("");
    resultLabel->setAlignment(Qt::AlignCenter);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(nameLabel);
    layout->addWidget(nameInput);
    layout->addWidget(colorLabel);
    layout->addWidget(colorDropdown);
    layout->addWidget(emojiCheck);
    layout->addWidget(fontLabel);
    layout->addWidget(fontSlider);
    layout->addWidget(button);
    layout->addWidget(resultLabel);
    setLayout(layout);

    loadSettings();
}

void InteractiveApp::showMessage()
{
    QString name = nameInput->text().trimmed();
    QString color = colorDropdown->currentText();
    bool emoji = emojiCheck->isChecked();
    int fontSize = fontSlider->value();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter your name!");
        return;
    }

    QString message = QString("Hello, %1! Your favorite color is %2.").arg(name, color);
    if (emoji) {
        message += " 😊";
    }

    resultLabel->setText(message);
    QString styleSheet = QString("color: %1; font-size: %2px;")
        .arg(color.toLower())
        .arg(fontSize);
    resultLabel->setStyleSheet(styleSheet);

    saveSettings(name, color, emoji, fontSize);
}

void InteractiveApp::saveSettings(const QString& name, const QString& color, bool emoji, int fontSize)
{
    QJsonObject data;
    data["name"] = name;
    data["color"] = color;
    data["emoji"] = emoji;
    data["font_size"] = fontSize;

    QJsonDocument doc(data);
    QFile file(SAVE_FILE);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
    else {
        qDebug() << "Failed to save settings to" << SAVE_FILE;
    }
}

void InteractiveApp::loadSettings()
{
    QFile file(SAVE_FILE);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();

        nameInput->setText(obj.value("name").toString(""));
        colorDropdown->setCurrentText(obj.value("color").toString("Black"));
        emojiCheck->setChecked(obj.value("emoji").toBool(false));
        fontSlider->setValue(obj.value("font_size").toInt(12));
    }
}

// ================================