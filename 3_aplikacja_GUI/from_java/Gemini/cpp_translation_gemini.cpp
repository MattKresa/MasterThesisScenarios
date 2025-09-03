#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QMessageBox>
#include <QFont>
#include <QPalette>
#include <QFile>
#include <QTextStream>
#include <QDebug> // For qWarning

// Include nlohmann/json for JSON parsing.
// You'll need to download this header-only library and place it in your project or include path.
// Get it from: https://github.com/nlohmann/json
#include "json.hpp" // Assuming json.hpp is in the same directory or accessible via include path

// Use a namespace alias for convenience
using json = nlohmann::json;

class InteractiveApp : public QMainWindow {
    Q_OBJECT // Required for Qt's meta-object system (signals and slots)

public:
    InteractiveApp(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Interactive App");
        setFixedSize(400, 350); // Set fixed window size

        // Create a central widget and layout
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);
        layout->setSpacing(10); // Spacing between widgets
        layout->setAlignment(Qt::AlignCenter); // Center all widgets in the layout

        // Name Input
        QLabel *nameLabel = new QLabel("Enter your name:", this);
        nameInput = new QLineEdit(this);
        nameInput->setFixedWidth(200); // Set a fixed width for the input field

        layout->addWidget(nameLabel);
        layout->addWidget(nameInput);

        // Color Dropdown
        QLabel *colorLabel = new QLabel("Choose a color:", this);
        colorDropdown = new QComboBox(this);
        colorDropdown->addItems({"Black", "Blue", "Green", "Red", "Purple"});
        colorDropdown->setCurrentText("Black"); // Set default value
        colorDropdown->setFixedWidth(200);

        layout->addWidget(colorLabel);
        layout->addWidget(colorDropdown);

        // Emoji Checkbox
        emojiCheck = new QCheckBox("Add an emoji 😊", this);
        layout->addWidget(emojiCheck);

        // Font Size Slider
        QLabel *fontLabel = new QLabel("Select font size:", this);
        fontSlider = new QSlider(Qt::Horizontal, this);
        fontSlider->setRange(10, 30); // Min and Max values
        fontSlider->setValue(12); // Default value
        fontSlider->setTickPosition(QSlider::TicksBoth); // Show ticks on both sides
        fontSlider->setTickInterval(5); // Tick marks every 5 units
        fontSlider->setFixedWidth(200);

        layout->addWidget(fontLabel);
        layout->addWidget(fontSlider);

        // Show Message Button
        QPushButton *button = new QPushButton("Show Message", this);
        // Connect button's clicked signal to showMessage slot
        connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);
        layout->addWidget(button);

        // Result Label
        resultLabel = new QLabel("", this);
        resultLabel->setAlignment(Qt::AlignCenter); // Center text within the label
        resultLabel->setWordWrap(true); // Allow text to wrap if it's too long
        layout->addWidget(resultLabel);

        // Load settings when the application starts
        loadSettings();
    }

private slots:
    void showMessage() {
        QString name = nameInput->text().trimmed(); // Get text and trim whitespace
        QString color = colorDropdown->currentText(); // Get selected text
        bool emoji = emojiCheck->isChecked(); // Get checkbox state
        int fontSize = fontSlider->value(); // Get slider value

        if (name.isEmpty()) {
            // Use QMessageBox for alerts
            QMessageBox::warning(this, "Input Error", "Please enter your name!");
            return;
        }

        QString message = QString("Hello, %1! Your favorite color is %2.").arg(name).arg(color);
        if (emoji) {
            message += " 😊";
        }

        resultLabel->setText(message);

        // Set text color using QPalette
        QPalette palette = resultLabel->palette();
        // QColor can parse common color names like "black", "blue", etc.
        palette.setColor(QPalette::WindowText, QColor(color.toLower()));
        resultLabel->setPalette(palette);

        // Set font size
        QFont font = resultLabel->font(); // Get current font
        font.setPointSize(fontSize); // Set new point size
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
        json data;
        data["name"] = name.toStdString(); // Convert QString to std::string for JSON
        data["color"] = color.toStdString();
        data["emoji"] = emoji;
        data["font_size"] = fontSize;

        QFile file(SAVE_FILE);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << QString::fromStdString(data.dump(4)); // Dump JSON with 4-space indent
            file.close();
        } else {
            qWarning() << "Could not open file for writing:" << file.errorString();
        }
    }

    void loadSettings() {
        QFile file(SAVE_FILE);
        if (!file.exists()) {
            return;
        }

        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString fileContent = in.readAll();
            file.close();

            try {
                json data = json::parse(fileContent.toStdString());

                // Use .value() with default to handle missing keys gracefully
                nameInput->setText(QString::fromStdString(data.value("name", "")));
                colorDropdown->setCurrentText(QString::fromStdString(data.value("color", "Black")));
                emojiCheck->setChecked(data.value("emoji", false));
                fontSlider->setValue(data.value("font_size", 12));

            } catch (const json::parse_error& e) {
                qWarning() << "JSON parse error:" << e.what();
            } catch (const json::exception& e) {
                qWarning() << "JSON error:" << e.what();
            }
        } else {
            qWarning() << "Could not open file for reading:" << file.errorString();
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv); // Create the Qt application object
    InteractiveApp appWindow; // Instantiate your main window
    appWindow.show(); // Show the window
    return a.exec(); // Start the Qt event loop
}

#include "main.moc" // Include the MOC-generated file for signals/slots
