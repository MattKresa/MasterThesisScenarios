import tkinter as tk
from tkinter import ttk, messagebox
import json
import os

class InteractiveApp:
    SAVE_FILE = "user_settings.json"
    
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Interactive App")
        self.root.geometry("400x400")
        
        # Create variables
        self.name_var = tk.StringVar()
        self.color_var = tk.StringVar(value="black")
        self.emoji_var = tk.BooleanVar()
        self.font_size_var = tk.IntVar(value=12)
        
        self.setup_ui()
        self.load_settings()
    
    def setup_ui(self):
        # Main frame
        main_frame = ttk.Frame(self.root, padding="20")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Name input
        ttk.Label(main_frame, text="Enter your name:").pack(pady=5)
        self.name_entry = ttk.Entry(main_frame, textvariable=self.name_var, width=30)
        self.name_entry.pack(pady=5)
        
        # Color dropdown
        ttk.Label(main_frame, text="Choose a color:").pack(pady=5)
        self.color_combo = ttk.Combobox(main_frame, textvariable=self.color_var, 
                                       values=["black", "blue", "green", "red", "purple"],
                                       state="readonly", width=27)
        self.color_combo.pack(pady=5)
        
        # Emoji checkbox
        self.emoji_check = ttk.Checkbutton(main_frame, text="Add an emoji 😊", 
                                          variable=self.emoji_var)
        self.emoji_check.pack(pady=5)
        
        # Font size slider
        ttk.Label(main_frame, text="Select font size:").pack(pady=5)
        self.font_slider = ttk.Scale(main_frame, from_=10, to=30, 
                                    variable=self.font_size_var, orient=tk.HORIZONTAL)
        self.font_slider.pack(pady=5, fill=tk.X, padx=20)
        
        # Font size display
        self.font_size_label = ttk.Label(main_frame, text="12")
        self.font_size_label.pack(pady=2)
        
        # Update font size display when slider changes
        self.font_slider.configure(command=self.update_font_size_display)
        
        # Show message button
        show_button = ttk.Button(main_frame, text="Show Message", command=self.show_message)
        show_button.pack(pady=15)
        
        # Result label
        self.result_label = tk.Label(main_frame, text="", wraplength=350, justify=tk.CENTER)
        self.result_label.pack(pady=10, fill=tk.X)
    
    def update_font_size_display(self, value=None):
        """Update the font size display label"""
        size = int(float(self.font_size_var.get()))
        self.font_size_label.config(text=str(size))
    
    def show_message(self):
        name = self.name_var.get().strip()
        color = self.color_var.get()
        emoji = self.emoji_var.get()
        font_size = self.font_size_var.get()
        
        if not name:
            messagebox.showwarning("Warning", "Please enter your name!")
            return
        
        message = f"Hello, {name}! Your favorite color is {color}."
        if emoji:
            message += " 😊"
        
        # Update result label
        self.result_label.config(
            text=message,
            fg=color,
            font=("Arial", font_size)
        )
        
        self.save_settings(name, color, emoji, font_size)
    
    def save_settings(self, name, color, emoji, font_size):
        """Save user settings to JSON file"""
        data = {
            "name": name,
            "color": color,
            "emoji": emoji,
            "font_size": font_size
        }
        
        try:
            with open(self.SAVE_FILE, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2)
        except Exception as e:
            print(f"Error saving settings: {e}")
    
    def load_settings(self):
        """Load user settings from JSON file"""
        if not os.path.exists(self.SAVE_FILE):
            return
        
        try:
            with open(self.SAVE_FILE, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            self.name_var.set(data.get("name", ""))
            self.color_var.set(data.get("color", "black"))
            self.emoji_var.set(data.get("emoji", False))
            self.font_size_var.set(data.get("font_size", 12))
            
            # Update font size display
            self.update_font_size_display()
            
        except Exception as e:
            print(f"Error loading settings: {e}")
    
    def run(self):
        """Start the application"""
        self.root.mainloop()

if __name__ == "__main__":
    app = InteractiveApp()
    app.run()