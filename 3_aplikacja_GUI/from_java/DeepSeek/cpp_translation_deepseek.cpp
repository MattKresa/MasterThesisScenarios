#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

class InteractiveApp : public QMainWindow {
    Q_OBJECT

public:
    InteractiveApp(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Interactive App");
        
        // Create widgets
        QLabel *nameLabel = new QLabel("Enter your name:");
        nameInput = new QLineEdit();
        
        QLabel *colorLabel = new QLabel("Choose a color:");
        colorDropdown = new QComboBox();
        colorDropdown->addItems({"Black", "Blue", "Green", "Red", "Purple"});
        colorDropdown->setCurrentText("Black");
        
        emojiCheck = new QCheckBox("Add an emoji 😊");
        
        QLabel *fontLabel = new QLabel("Select font size:");
        fontSlider = new QSlider(Qt::Horizontal);
        fontSlider->setRange(10, 30);
        fontSlider->setValue(12);
        fontSlider->setTickPosition(QSlider::TicksBelow);
        fontSlider->setTickInterval(5);
        
        QPushButton *button = new QPushButton("Show Message");
        connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);
        
        resultLabel = new QLabel("");
        resultLabel->setAlignment(Qt::AlignCenter);
        
        // Layout
        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget(nameLabel);
        layout->addWidget(nameInput);
        layout->addWidget(colorLabel);
        layout->addWidget(colorDropdown);
        layout->addWidget(emojiCheck);
        layout->addWidget(fontLabel);
        layout->addWidget(fontSlider);
        layout->addWidget(button);
        layout->addWidget(resultLabel);
        
        QWidget *centralWidget = new QWidget();
        centralWidget->setLayout(layout);
        setCentralWidget(centralWidget);
        
        resize(400, 300);
        
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
        
        // Set color
        QColor textColor;
        if (color == "Black") textColor = Qt::black;
        else if (color == "Blue") textColor = Qt::blue;
        else if (color == "Green") textColor = Qt::green;
        else if (color == "Red") textColor = Qt::red;
        else if (color == "Purple") textColor = QColor(128, 0, 128); // Purple
        
        QPalette palette = resultLabel->palette();
        palette.setColor(QPalette::WindowText, textColor);
        resultLabel->setPalette(palette);
        
        // Set font size
        QFont font = resultLabel->font();
        font.setPointSize(fontSize);
        resultLabel->setFont(font);
        
        saveSettings(name, color, emoji, fontSize);
    }

private:
    const QString SAVE_FILE = "user_settings.json";
    QLineEdit *nameInput;
    QComboBox *colorDropdown;
    QCheckBox *emojiCheck;
    QSlider *fontSlider;
    QLabel *resultLabel;
    
    void saveSettings(const QString &name, const QString &color, bool emoji, int fontSize) {
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
    }
    
    void loadSettings() {
        QFile file(SAVE_FILE);
        if (!file.exists()) return;
        
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray jsonData = file.readAll();
            file.close();
            
            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            QJsonObject data = doc.object();
            
            nameInput->setText(data.value("name").toString(""));
            colorDropdown->setCurrentText(data.value("color").toString("Black"));
            emojiCheck->setChecked(data.value("emoji").toBool(false));
            fontSlider->setValue(data.value("font_size").toInt(12));
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    InteractiveApp window;
    window.show();
    
    return app.exec();
}

#include "main.moc"  // Needed for Qt's meta-object compiler