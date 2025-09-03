// InteractiveApp.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import java.util.HashMap;
import java.util.Map;

/**
 * Interactive Application - Java Swing implementation
 * Translated from C++ Qt application
 */
public class InteractiveAppFixed extends JFrame {

    // Constants
    private static final String SAVE_FILE = "user_settings.json";

    // UI Components
    private JLabel nameLabel;
    private JTextField nameInput;
    private JLabel colorLabel;
    private JComboBox<String> colorDropdown;
    private JCheckBox emojiCheck;
    private JLabel fontLabel;
    private JSlider fontSlider;
    private JButton button;
    private JLabel resultLabel;

    // JSON handler
    private Gson gson;

    /**
     * Constructor - initializes the application
     */
    public InteractiveApp() {
        gson = new Gson();
        initializeUI();
        loadSettings();
    }

    /**
     * Initialize and setup the user interface
     */
    private void initializeUI() {
        setTitle("Interactive App");
        setBounds(300, 300, 400, 300);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        // Create UI components
        createComponents();
        setupLayout();
        connectEventHandlers();
    }

    /**
     * Create all UI components
     */
    private void createComponents() {
        nameLabel = new JLabel("Enter your name:");
        nameInput = new JTextField();
        nameInput.setPreferredSize(new Dimension(250, 25));

        colorLabel = new JLabel("Choose a color:");
        colorDropdown = new JComboBox<>(new String[]{"Black", "Blue", "Green", "Red", "Purple"});
        colorDropdown.setPreferredSize(new Dimension(250, 25));

        emojiCheck = new JCheckBox("Add an emoji 😊");

        fontLabel = new JLabel("Select font size:");
        fontSlider = new JSlider(JSlider.HORIZONTAL, 10, 30, 12);
        fontSlider.setMajorTickSpacing(5);
        fontSlider.setMinorTickSpacing(1);
        fontSlider.setPaintTicks(true);
        fontSlider.setPaintLabels(true);
        fontSlider.setPreferredSize(new Dimension(250, 50));

        button = new JButton("Show Message");
        button.setPreferredSize(new Dimension(150, 30));

        resultLabel = new JLabel("");
        resultLabel.setHorizontalAlignment(SwingConstants.CENTER);
        resultLabel.setPreferredSize(new Dimension(350, 40));
        resultLabel.setBorder(BorderFactory.createEtchedBorder());
    }

    /**
     * Setup the layout manager and add components
     */
    private void setupLayout() {
        setLayout(new BoxLayout(getContentPane(), BoxLayout.Y_AXIS));

        // Add components with proper spacing
        add(Box.createVerticalStrut(10));
        add(createCenteredPanel(nameLabel));
        add(Box.createVerticalStrut(5));
        add(createCenteredPanel(nameInput));
        add(Box.createVerticalStrut(10));
        add(createCenteredPanel(colorLabel));
        add(Box.createVerticalStrut(5));
        add(createCenteredPanel(colorDropdown));
        add(Box.createVerticalStrut(10));
        add(createCenteredPanel(emojiCheck));
        add(Box.createVerticalStrut(10));
        add(createCenteredPanel(fontLabel));
        add(Box.createVerticalStrut(5));
        add(createCenteredPanel(fontSlider));
        add(Box.createVerticalStrut(15));
        add(createCenteredPanel(button));
        add(Box.createVerticalStrut(15));
        add(createCenteredPanel(resultLabel));
        add(Box.createVerticalStrut(10));
    }

