import tkinter as tk
from tkinter import ttk, messagebox, font
import json
import os

class InteractiveApp:
    SAVE_FILE = "user_settings.json"

    def __init__(self, master):
        """
        Initializes the Tkinter application window and widgets.
        Args:
            master: The root Tkinter window.
        """
        self.master = master
        master.title("Interactive App")
        master.geometry("400x350") # Set initial window size
        master.resizable(False, False) # Make window non-resizable for consistent layout

        # Configure a style for better looking widgets (especially Combobox)
        style = ttk.Style()
        style.theme_use('clam') # 'clam', 'alt', 'default', 'classic'

        # Create and pack widgets
        # Name Input
        self.name_label = tk.Label(master, text="Enter your name:")
        self.name_label.pack(pady=(10,0))
        self.name_input = tk.Entry(master, width=30)
        self.name_input.pack(pady=5)

        # Color Dropdown
        self.color_label = tk.Label(master, text="Choose a color:")
        self.color_label.pack()
        self.color_options = ["Black", "Blue", "Green", "Red", "Purple"]
        self.color_dropdown = ttk.Combobox(master, values=self.color_options, state="readonly", width=27)
        self.color_dropdown.set("Black") # Set default value
        self.color_dropdown.pack(pady=5)

        # Emoji Checkbox
        self.emoji_check_var = tk.BooleanVar() # Variable to hold checkbox state
        self.emoji_check = tk.Checkbutton(master, text="Add an emoji 😊", variable=self.emoji_check_var)
        self.emoji_check.pack(pady=5)

        # Font Size Slider
        self.font_label = tk.Label(master, text="Select font size:")
        self.font_label.pack()
        # from_, to, resolution for step, orient for orientation
        self.font_slider = tk.Scale(master, from_=10, to=30, orient=tk.HORIZONTAL,
                                    length=200, showvalue=True, tickinterval=5)
        self.font_slider.set(12) # Set default value
        self.font_slider.pack(pady=5)

        # Show Message Button
        self.show_message_button = tk.Button(master, text="Show Message", command=self.show_message)
        self.show_message_button.pack(pady=10)

        # Result Label
        self.result_label = tk.Label(master, text="", wraplength=350) # wraplength to prevent text going off-screen
        self.result_label.pack(pady=10)

        # Load settings when the application starts
        self.load_settings()

    def show_message(self):
        """
        Generates and displays a message based on user input,
        and saves the current settings.
        """
        name = self.name_input.get().strip()
        color = self.color_dropdown.get()
        emoji = self.emoji_check_var.get()
        font_size = self.font_slider.get()

        if not name:
            # Use tkinter.messagebox for alerts
            messagebox.showwarning("Input Error", "Please enter your name!")
            return

        message = f"Hello, {name}! Your favorite color is {color}."
        if emoji:
            message += " 😊"

        self.result_label.config(text=message) # Update label text
        self.result_label.config(fg=color.lower()) # Set text color (Tkinter uses string names for colors)
        # Create a font object for the label
        self.result_label.config(font=font.Font(size=font_size))

        self.save_settings(name, color, emoji, font_size)

    def save_settings(self, name, color, emoji, font_size):
        """
        Saves the current application settings to a JSON file.
        Args:
            name (str): The entered name.
            color (str): The selected color.
            emoji (bool): The state of the emoji checkbox.
            font_size (int): The selected font size.
        """
        settings = {
            "name": name,
            "color": color,
            "emoji": emoji,
            "font_size": font_size
        }
        try:
            with open(self.SAVE_FILE, 'w') as f:
                json.dump(settings, f, indent=4) # Use indent for pretty printing JSON
        except IOError as e:
            print(f"Error saving settings: {e}") # Print error to console

    def load_settings(self):
        """
        Loads application settings from a JSON file if it exists.
        """
        if not os.path.exists(self.SAVE_FILE):
            return

        try:
            with open(self.SAVE_FILE, 'r') as f:
                settings = json.load(f)

            # Apply loaded settings to widgets, using .get() for safety
            self.name_input.delete(0, tk.END) # Clear current text before inserting
            self.name_input.insert(0, settings.get("name", ""))
            self.color_dropdown.set(settings.get("color", "Black"))
            self.emoji_check_var.set(settings.get("emoji", False))
            self.font_slider.set(settings.get("font_size", 12))

        except (IOError, json.JSONDecodeError) as e:
            print(f"Error loading settings: {e}") # Print error to console

if __name__ == "__main__":
    # Create the root Tkinter window
    root = tk.Tk()
    # Instantiate the application
    app = InteractiveApp(root)
    # Start the Tkinter event loop
    root.mainloop()
