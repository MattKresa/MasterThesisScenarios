import tkinter as tk
from tkinter import ttk, messagebox
import json
import os

SAVE_FILE = "user_settings.json"

class InteractiveApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Interactive App")
        self.root.geometry("400x300")

        # Name label and input
        tk.Label(root, text="Enter your name:").pack()
        self.name_input = tk.Entry(root)
        self.name_input.pack()

        # Color dropdown
        tk.Label(root, text="Choose a color:").pack()
        self.color_var = tk.StringVar(value="Black")
        self.color_dropdown = ttk.Combobox(root, textvariable=self.color_var, values=["Black", "Blue", "Green", "Red", "Purple"], state="readonly")
        self.color_dropdown.pack()

        # Emoji checkbox
        self.emoji_var = tk.BooleanVar()
        tk.Checkbutton(root, text="Add an emoji 😊", variable=self.emoji_var).pack()

        # Font size slider
        tk.Label(root, text="Select font size:").pack()
        self.font_var = tk.DoubleVar(value=12)
        self.font_slider = tk.Scale(root, from_=10, to=30, orient=tk.HORIZONTAL, variable=self.font_var)
        self.font_slider.pack()

        # Show message button
        tk.Button(root, text="Show Message", command=self.show_message).pack()

        # Result label
        self.result_label = tk.Label(root, text="", anchor="center")
        self.result_label.pack(pady=10)

        # Load settings if available
        self.load_settings()

    def show_message(self):
        name = self.name_input.get().strip()
        color = self.color_var.get()
        emoji = self.emoji_var.get()
        font_size = int(self.font_var.get())

        if not name:
            messagebox.showwarning("Warning", "Please enter your name!")
            return

        message = f"Hello, {name}! Your favorite color is {color}."
        if emoji:
            message += " 😊"

        self.result_label.config(text=message, fg=color.lower(), font=("Arial", font_size))

        self.save_settings(name, color, emoji, font_size)

    def save_settings(self, name, color, emoji, font_size):
        data = {
            "name": name,
            "color": color,
            "emoji": emoji,
            "font_size": font_size
        }
        try:
            with open(SAVE_FILE, "w") as f:
                json.dump(data, f)
        except IOError as e:
            print("Error saving settings:", e)

    def load_settings(self):
        if not os.path.exists(SAVE_FILE):
            return
        try:
            with open(SAVE_FILE, "r") as f:
                data = json.load(f)
                self.name_input.insert(0, data.get("name", ""))
                self.color_var.set(data.get("color", "Black"))
                self.emoji_var.set(data.get("emoji", False))
                self.font_var.set(data.get("font_size", 12))
        except IOError as e:
            print("Error loading settings:", e)


if __name__ == "__main__":
    root = tk.Tk()
    app = InteractiveApp(root)
    root.mainloop()
