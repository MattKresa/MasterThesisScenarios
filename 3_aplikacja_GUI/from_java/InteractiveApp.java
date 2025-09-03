import javafx.application.Application;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;
import javafx.scene.text.Font;
import javafx.scene.paint.Color;

import com.fasterxml.jackson.databind.ObjectMapper;
import java.io.File;
import java.io.IOException;
import java.util.Map;
import java.util.HashMap;

public class InteractiveApp extends Application {

    private static final String SAVE_FILE = "user_settings.json";

    private TextField nameInput;
    private ComboBox<String> colorDropdown;
    private CheckBox emojiCheck;
    private Slider fontSlider;
    private Label resultLabel;

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
        fontSlider.setShowTickMarks(true);
        fontSlider.setShowTickLabels(true);

        Button button = new Button("Show Message");
        button.setOnAction(e -> showMessage());

        resultLabel = new Label("");
        resultLabel.setAlignment(Pos.CENTER);

        VBox layout = new VBox(10, nameLabel, nameInput, colorLabel, colorDropdown,
                emojiCheck, fontLabel, fontSlider, button, resultLabel);
        layout.setAlignment(Pos.CENTER);
        layout.setPrefSize(400, 300);

        primaryStage.setScene(new Scene(layout));
        primaryStage.show();

        loadSettings();
    }

    private void showMessage() {
        String name = nameInput.getText().trim();
        String color = colorDropdown.getValue();
        boolean emoji = emojiCheck.isSelected();
        int fontSize = (int) fontSlider.getValue();

        if (name.isEmpty()) {
            Alert alert = new Alert(Alert.AlertType.WARNING, "Please enter your name!");
            alert.showAndWait();
            return;
        }

        String message = "Hello, " + name + "! Your favorite color is " + color + ".";
        if (emoji) {
            message += " 😊";
        }

        resultLabel.setText(message);
        resultLabel.setTextFill(Color.web(color.toLowerCase()));
        resultLabel.setFont(Font.font(fontSize));

        saveSettings(name, color, emoji, fontSize);
    }

    private void saveSettings(String name, String color, boolean emoji, int fontSize) {
        Map<String, Object> data = new HashMap<>();
        data.put("name", name);
        data.put("color", color);
        data.put("emoji", emoji);
        data.put("font_size", fontSize);

        ObjectMapper mapper = new ObjectMapper();
        try {
            mapper.writeValue(new File(SAVE_FILE), data);
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    private void loadSettings() {
        File file = new File(SAVE_FILE);
        if (!file.exists()) return;

        ObjectMapper mapper = new ObjectMapper();
        try {
            Map<String, Object> data = mapper.readValue(file, Map.class);
            nameInput.setText((String) data.getOrDefault("name", ""));
            colorDropdown.setValue((String) data.getOrDefault("color", "Black"));
            emojiCheck.setSelected((Boolean) data.getOrDefault("emoji", false));
            fontSlider.setValue(((Number) data.getOrDefault("font_size", 12)).intValue());
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    public static void main(String[] args) {
        launch(args);
    }
}