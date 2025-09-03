#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDir>

const QString SAVE_FILE = "user_settings.json";

class InteractiveApp : public QWidget {
    Q_OBJECT

public:
    InteractiveApp(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Interactive App");
        resize(400, 300);

        // Widgets
        name_label = new QLabel("Enter your name:");
        name_input = new QLineEdit();

        color_label = new QLabel("Choose a color:");
        color_dropdown = new QComboBox();
        color_dropdown->addItems({"Black", "Blue", "Green", "Red", "Purple"});

        emoji_check = new QCheckBox("Add an emoji 😊");

        font_label = new QLabel("Select font size:");
        font_slider = new QSlider(Qt::Horizontal);
        font_slider->setRange(10, 30);
        font_slider->setValue(12);

        button = new QPushButton("Show Message");
        connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);

        result_label = new QLabel("");
        result_label->setAlignment(Qt::AlignCenter);

        // Layout
        QVBoxLayout *layout = new QVBoxLayout();
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

        loadSettings();
    }

private slots:
    void showMessage() {
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
        result_label->setStyleSheet(QString("color: %1; font-size: %2px;").arg(color.toLower()).arg(font_size));

        saveSettings(name, color, emoji, font_size);
    }

private:
    void saveSettings(const QString &name, const QString &color, bool emoji, int font_size) {
        QJsonObject data;
        data["name"] = name;
        data["color"] = color;
        data["emoji"] = emoji;
        data["font_size"] = font_size;

        QFile file(SAVE_FILE);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(data).toJson());
            file.close();
        }
    }

    void loadSettings() {
        QFile file(SAVE_FILE);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            QByteArray jsonData = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            QJsonObject data = doc.object();

            name_input->setText(data.value("name").toString(""));
            color_dropdown->setCurrentText(data.value("color").toString("Black"));
            emoji_check->setChecked(data.value("emoji").toBool(false));
            font_slider->setValue(data.value("font_size").toInt(12));
        }
    }

    // Widget declarations
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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    InteractiveApp window;
    window.show();

    return app.exec();
}

#include "main.moc"