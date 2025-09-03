#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSlider>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtGui/QFont>
#include <QtGui/QPalette>

class InteractiveApp : public QMainWindow
{
    Q_OBJECT

private:
    static const QString SAVE_FILE;
    
    QLineEdit* nameInput;
    QComboBox* colorDropdown;
    QCheckBox* emojiCheck;
    QSlider* fontSlider;
    QLabel* resultLabel;
    QLabel* fontSizeLabel;

public:
    InteractiveApp(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("Interactive App");
        setFixedSize(400, 350);
        
        setupUI();
        loadSettings();
    }

private slots:
    void showMessage()
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
        
        // Set text color
        QPalette palette = resultLabel->palette();
        QColor textColor;
        if (color == "Black") textColor = Qt::black;
        else if (color == "Blue") textColor = Qt::blue;
        else if (color == "Green") textColor = Qt::green;
        else if (color == "Red") textColor = Qt::red;
        else if (color == "Purple") textColor = QColor(128, 0, 128); // Purple
        
        palette.setColor(QPalette::WindowText, textColor);
        resultLabel->setPalette(palette);
        
        // Set font size
        QFont font = resultLabel->font();
        font.setPointSize(fontSize);
        resultLabel->setFont(font);

        saveSettings(name, color, emoji, fontSize);
    }

    void updateFontSizeLabel()
    {
        fontSizeLabel->setText(QString::number(fontSlider->value()));
    }

private:
    void setupUI()
    {
        QWidget* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        layout->setSpacing(10);
        layout->setContentsMargins(20, 20, 20, 20);

        // Name input
        QLabel* nameLabel = new QLabel("Enter your name:");
        layout->addWidget(nameLabel);
        
        nameInput = new QLineEdit();
        layout->addWidget(nameInput);

        // Color dropdown
        QLabel* colorLabel = new QLabel("Choose a color:");
        layout->addWidget(colorLabel);
        
        colorDropdown = new QComboBox();
        colorDropdown->addItems({"Black", "Blue", "Green", "Red", "Purple"});
        colorDropdown->setCurrentText("Black");
        layout->addWidget(colorDropdown);

        // Emoji checkbox
        emojiCheck = new QCheckBox("Add an emoji 😊");
        layout->addWidget(emojiCheck);

        // Font size slider
        QLabel* fontLabel = new QLabel("Select font size:");
        layout->addWidget(fontLabel);
        
        fontSlider = new QSlider(Qt::Horizontal);
        fontSlider->setMinimum(10);
        fontSlider->setMaximum(30);
        fontSlider->setValue(12);
        fontSlider->setTickPosition(QSlider::TicksBelow);
        fontSlider->setTickInterval(5);
        layout->addWidget(fontSlider);
        
        // Font size display
        fontSizeLabel = new QLabel("12");
        fontSizeLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(fontSizeLabel);
        
        // Connect slider to update label
        connect(fontSlider, &QSlider::valueChanged, this, &InteractiveApp::updateFontSizeLabel);

        // Show message button
        QPushButton* button = new QPushButton("Show Message");
        connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);
        layout->addWidget(button);

        // Result label
        resultLabel = new QLabel("");
        resultLabel->setAlignment(Qt::AlignCenter);
        resultLabel->setWordWrap(true);
        resultLabel->setMinimumHeight(60);
        layout->addWidget(resultLabel);
        
        // Add stretch to center content
        layout->addStretch();
    }

    void saveSettings(const QString& name, const QString& color, bool emoji, int fontSize)
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
    }

    void loadSettings()
    {
        QFile file(SAVE_FILE);
        if (!file.exists()) {
            return;
        }

        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            
            nameInput->setText(obj.value("name").toString(""));
            
            QString color = obj.value("color").toString("Black");
            int colorIndex = colorDropdown->findText(color);
            if (colorIndex >= 0) {
                colorDropdown->setCurrentIndex(colorIndex);
            }
            
            emojiCheck->setChecked(obj.value("emoji").toBool(false));
            fontSlider->setValue(obj.value("font_size").toInt(12));
            updateFontSizeLabel();
        }
    }
};

const QString InteractiveApp::SAVE_FILE = "user_settings.json";

// Main function
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    InteractiveApp window;
    window.show();

    return app.exec();
}

// Include the MOC file for Qt's meta-object system
#include "main.moc"