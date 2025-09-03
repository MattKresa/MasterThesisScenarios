package com.example.interactiveapp;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import javafx.application.Application;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

import java.io.*;
import java.nio.charset.StandardCharsets;

public class InteractiveApp extends Application {

    private static final String SAVE_FILE = "user_settings.json";

    private TextField nameInput;
    private ComboBox<String> colorDropdown;
    private CheckBox emojiCheck;
    private Slider fontSlider;
    private Label resultLabel;

    @Override
    public void start(Stage stage) {
        stage.setTitle("Interactive App");

        // Widgets
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

        Button button = new Button("Show Message");
        button.setOnAction(e -> showMessage());

        resultLabel = new Label("");
        resultLabel.setAlignment(Pos.CENTER);

        VBox layout = new VBox(10);
        layout.setPadding(new Insets(10));
        layout.getChildren().addAll(
                nameLabel, nameInput,
                colorLabel, colorDropdown,
                emojiCheck,
                fontLabel, fontSlider,
                button, resultLabel
        );

        Scene scene = new Scene(layout, 400, 300);
        stage.setScene(scene);

        loadSettings();

        stage.show();
    }

    private void showMessage() {
        String name = nameInput.getText().trim();
        String color = colorDropdown.getValue();
        boolean emoji = emojiCheck.isSelected();
        int fontSize = (int) fontSlider.getValue();

        if (name.isEmpty()) {
            Alert alert = new Alert(Alert.AlertType.WARNING, "Please enter your name!", ButtonType.OK);
            alert.setHeaderText("Warning");
            alert.showAndWait();
            return;
        }

        String message = String.format("Hello, %s! Your favorite color is %s.", name, color);
        if (emoji) {
            message += " 😊";
        }

        resultLabel.setText(message);
        resultLabel.setStyle(String.format("-fx-text-fill: %s; -fx-font-size: %dpx;", color.toLowerCase(), fontSize));

        saveSettings(name, color, emoji, fontSize);
    }

    private void saveSettings(String name, String color, boolean emoji, int fontSize) {
        Gson gson = new Gson();
        JsonObject obj = new JsonObject();
        obj.addProperty("name", name);
        obj.addProperty("color", color);
        obj.addProperty("emoji", emoji);
        obj.addProperty("font_size", fontSize);

        try (Writer writer = new OutputStreamWriter(new FileOutputStream(SAVE_FILE), StandardCharsets.UTF_8)) {
            gson.toJson(obj, writer);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void loadSettings() {
        File file = new File(SAVE_FILE);
        if (!file.exists()) return;

        Gson gson = new Gson();
        try (Reader reader = new InputStreamReader(new FileInputStream(file), StandardCharsets.UTF_8)) {
            JsonObject obj = gson.fromJson(reader, JsonObject.class);
            nameInput.setText(obj.has("name") ? obj.get("name").getAsString() : "");
            colorDropdown.setValue(obj.has("color") ? obj.get("color").getAsString() : "Black");
            emojiCheck.setSelected(obj.has("emoji") && obj.get("emoji").getAsBoolean());
            fontSlider.setValue(obj.has("font_size") ? obj.get("font_size").getAsInt() : 12);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) {
        launch(args);
    }
}