    /**
     * Create a panel with centered component
     */
    private JPanel createCenteredPanel(JComponent component) {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.CENTER));
        panel.add(component);
        return panel;
    }

    /**
     * Connect event handlers to UI components
     */
    private void connectEventHandlers() {
        button.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                showMessage();
            }
        });
    }

    /**
     * Handle the show message action (equivalent to C++ slot)
     */
    private void showMessage() {
        String name = nameInput.getText().trim();
        String color = (String) colorDropdown.getSelectedItem();
        boolean emoji = emojiCheck.isSelected();
        int fontSize = fontSlider.getValue();

        // Validate input
        if (name.isEmpty()) {
            JOptionPane.showMessageDialog(this, "Please enter your name!", "Warning", JOptionPane.WARNING_MESSAGE);
            return;
        }

        // Build message
        String message = String.format("Hello, %s! Your favorite color is %s.", name, color);
        if (emoji) {
            message += " 😊";
        }

        // Update result label
        resultLabel.setText(message);
        updateResultLabelStyle(color, fontSize);

        // Save settings
        saveSettings(name, color, emoji, fontSize);
    }

    /**
     * Update the result label with color and font size styling
     */
    private void updateResultLabelStyle(String colorName, int fontSize) {
        Color textColor = getColorFromString(colorName);
        Font currentFont = resultLabel.getFont();
        Font newFont = new Font(currentFont.getName(), currentFont.getStyle(), fontSize);

        resultLabel.setFont(newFont);
        resultLabel.setForeground(textColor);
    }

    /**
     * Convert color name string to Color object
     */
    private Color getColorFromString(String colorName) {
        switch (colorName.toLowerCase()) {
            case "black": return Color.BLACK;
            case "blue": return Color.BLUE;
            case "green": return Color.GREEN;
            case "red": return Color.RED;
            case "purple": return new Color(128, 0, 128);
            default: return Color.BLACK;
        }
    }

    /**
     * Save user settings to JSON file
     */
    private void saveSettings(String name, String color, boolean emoji, int fontSize) {
        try {
            Map<String, Object> settings = new HashMap<>();
            settings.put("name", name);
            settings.put("color", color);
            settings.put("emoji", emoji);
            settings.put("font_size", fontSize);

            String json = gson.toJson(settings);

            try (FileWriter writer = new FileWriter(SAVE_FILE)) {
                writer.write(json);
            }
        } catch (IOException e) {
            System.err.println("Error saving settings: " + e.getMessage());
        }
    }

    /**
     * Load user settings from JSON file
     */
    private void loadSettings() {
        Path filePath = Paths.get(SAVE_FILE);
        if (Files.exists(filePath)) {
            try {
                String json = new String(Files.readAllBytes(filePath));
                @SuppressWarnings("unchecked")
                Map<String, Object> settings = gson.fromJson(json, Map.class);

                if (settings != null) {
                    // Load settings with default values
                    String name = (String) settings.getOrDefault("name", "");
                    String color = (String) settings.getOrDefault("color", "Black");
                    boolean emoji = settings.containsKey("emoji") ? (Boolean) settings.get("emoji") : false;

                    // Handle font_size as double from JSON
                    int fontSize = 12;
                    if (settings.containsKey("font_size")) {
                        Object fontSizeObj = settings.get("font_size");
                        if (fontSizeObj instanceof Double) {
                            fontSize = ((Double) fontSizeObj).intValue();
                        } else if (fontSizeObj instanceof Integer) {
                            fontSize = (Integer) fontSizeObj;
                        }
                    }

                    // Apply loaded settings to UI
                    nameInput.setText(name);
                    colorDropdown.setSelectedItem(color);
                    emojiCheck.setSelected(emoji);
                    fontSlider.setValue(fontSize);
                }
            } catch (IOException | JsonSyntaxException e) {
                System.err.println("Error loading settings: " + e.getMessage());
            }
        }
    }

    public static void main(String[] args) {
        // Set system look and feel
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            System.err.println("Could not set system look and feel: " + e.getMessage());
        }

        // Start application on Event Dispatch Thread
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                try {
                    InteractiveApp app = new InteractiveApp();
                    app.setVisible(true);
                } catch (Exception e) {
                    System.err.println("Error starting application: " + e.getMessage());
                    e.printStackTrace();
                }
            }
        });
    }
}
