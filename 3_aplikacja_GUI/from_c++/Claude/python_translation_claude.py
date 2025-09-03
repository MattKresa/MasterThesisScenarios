#!/usr/bin/env python3
"""
Interactive App - Python translation from C++ Qt
"""

import sys
import json
import os
from PySide6.QtWidgets import (
    QApplication, QWidget, QLabel, QPushButton, QVBoxLayout, QLineEdit,
    QComboBox, QCheckBox, QSlider, QMessageBox
)
from PySide6.QtCore import Qt


class InteractiveApp(QWidget):
    """Main application window for the Interactive App."""

    SAVE_FILE = "user_settings.json"

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setup_ui()
        self.load_settings()

    def setup_ui(self):
        """Initialize and setup the user interface."""
        self.setWindowTitle("Interactive App")
        self.setGeometry(300, 300, 400, 300)

        # Create widgets
        self.name_label = QLabel("Enter your name:")
        self.name_input = QLineEdit()

        self.color_label = QLabel("Choose a color:")
        self.color_dropdown = QComboBox()
        self.color_dropdown.addItems(["Black", "Blue", "Green", "Red", "Purple"])

        self.emoji_check = QCheckBox("Add an emoji 😊")

        self.font_label = QLabel("Select font size:")
        self.font_slider = QSlider(Qt.Horizontal)
        self.font_slider.setRange(10, 30)
        self.font_slider.setValue(12)

        self.button = QPushButton("Show Message")
        self.result_label = QLabel("")
        self.result_label.setAlignment(Qt.AlignCenter)

        # Setup layout
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
        self.setLayout(layout)

        # Connect signals
        self.button.clicked.connect(self.show_message)

    def show_message(self):
        """Handle the show message button click."""
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
        self.result_label.setStyleSheet(f"color: {color.lower()}; font-size: {font_size}px;")

        self.save_settings(name, color, emoji, font_size)

    def save_settings(self, name, color, emoji, font_size):
        """Save user settings to JSON file."""
        data = {
            "name": name,
            "color": color,
            "emoji": emoji,
            "font_size": font_size
        }

        try:
            with open(self.SAVE_FILE, "w", encoding="utf-8") as file:
                json.dump(data, file, indent=2)
        except IOError as e:
            print(f"Error saving settings: {e}")

    def load_settings(self):
        """Load user settings from JSON file if it exists."""
        if os.path.exists(self.SAVE_FILE):
            try:
                with open(self.SAVE_FILE, "r", encoding="utf-8") as file:
                    data = json.load(file)

                    self.name_input.setText(data.get("name", ""))
                    self.color_dropdown.setCurrentText(data.get("color", "Black"))
                    self.emoji_check.setChecked(data.get("emoji", False))
                    self.font_slider.setValue(data.get("font_size", 12))

            except (IOError, json.JSONDecodeError) as e:
                print(f"Error loading settings: {e}")


def main():
    """Main application entry point."""
    app = QApplication(sys.argv)

    window = InteractiveApp()
    window.show()

    return app.exec()


if __name__ == "__main__":
    sys.exit(main())