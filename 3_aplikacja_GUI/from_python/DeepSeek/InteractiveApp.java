import javafx.application.Application;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;
import javafx.scene.text.Font;
import javafx.scene.paint.Color;
import javafx.scene.text.Text;
import javafx.util.StringConverter;
import javafx.util.converter.IntegerStringConverter;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

public class InteractiveApp extends Application {
    private static final String SAVE_FILE = "user_settings.json";
    
    private TextField nameInput;
    private ComboBox<String> colorDropdown;
    private CheckBox emojiCheck;
    private Slider fontSlider;
    private Text resultLabel;
    
    public static void main(String[] args) {
        launch(args);
    }
    
    @Override
    public void start(Stage primaryStage) {
        primaryStage.setTitle("Interactive App");
        
        // Create UI components
        Label nameLabel = new Label("Enter your name:");
        nameInput = new TextField();
        
        Label colorLabel = new Label("Choose a color:");
        colorDropdown = new ComboBox<>();
        colorDropdown.getItems().addAll("Black", "Blue", "Green", "Red", "Purple");
        
        emojiCheck = new CheckBox("Add an emoji \uD83D\uDE0A");
        
        Label fontLabel = new Label("Select font size:");
        fontSlider = new Slider(10, 30, 12);
        fontSlider.setShowTickLabels(true);
        fontSlider.setShowTickMarks(true);
        fontSlider.setMajorTickUnit(5);
        fontSlider.setMinorTickCount(1);
        fontSlider.setSnapToTicks(true);
        
        Button button = new Button("Show Message");
        button.setOnAction(e -> showMessage());
        
        resultLabel = new Text();
        resultLabel.setTextAlignment(javafx.scene.text.TextAlignment.CENTER);
        
        // Layout
        VBox layout = new VBox(10);
        layout.setPadding(new Insets(20));
        layout.setAlignment(Pos.CENTER);
        layout.getChildren().addAll(
            nameLabel, nameInput,
            colorLabel, colorDropdown,
            emojiCheck,
            fontLabel, fontSlider,
            button, resultLabel
        );
        
        // Load saved settings
        loadSettings();
        
        primaryStage.setScene(new Scene(layout, 400, 400));
        primaryStage.show();
    }
    
    private void showMessage() {
        String name = nameInput.getText().trim();
        String color = colorDropdown.getValue();
        boolean emoji = emojiCheck.isSelected();
        int fontSize = (int)fontSlider.getValue();
        
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
            message += " \uD83D\uDE0A";
        }
        
        resultLabel.setText(message);
        resultLabel.setFill(Color.web(color.toLowerCase()));
        resultLabel.setFont(Font.font(fontSize));
        
        saveSettings(name, color, emoji, fontSize);
    }
    
    private void saveSettings(String name, String color, boolean emoji, int fontSize) {
        Map<String, Object> data = new HashMap<>();
        data.put("name", name);
        data.put("color", color);
        data.put("emoji", emoji);
        data.put("font_size", fontSize);
        
        try (Writer writer = new FileWriter(SAVE_FILE)) {
            new Gson().toJson(data, writer);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    
    private void loadSettings() {
        if (Files.exists(Paths.get(SAVE_FILE))) {
            try (Reader reader = new FileReader(SAVE_FILE)) {
                Map<String, Object> data = new Gson().fromJson(
                    reader, 
                    new TypeToken<Map<String, Object>>(){}.getType()
                );
                
                nameInput.setText(data.getOrDefault("name", "").toString());
                colorDropdown.setValue(data.getOrDefault("color", "Black").toString());
                emojiCheck.setSelected(Boolean.parseBoolean(data.getOrDefault("emoji", "false").toString()));
                fontSlider.setValue(Double.parseDouble(data.getOrDefault("font_size", "12").toString()));
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}