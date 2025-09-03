import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Timestamp;
import java.text.SimpleDateFormat;

public class Main {

    // JDBC URL for PostgreSQL connection
    private static final String DB_URL = "jdbc:postgresql://localhost:5432/master_thesis";
    private static final String DB_USER = "postgres";
    private static final String DB_PASSWORD = "9";

    /**
     * Establishes a connection to the PostgreSQL database.
     * @return A Connection object.
     * @throws SQLException if a database access error occurs.
     */
    public static Connection connectToDb() throws SQLException {
        // DriverManager.getConnection attempts to establish a connection to the given database URL.
        // The PostgreSQL JDBC driver will automatically be loaded if it's in the classpath.
        return DriverManager.getConnection(DB_URL, DB_USER, DB_PASSWORD);
    }

    /**
     * Sets up the database schema by creating 'users' and 'orders' tables if they don't exist.
     * @param conn The database connection.
     * @throws SQLException if a database access error occurs.
     */
    public static void setupSchema(Connection conn) throws SQLException {
        // try-with-resources ensures that the Statement object is automatically closed.
        try (Statement stmt = conn.createStatement()) {
            // SQL to create the 'users' table.
            // SERIAL PRIMARY KEY: auto-increments and serves as primary key.
            // TEXT NOT NULL: string type, cannot be null.
            // TEXT UNIQUE NOT NULL: string type, must be unique, cannot be null.
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                );
            """);

            // SQL to create the 'orders' table.
            // INTEGER REFERENCES users(id): foreign key constraint linking to the 'users' table.
            // NUMERIC(10,2): fixed-point number with 10 total digits and 2 digits after the decimal.
            // TIMESTAMP DEFAULT CURRENT_TIMESTAMP: stores date and time, defaults to current time on insert.
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
     * Inserts sample data into the 'users' and 'orders' tables.
     * @param conn The database connection.
     * @throws SQLException if a database access error occurs.
     */
    public static void insertSampleData(Connection conn) throws SQLException {
        // In JDBC, auto-commit is true by default. For multiple related inserts,
        // it's often better to manage transactions explicitly for atomicity.
        // However, to match the Python's `conn.autocommit = True`, we'll rely on default behavior
        // or explicitly set it to true if it was changed elsewhere.
        // For the ON CONFLICT clause, we'll use a Statement.

        try (Statement stmt = conn.createStatement()) {
            // Insert sample users.
            // ON CONFLICT (email) DO NOTHING: If a user with the same email already exists,
            // do not insert and do not throw an error.
            stmt.execute("""
                INSERT INTO users (name, email)
                VALUES
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING;
            """);
            System.out.println("Sample users inserted.");
        }

        // Fetch the user_id for 'alice@example.com'.
        int userIdAlice = -1; // Initialize with an invalid ID
        try (PreparedStatement pstmt = conn.prepareStatement("SELECT id FROM users WHERE email = ?")) {
            pstmt.setString(1, "alice@example.com");
            try (ResultSet rs = pstmt.executeQuery()) {
                if (rs.next()) {
                    userIdAlice = rs.getInt("id");
                }
            }
        }

        if (userIdAlice == -1) {
            System.err.println("Error: Alice's user ID not found. Cannot insert orders.");
            return;
        }

        // Check if orders for Alice already exist to prevent duplicate insertions on re-run.
        boolean ordersExist = false;
        try (PreparedStatement pstmt = conn.prepareStatement("SELECT COUNT(*) AS order_count FROM orders WHERE user_id = ?")) {
            pstmt.setInt(1, userIdAlice);
            try (ResultSet rs = pstmt.executeQuery()) {
                if (rs.next() && rs.getInt("order_count") > 0) {
                    ordersExist = true;
                }
            }
        }

        if (!ordersExist) {
            // Insert orders for Alice using PreparedStatement for parameterized queries.
            // This is safer and more efficient than concatenating strings.
            String insertOrderSql = "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)";
            try (PreparedStatement pstmt = conn.prepareStatement(insertOrderSql)) {
                // Add batch for Laptop
                pstmt.setInt(1, userIdAlice);
                pstmt.setString(2, "Laptop");
                pstmt.setDouble(3, 3200.00);
                pstmt.addBatch(); // Add to batch

                // Add batch for Mouse
                pstmt.setInt(1, userIdAlice);
                pstmt.setString(2, "Mouse");
                pstmt.setDouble(3, 120.00);
                pstmt.addBatch(); // Add to batch

                // Add batch for Keyboard
                pstmt.setInt(1, userIdAlice);
                pstmt.setString(2, "Keyboard");
                pstmt.setDouble(3, 90.00);
                pstmt.addBatch(); // Add to batch

                // Execute the batch inserts
                int[] updateCounts = pstmt.executeBatch();
                System.out.println("Inserted " + updateCounts.length + " sample orders for Alice.");
            }
        } else {
            System.out.println("Orders for Alice already exist, skipping insertion.");
        }
        System.out.println("Sample data insertion complete.");
    }

    /**
     * Queries and processes data from the database, specifically orders over 100 zł.
     * @param conn The database connection.
     * @throws SQLException if a database access error occurs.
     */
    public static void queryAndProcess(Connection conn) throws SQLException {
        // SQL query to get orders over a certain amount, joining 'orders' and 'users' tables.
        // Use '?' as a placeholder for parameters in PreparedStatement.
        String sql = """
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

        System.out.println("\nOrders over 100 zl:\n");

        // try-with-resources ensures PreparedStatement and ResultSet are closed.
        try (PreparedStatement pstmt = conn.prepareStatement(sql)) {
            pstmt.setDouble(1, 100.0); // Set the parameter for the WHERE clause

            try (ResultSet rs = pstmt.executeQuery()) {
                // Iterate through the rows returned by the query.
                while (rs.next()) {
                    // Retrieve column values by their alias or column name.
                    String userName = rs.getString("user_name");
                    String email = rs.getString("email");
                    String product = rs.getString("product");
                    double amount = rs.getDouble("amount");
                    Timestamp orderDate = rs.getTimestamp("order_date");

                    // Format the timestamp for display.
                    SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                    String formattedDate = dateFormat.format(orderDate);

                    // Print the formatted output.
                    System.out.printf("%s (%s) ordered %s for %.2f zł on %s%n",
                            userName, email, product, amount, formattedDate);
                }
            }
        }
    }

