import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.io.*;
import org.json.JSONObject;
import org.json.JSONTokener;

public class InteractiveApp extends JFrame {

    private JLabel nameLabel;
    private JTextField nameInput;
    private JLabel colorLabel;
    private JComboBox<String> colorDropdown;
    private JCheckBox emojiCheck;
    private JLabel fontLabel;
    private JSlider fontSlider;
    private JButton button;
    private JLabel resultLabel;

    private final String SAVE_FILE = "user_settings.json";

    public InteractiveApp() {
        setTitle("Interactive App");
        setSize(400, 300);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        // Initialize components
        nameLabel = new JLabel("Enter your name:");
        nameInput = new JTextField();

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
        resultLabel = new JLabel("", SwingConstants.CENTER);

        // Set up layout
        JPanel panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
        panel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));

        panel.add(nameLabel);
        panel.add(nameInput);
        panel.add(Box.createRigidArea(new Dimension(0, 5)));
        panel.add(colorLabel);
        panel.add(colorDropdown);
        panel.add(Box.createRigidArea(new Dimension(0, 5)));
        panel.add(emojiCheck);
        panel.add(Box.createRigidArea(new Dimension(0, 5)));
        panel.add(fontLabel);
        panel.add(fontSlider);
        panel.add(Box.createRigidArea(new Dimension(0, 10)));
        panel.add(button);
        panel.add(Box.createRigidArea(new Dimension(0, 10)));
        panel.add(resultLabel);

        add(panel);

        // Add action listener
        button.addActionListener(this::showMessage);

        // Load settings
        loadSettings();
    }

    private void showMessage(ActionEvent e) {
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
        resultLabel.setForeground(getColorFromString(color));
        resultLabel.setFont(new Font(resultLabel.getFont().getName(), Font.PLAIN, fontSize));

        saveSettings(name, color, emoji, fontSize);
    }

    private Color getColorFromString(String color) {
        switch (color.toLowerCase()) {
            case "blue": return Color.BLUE;
            case "green": return Color.GREEN;
            case "red": return Color.RED;
            case "purple": return new Color(128, 0, 128);
            default: return Color.BLACK;
        }
    }

    private void saveSettings(String name, String color, boolean emoji, int fontSize) {
        JSONObject obj = new JSONObject();
        obj.put("name", name);
        obj.put("color", color);
        obj.put("emoji", emoji);
        obj.put("font_size", fontSize);

        try (FileWriter file = new FileWriter(SAVE_FILE)) {
            file.write(obj.toString());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void loadSettings() {
        File file = new File(SAVE_FILE);
        if (file.exists()) {
            try (FileReader reader = new FileReader(file)) {
                JSONTokener tokener = new JSONTokener(reader);
                JSONObject obj = new JSONObject(tokener);

                nameInput.setText(obj.optString("name", ""));
                colorDropdown.setSelectedItem(obj.optString("color", "Black"));
                emojiCheck.setSelected(obj.optBoolean("emoji", false));
                fontSlider.setValue(obj.optInt("font_size", 12));
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            InteractiveApp app = new InteractiveApp();
            app.setVisible(true);
        });
    }
}