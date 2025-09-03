import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.math.BigDecimal; // For NUMERIC type

public class DatabaseManager {

    // Database connection details
    private static final String DB_URL = "jdbc:postgresql://localhost:5432/master_thesis";
    private static final String USER = "postgres";
    private static final String PASSWORD = "9";

    /**
     * Sets up the database schema by creating 'users' and 'orders' tables
     * if they do not already exist.
     *
     * @param conn The database connection.
     * @throws SQLException If a database access error occurs.
     */
    public static void setupSchema(Connection conn) throws SQLException {
        System.out.println("Setting up database schema...");
        try (Statement stmt = conn.createStatement()) {
            // Create users table
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                );
            """);

            // Create orders table
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS orders (
                    id SERIAL PRIMARY KEY,
                    user_id INTEGER REFERENCES users(id),
                    product TEXT NOT NULL,
                    amount NUMERIC(10,2),
                    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                );
            """);
            System.out.println("Schema setup complete.");
        }
    }

    /**
     * Inserts sample user and order data into the database.
     * It checks for existing users/orders to prevent duplicate entries.
     *
     * @param conn The database connection.
     * @throws SQLException If a database access error occurs.
     */
    public static void insertSampleData(Connection conn) throws SQLException {
        System.out.println("Inserting sample data...");

        // Insert users, handling conflicts on email
        // Using Statement for ON CONFLICT as it's a simple bulk insert
        try (Statement stmt = conn.createStatement()) {
            stmt.execute("""
                INSERT INTO users (name, email)
                VALUES
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING;
            """);
        }

        // Get Alicja's user ID
        int userIdAlice = -1;
        String selectAlicjaIdSql = "SELECT id FROM users WHERE email = ?";
        try (PreparedStatement pstmt = conn.prepareStatement(selectAlicjaIdSql)) {
            pstmt.setString(1, "alice@example.com");
            try (ResultSet rs = pstmt.executeQuery()) {
                if (rs.next()) {
                    userIdAlice = rs.getInt("id");
                    System.out.println("Alicja's user ID: " + userIdAlice);
                } else {
                    System.err.println("Error: Alicja's user ID not found after insertion.");
                    return; // Exit if Alicja's ID is not found
                }
            }
        }

        // Check if Alicja already has orders
        String countOrdersSql = "SELECT COUNT(*) FROM orders WHERE user_id = ?";
        try (PreparedStatement pstmt = conn.prepareStatement(countOrdersSql)) {
            pstmt.setInt(1, userIdAlice);
            try (ResultSet rs = pstmt.executeQuery()) {
                if (rs.next()) {
                    long orderCount = rs.getLong(1);
                    if (orderCount == 0) {
                        System.out.println("Inserting orders for Alicja...");
                        String insertOrderSql = "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)";
                        try (PreparedStatement insertPstmt = conn.prepareStatement(insertOrderSql)) {
                            insertPstmt.setInt(1, userIdAlice);

                            insertPstmt.setString(2, "Laptop");
                            insertPstmt.setBigDecimal(3, new BigDecimal("3200.00"));
                            insertPstmt.addBatch(); // Add to batch

                            insertPstmt.setString(2, "Mouse");
                            insertPstmt.setBigDecimal(3, new BigDecimal("120.00"));
                            insertPstmt.addBatch(); // Add to batch

                            insertPstmt.setString(2, "Keyboard");
                            insertPstmt.setBigDecimal(3, new BigDecimal("90.00"));
                            insertPstmt.addBatch(); // Add to batch

                            insertPstmt.executeBatch(); // Execute all batched inserts
                            System.out.println("Orders inserted for Alicja.");
                        }
                    } else {
                        System.out.println("Alicja already has " + orderCount + " orders. Skipping order insertion.");
                    }
                }
            }
        }
        System.out.println("Sample data insertion complete.");
    }

    /**
     * Queries the database for orders with an amount greater than a specified value
     * and prints the results.
     *
     * @param conn The database connection.
     * @throws SQLException If a database access error occurs.
     */
    public static void queryAndProcess(Connection conn) throws SQLException {
        System.out.println("\nQuerying and processing data...");
        String querySql = """
            SELECT
                u.name AS user_name,
                u.email,
                o.product,
                o.amount,
                o.order_date
            FROM orders o
            JOIN users u ON o.user_id = u.id
            WHERE o.amount > ?
            ORDER BY o.amount DESC;
        """;

        try (PreparedStatement pstmt = conn.prepareStatement(querySql)) {
            pstmt.setBigDecimal(1, new BigDecimal("100.00")); // Set the amount parameter

            try (ResultSet rs = pstmt.executeQuery()) {
                System.out.println("Orders over 100 zł:\n");
                boolean foundOrders = false;
                while (rs.next()) {
                    foundOrders = true;
                    String userName = rs.getString("user_name");
                    String email = rs.getString("email");
                    String product = rs.getString("product");
                    BigDecimal amount = rs.getBigDecimal("amount");
                    java.sql.Timestamp orderDate = rs.getTimestamp("order_date");

                    System.out.printf("%s (%s) ordered %s for %.2f zł on %s%n",
                            userName, email, product, amount.doubleValue(), orderDate);
                }
                if (!foundOrders) {
                    System.out.println("No orders found matching the criteria.");
                }
            }
        }
        System.out.println("\nQuery and processing complete.");
    }

    public static void main(String[] args) {
        Connection conn = null;
        try {
            // Register the JDBC driver (optional for modern JDBC versions, but good practice)
            // Class.forName("org.postgresql.Driver"); // No longer strictly needed for JDBC 4.0+

            // Establish connection to PostgreSQL
            conn = DriverManager.getConnection(DB_URL, USER, PASSWORD);
            conn.setAutoCommit(false); // Disable auto-commit to manage transactions manually

            // First transaction: Schema setup and data insertion
            setupSchema(conn);
            insertSampleData(conn);
            conn.commit(); // Commit the changes for schema and data insertion
            System.out.println("First transaction committed (schema and data).");

            // Second transaction: Query and process data
            // For read-only operations, a new transaction isn't strictly necessary
            // but it mirrors the C++ example's `tx2(conn)`
            queryAndProcess(conn);
            conn.commit(); // Commit the read transaction (no changes, but good practice)
            System.out.println("Second transaction committed (query).");

        } catch (SQLException e) {
            System.err.println("Database error: " + e.getMessage());
            if (conn != null) {
                try {
                    conn.rollback(); // Rollback any changes on error
                    System.err.println("Transaction rolled back due to error.");
                } catch (SQLException rollbackEx) {
                    System.err.println("Error during rollback: " + rollbackEx.getMessage());
                }
            }
        } catch (Exception e) {
            System.err.println("An unexpected error occurred: " + e.getMessage());
        } finally {
            if (conn != null) {
                try {
                    conn.close(); // Close the connection
                    System.out.println("Database connection closed.");
                } catch (SQLException e) {
                    System.err.println("Error closing connection: " + e.getMessage());
                }
            }
        }
    }
}
