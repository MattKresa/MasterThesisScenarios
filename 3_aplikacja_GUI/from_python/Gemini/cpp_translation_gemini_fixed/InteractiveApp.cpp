#include "InteractiveApp.h" // Include the header for this class

#include <QVBoxLayout>
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
#include <QDebug> // For debugging output

// Define the save file name
const QString SAVE_FILE = "user_settings.json";

/**
 * @brief Constructor for the InteractiveApp.
 * @param parent The parent widget (defaults to nullptr).
 *
 * Initializes the UI, sets up the layout, connects signals and slots,
 * and loads any previously saved settings.
 */
InteractiveApp::InteractiveApp(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Interactive App");
    setGeometry(300, 300, 400, 300); // x, y, width, height

    // --- Widget Initialization ---
    // QLabel for name input
    nameLabel = new QLabel("Enter your name:", this);
    // QLineEdit for name input
    nameInput = new QLineEdit(this);

    // QLabel for color dropdown
    colorLabel = new QLabel("Choose a color:", this);
    // QComboBox for color selection
    colorDropdown = new QComboBox(this);
    colorDropdown->addItems({ "Black", "Blue", "Green", "Red", "Purple" });

    // QCheckBox for adding an emoji
    emojiCheck = new QCheckBox("Add an emoji 😊", this);

    // QLabel for font size slider
    fontLabel = new QLabel("Select font size:", this);
    // QSlider for font size selection (horizontal orientation)
    fontSlider = new QSlider(Qt::Horizontal, this);
    fontSlider->setRange(10, 30); // Set minimum and maximum font size
    fontSlider->setValue(12);     // Set initial font size

    // QPushButton to trigger message display
    button = new QPushButton("Show Message", this);
    // Connect the button's clicked signal to the showMessage slot
    connect(button, &QPushButton::clicked, this, &InteractiveApp::showMessage);

    // QLabel to display the result message
    resultLabel = new QLabel("", this);
    resultLabel->setAlignment(Qt::AlignCenter); // Center align the text

    // --- Layout Setup ---
    QVBoxLayout* layout = new QVBoxLayout(this); // Vertical layout
    layout->addWidget(nameLabel);
    layout->addWidget(nameInput);
    layout->addWidget(colorLabel);
    layout->addWidget(colorDropdown);
    layout->addWidget(emojiCheck);
    layout->addWidget(fontLabel);
    layout->addWidget(fontSlider);
    layout->addWidget(button);
    layout->addWidget(resultLabel);

    // Set the layout for the main window
    setLayout(layout);

    // Load settings when the application starts
    loadSettings();
}

/**
 * @brief Slot to display the message based on user input.
 *
 * Gathers data from UI widgets, constructs a message, applies styling,
 * and saves the current settings.
 */
void InteractiveApp::showMessage()
{
    QString name = nameInput->text().trimmed(); // Get text and trim whitespace
    QString color = colorDropdown->currentText();
    bool emoji = emojiCheck->isChecked();
    int fontSize = fontSlider->value();

    // Input validation: Check if name is empty
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter your name!");
        return; // Stop execution if validation fails
    }

    // Construct the message
    QString message = QString("Hello, %1! Your favorite color is %2.").arg(name).arg(color);
    if (emoji) {
        message += " 😊";
    }

    // Set the message and apply styling (color and font size)
    resultLabel->setText(message);
    // Convert color to lowercase for CSS property
    resultLabel->setStyleSheet(QString("color: %1; font-size: %2px;")
        .arg(color.toLower())
        .arg(fontSize));

    // Save the current settings
    saveSettings(name, color, emoji, fontSize);
}

/**
 * @brief Saves the current user settings to a JSON file.
 * @param name The user's name.
 * @param color The chosen color.
 * @param emoji Whether emoji is enabled.
 * @param fontSize The selected font size.
 */
void InteractiveApp::saveSettings(const QString& name, const QString& color, bool emoji, int fontSize)
{
    QJsonObject settingsObject; // Create a JSON object
    settingsObject["name"] = name;
    settingsObject["color"] = color;
    settingsObject["emoji"] = emoji;
    settingsObject["font_size"] = fontSize;

    // Create a JSON document from the object
    QJsonDocument saveDoc(settingsObject);

    QFile saveFile(SAVE_FILE); // Create a QFile object for the save file
    // Open the file in write mode (creates if not exists, truncates if exists)
    if (saveFile.open(QIODevice::WriteOnly)) {
        // Write the JSON document to the file in a compact format
        saveFile.write(saveDoc.toJson());
        saveFile.close(); // Close the file
        qDebug() << "Settings saved successfully.";
    }
    else {
        qWarning() << "Couldn't open save file:" << SAVE_FILE << "for writing.";
    }
}

/**
 * @brief Loads user settings from a JSON file.
 *
 * If the save file exists, it reads the settings and applies them
 * to the respective UI widgets. Provides default values if keys are missing.
 */
void InteractiveApp::loadSettings()
{
    QFile loadFile(SAVE_FILE); // Create a QFile object for the load file

    // Check if the file exists before attempting to open
    if (!loadFile.exists()) {
        qDebug() << "Save file not found:" << SAVE_FILE;
        return; // No file to load from
    }

    // Open the file in read-only mode
    if (loadFile.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = loadFile.readAll(); // Read all content into a byte array
        loadFile.close(); // Close the file

        // Create a JSON document from the byte array
        QJsonDocument loadDoc = QJsonDocument::fromJson(jsonData);

        // Check if the document is a valid JSON object
        if (loadDoc.isObject()) {
            QJsonObject settingsObject = loadDoc.object(); // Get the root JSON object

            // Retrieve values with default fallbacks
            nameInput->setText(settingsObject.value("name").toString(""));
            colorDropdown->setCurrentText(settingsObject.value("color").toString("Black"));
            emojiCheck->setChecked(settingsObject.value("emoji").toBool(false));
            fontSlider->setValue(settingsObject.value("font_size").toInt(12));

            qDebug() << "Settings loaded successfully.";
        }
        else {
            qWarning() << "Invalid JSON format in save file:" << SAVE_FILE;
        }
    }
    else {
        qWarning() << "Couldn't open save file:" << SAVE_FILE << "for reading.";
    }
}

// This file is generated by moc (Meta-Object Compiler) from InteractiveApp.h
// It must be included at the end of the .cpp file that implements the Q_OBJECT class.
#include "main.moc"
