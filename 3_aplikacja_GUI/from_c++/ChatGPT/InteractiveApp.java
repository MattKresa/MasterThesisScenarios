package com.example.interactiveapp;

import javafx.application.Application;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.stage.Stage;
import org.json.JSONObject;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

public class InteractiveApp extends Application {

    private TextField nameInput;
    private ComboBox<String> colorDropdown;
    private CheckBox emojiCheck;
    private Slider fontSlider;
    private Label resultLabel;

    private static final String SAVE_FILE = "user_settings.json";

    @Override
    public void start(Stage primaryStage) {
        primaryStage.setTitle("Interactive App");

        Label nameLabel = new Label("Enter your name:");
        nameInput = new TextField();

        Label colorLabel = new Label("Choose a color:");
        colorDropdown = new ComboBox<>();
        colorDropdown.getItems().addAll("Black", "Blue", "Green", "Red", "Purple");
        colorDropdown.setValue("Black");

        emojiCheck = new CheckBox("Add an emoji 😊");

        Label fontLabel = new Label("Select font size:");
        fontSlider = new Slider(10, 30, 12);
        fontSlider.setShowTickLabels(true);
        fontSlider.setShowTickMarks(true);
        fontSlider.setMajorTickUnit(5);
        fontSlider.setMinorTickCount(1);
        fontSlider.setBlockIncrement(1);

        Button button = new Button("Show Message");
        button.setOnAction(e -> showMessage());

        resultLabel = new Label("");
        resultLabel.setAlignment(Pos.CENTER);

        VBox layout = new VBox(10);
        layout.setPadding(new Insets(10));
        layout.setAlignment(Pos.CENTER_LEFT);
        layout.getChildren().addAll(
                nameLabel, nameInput,
                colorLabel, colorDropdown,
                emojiCheck,
                fontLabel, fontSlider,
                button, resultLabel
        );

        loadSettings();

        Scene scene = new Scene(layout, 400, 300);
        primaryStage.setScene(scene);
        primaryStage.show();
    }

    private void showMessage() {
        String name = nameInput.getText().trim();
        String color = colorDropdown.getValue();
        boolean emoji = emojiCheck.isSelected();
        int fontSize = (int) fontSlider.getValue();

        if (name.isEmpty()) {
            Alert alert = new Alert(Alert.AlertType.WARNING);
            alert.setTitle("Warning");
            alert.setHeaderText(null);
            alert.setContentText("Please enter your name!");
            alert.showAndWait();
            return;
        }

        String message = "Hello, " + name + "! Your favorite color is " + color + ".";
        if (emoji) {
            message += " 😊";
        }

        resultLabel.setText(message);
        resultLabel.setTextFill(getColorFromName(color));
        resultLabel.setFont(new Font(fontSize));

        saveSettings(name, color, emoji, fontSize);
    }

    private void saveSettings(String name, String color, boolean emoji, int fontSize) {
        try {
            JSONObject obj = new JSONObject();
            obj.put("name", name);
            obj.put("color", color);
            obj.put("emoji", emoji);
            obj.put("font_size", fontSize);

            Files.write(Paths.get(SAVE_FILE), obj.toString(4).getBytes());
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    private void loadSettings() {
        try {
            if (Files.exists(Paths.get(SAVE_FILE))) {
                String content = new String(Files.readAllBytes(Paths.get(SAVE_FILE)));
                JSONObject obj = new JSONObject(content);

                nameInput.setText(obj.optString("name", ""));
                colorDropdown.setValue(obj.optString("color", "Black"));
                emojiCheck.setSelected(obj.optBoolean("emoji", false));
                fontSlider.setValue(obj.optInt("font_size", 12));
            }
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    private Color getColorFromName(String colorName) {
        switch (colorName.toLowerCase()) {
            case "blue": return Color.BLUE;
            case "green": return Color.GREEN;
            case "red": return Color.RED;
            case "purple": return Color.PURPLE;
            default: return Color.BLACK;
        }
    }

    public static void main(String[] args) {
        launch(args);
    }
}
