import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
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

public class InteractiveApp extends JFrame {
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
    
    // For JSON serialization
    private Gson gson;
    
    public InteractiveApp() {
        gson = new Gson();
        initializeUI();
        loadSettings();
    }
    
    private void initializeUI() {
        setTitle("Interactive App");
        setBounds(300, 300, 400, 350);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        
        // Create components
        nameLabel = new JLabel("Enter your name:");
        nameInput = new JTextField();
        nameInput.setPreferredSize(new Dimension(200, 25));
        
        colorLabel = new JLabel("Choose a color:");
        colorDropdown = new JComboBox<>(new String[]{"Black", "Blue", "Green", "Red", "Purple"});
        
        emojiCheck = new JCheckBox("Add an emoji 😊");
        
        fontLabel = new JLabel("Select font size:");
        fontSlider = new JSlider(JSlider.HORIZONTAL, 10, 30, 12);
        fontSlider.setMajorTickSpacing(5);
        fontSlider.setMinorTickSpacing(1);
        fontSlider.setPaintTicks(true);
        fontSlider.setPaintLabels(true);
        
        button = new JButton("Show Message");
        button.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                showMessage();
            }
        });
        
        resultLabel = new JLabel("");
        resultLabel.setHorizontalAlignment(SwingConstants.CENTER);
        resultLabel.setPreferredSize(new Dimension(350, 40));
        
        // Layout
        setLayout(new BoxLayout(getContentPane(), BoxLayout.Y_AXIS));
        
        // Add components with padding
        add(Box.createVerticalStrut(10));
        add(createPanelWithComponent(nameLabel));
        add(createPanelWithComponent(nameInput));
        add(Box.createVerticalStrut(5));
        add(createPanelWithComponent(colorLabel));
        add(createPanelWithComponent(colorDropdown));
        add(Box.createVerticalStrut(5));
        add(createPanelWithComponent(emojiCheck));
        add(Box.createVerticalStrut(5));
        add(createPanelWithComponent(fontLabel));
        add(createPanelWithComponent(fontSlider));
        add(Box.createVerticalStrut(10));
        add(createPanelWithComponent(button));
        add(Box.createVerticalStrut(10));
        add(createPanelWithComponent(resultLabel));
        add(Box.createVerticalStrut(10));
    }
    
    private JPanel createPanelWithComponent(JComponent component) {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.CENTER));
        panel.add(component);
        return panel;
    }
    
    private void showMessage() {
        String name = nameInput.getText().trim();
        String color = (String) colorDropdown.getSelectedItem();
        boolean emoji = emojiCheck.isSelected();
        int fontSize = fontSlider.getValue();
        
        if (name.isEmpty()) {
            JOptionPane.showMessageDialog(this, "Please enter your name!", "Warning", JOptionPane.WARNING_MESSAGE);
            return;
        }
        
        String message = String.format("Hello, %s! Your favorite color is %s.", name, color);
        if (emoji) {
            message += " 😊";
        }
        
        resultLabel.setText(message);
        
        // Set color and font size
        Color textColor = getColorFromString(color);
        Font currentFont = resultLabel.getFont();
        Font newFont = new Font(currentFont.getName(), currentFont.getStyle(), fontSize);
        resultLabel.setFont(newFont);
        resultLabel.setForeground(textColor);
        
        saveSettings(name, color, emoji, fontSize);
    }
    
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
    
    private void saveSettings(String name, String color, boolean emoji, int fontSize) {
        try {
            Map<String, Object> data = new HashMap<>();
            data.put("name", name);
            data.put("color", color);
            data.put("emoji", emoji);
            data.put("font_size", fontSize);
            
            String json = gson.toJson(data);
            
            try (FileWriter writer = new FileWriter(SAVE_FILE)) {
                writer.write(json);
            }
        } catch (IOException e) {
            System.err.println("Failed to save settings: " + e.getMessage());
        }
    }
    
    private void loadSettings() {
        Path filePath = Paths.get(SAVE_FILE);
        if (Files.exists(filePath)) {
            try {
                String json = new String(Files.readAllBytes(filePath));
                @SuppressWarnings("unchecked")
                Map<String, Object> data = gson.fromJson(json, Map.class);
                
                if (data != null) {
                    String name = (String) data.getOrDefault("name", "");
                    String color = (String) data.getOrDefault("color", "Black");
                    boolean emoji = data.containsKey("emoji") ? (Boolean) data.get("emoji") : false;
                    double fontSizeDouble = data.containsKey("font_size") ? (Double) data.get("font_size") : 12.0;
                    int fontSize = (int) fontSizeDouble;
                    
                    nameInput.setText(name);
                    colorDropdown.setSelectedItem(color);
                    emojiCheck.setSelected(emoji);
                    fontSlider.setValue(fontSize);
                }
            } catch (IOException | JsonSyntaxException e) {
                System.err.println("Failed to load settings: " + e.getMessage());
            }
        }
    }
    
    public static void main(String[] args) {
        // Set look and feel to system default
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeel());
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                new InteractiveApp().setVisible(true);
            }
        });
    }
}