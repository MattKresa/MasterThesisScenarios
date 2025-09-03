import sys
import json
from PySide6.QtWidgets import (
    QApplication, QWidget, QLabel, QLineEdit, QComboBox,
    QCheckBox, QSlider, QPushButton, QVBoxLayout, QMessageBox
)
from PySide6.QtCore import Qt, QSettings

class InteractiveApp(QWidget):
    """
    A PyQt5 application that demonstrates interactive UI elements,
    message display, and saving/loading user settings.
    """

    # Define the file name for saving user settings
    SAVE_FILE = "user_settings.json"

    def __init__(self, parent=None):
        """
        Initializes the InteractiveApp window and its UI components.
        """
        super().__init__(parent)
        self.setWindowTitle("Interactive App")
        # Set initial window geometry (x, y, width, height)
        self.setGeometry(300, 300, 400, 300)

        # Initialize UI components
        self.name_label = QLabel("Enter your name:")
        self.name_input = QLineEdit()

        self.color_label = QLabel("Choose a color:")
        self.color_dropdown = QComboBox()
        self.color_dropdown.addItems(["Black", "Blue", "Green", "Red", "Purple"])

        self.emoji_check = QCheckBox("Add an emoji 😊")

        self.font_label = QLabel("Select font size:")
        self.font_slider = QSlider(Qt.Horizontal)
        self.font_slider.setRange(10, 30)  # Set font size range from 10 to 30
        self.font_slider.setValue(12)      # Set default font size to 12

        self.button = QPushButton("Show Message")
        self.result_label = QLabel("")
        self.result_label.setAlignment(Qt.AlignCenter) # Center align the result text

        # Create a vertical layout and add all widgets
        layout = QVBoxLayout()
        layout.addWidget(self.name_label)
        layout.addWidget(self.name_input)
        layout.addWidget(self.color_label)
        layout.addWidget(self.color_dropdown)
        layout.addWidget(self.emoji_check)
        layout.addWidget(self.font_label)
        layout.addWidget(self.font_slider)
        layout.addWidget(self.button)
        layout.addWidget(self.result_label)
        self.setLayout(layout) # Set the layout for the window

        # Connect the button's clicked signal to the showMessage slot
        self.button.clicked.connect(self.showMessage)

        # Load previously saved settings when the application starts
        self.loadSettings()

    def showMessage(self):
        """
        Retrieves user input, constructs a message, displays it with
        selected styling, and saves the current settings.
        """
        name = self.name_input.text().strip() # Get name and remove leading/trailing whitespace
        color = self.color_dropdown.currentText()
        emoji = self.emoji_check.isChecked()
        font_size = self.font_slider.value()

        # Validate if the name input is empty
        if not name:
            QMessageBox.warning(self, "Warning", "Please enter your name!")
            return

        # Construct the message string
        message = f"Hello, {name}! Your favorite color is {color}."
        if emoji:
            message += " 😊"

        # Set the message text and apply styling (color and font size)
        self.result_label.setText(message)
        self.result_label.setStyleSheet(f"color: {color.lower()}; font-size: {font_size}px;")

        # Save the current settings
        self.saveSettings(name, color, emoji, font_size)

    def saveSettings(self, name, color, emoji, font_size):
        """
        Saves the current application settings (name, color, emoji, font size)
        to a JSON file.
        """
        settings = {
            "name": name,
            "color": color,
            "emoji": emoji,
            "font_size": font_size
        }
        try:
            with open(self.SAVE_FILE, 'w') as f:
                json.dump(settings, f, indent=4) # Write settings to JSON file with indentation
        except IOError as e:
            print(f"Error saving settings: {e}") # Print error if file operation fails

    def loadSettings(self):
        """
        Loads previously saved application settings from a JSON file
        and applies them to the UI components.
        """
        try:
            with open(self.SAVE_FILE, 'r') as f:
                settings = json.load(f) # Load settings from JSON file

            # Apply loaded settings to UI components, providing default values
            self.name_input.setText(settings.get("name", ""))
            # Set current text, default to "Black" if not found
            self.color_dropdown.setCurrentText(settings.get("color", "Black"))
            self.emoji_check.setChecked(settings.get("emoji", False)) # Default to False
            self.font_slider.setValue(settings.get("font_size", 12)) # Default to 12
        except FileNotFoundError:
            print(f"Settings file '{self.SAVE_FILE}' not found. Using default settings.")
        except json.JSONDecodeError as e:
            print(f"Error decoding JSON from settings file: {e}")
        except Exception as e:
            print(f"An unexpected error occurred while loading settings: {e}")


if __name__ == "__main__":
    # Create the QApplication instance
    app = QApplication(sys.argv)
    # Create an instance of the InteractiveApp window
    window = InteractiveApp()
    # Show the window
    window.show()
    # Start the application's event loop
    sys.exit(app.exec_())