    /**
     * Main method to run the database operations.
     * Handles connection, schema setup, data insertion, and querying.
     */
    public static void main(String[] args) {
        Connection conn = null; // Declare connection outside try-catch for finally block access
        try {
            // Establish connection
            conn = connectToDb();
            System.out.println("Successfully connected to the database.");

            // JDBC connections are auto-commit by default.
            // If you need explicit transaction control (commit/rollback), set:
            // conn.setAutoCommit(false);
            // And then call conn.commit() or conn.rollback() as needed.

            // Perform database operations
            setupSchema(conn);
            insertSampleData(conn);
            queryAndProcess(conn);

        } catch (SQLException e) {
            // Catch and print any SQL exceptions that occur.
            System.err.println("Database Error: " + e.getMessage());
            // Optionally, print the SQL state and error code for more detail.
            // System.err.println("SQL State: " + e.getSQLState());
            // System.err.println("Error Code: " + e.getErrorCode());
        } catch (Exception e) {
            // Catch any other general exceptions.
            System.err.println("General Error: " + e.getMessage());
        } finally {
            // Ensure the connection is closed in the finally block to release resources.
            if (conn != null) {
                try {
                    conn.close();
                    System.out.println("Database connection closed.");
                } catch (SQLException e) {
                    System.err.println("Error closing connection: " + e.getMessage());
                }
            }
        }
    }
}
