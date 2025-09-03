import sys
import json
from PySide6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QWidget, QLabel,
                             QLineEdit, QComboBox, QCheckBox, QSlider, QPushButton,
                             QMessageBox)
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor, QFont

SAVE_FILE = "user_settings.json"


class InteractiveApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Interactive App")

        # Create widgets
        self.name_label = QLabel("Enter your name:")
        self.name_input = QLineEdit()

        self.color_label = QLabel("Choose a color:")
        self.color_dropdown = QComboBox()
        self.color_dropdown.addItems(["Black", "Blue", "Green", "Red", "Purple"])
        self.color_dropdown.setCurrentText("Black")

        self.emoji_check = QCheckBox("Add an emoji 😊")

        self.font_label = QLabel("Select font size:")
        self.font_slider = QSlider(Qt.Horizontal)
        self.font_slider.setRange(10, 30)
        self.font_slider.setValue(12)
        self.font_slider.setTickPosition(QSlider.TicksBelow)
        self.font_slider.setTickInterval(5)

        self.button = QPushButton("Show Message")
        self.button.clicked.connect(self.show_message)

        self.result_label = QLabel("")
        self.result_label.setAlignment(Qt.AlignCenter)

        # Layout
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

        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

        self.resize(400, 300)

        self.load_settings()

    def show_message(self):
        name = self.name_input.text().strip()
        color = self.color_dropdown.currentText()
        emoji = self.emoji_check.isChecked()
        font_size = self.font_slider.value()

        if not name:
            QMessageBox.warning(self, "Warning", "Please enter your name!")
            return

        message = f"Hello, {name}! Your favorite color is {color}."
        if emoji:
            message += " 😊"

        self.result_label.setText(message)

        # Set color (using basic color names for simplicity)
        color_map = {
            "Black": QColor(0, 0, 0),
            "Blue": QColor(0, 0, 255),
            "Green": QColor(0, 128, 0),
            "Red": QColor(255, 0, 0),
            "Purple": QColor(128, 0, 128)
        }
        self.result_label.setStyleSheet(f"color: {color_map[color].name()}")

        # Set font size
        font = self.result_label.font()
        font.setPointSize(font_size)
        self.result_label.setFont(font)

        self.save_settings(name, color, emoji, font_size)

    def save_settings(self, name, color, emoji, font_size):
        data = {
            "name": name,
            "color": color,
            "emoji": emoji,
            "font_size": font_size
        }

        try:
            with open(SAVE_FILE, 'w') as f:
                json.dump(data, f)
        except Exception as e:
            print(f"Error saving settings: {e}")

    def load_settings(self):
        try:
            with open(SAVE_FILE, 'r') as f:
                data = json.load(f)

            self.name_input.setText(data.get("name", ""))
            self.color_dropdown.setCurrentText(data.get("color", "Black"))
            self.emoji_check.setChecked(data.get("emoji", False))
            self.font_slider.setValue(data.get("font_size", 12))
        except FileNotFoundError:
            pass  # First run, no settings file yet
        except Exception as e:
            print(f"Error loading settings: {e}")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = InteractiveApp()
    window.show()
    sys.exit(app.exec_())