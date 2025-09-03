import java.sql.*;
import java.util.ArrayList;
import java.util.List;

public class Main {

    private static Connection connectToDb() throws SQLException {
        String url = "jdbc:postgresql://localhost:5432/master_thesis";
        String user = "postgres";
        String password = "9";
        return DriverManager.getConnection(url, user, password);
    }

    private static void setupSchema(Connection conn) throws SQLException {
        try (Statement stmt = conn.createStatement()) {
            // Create users table
            stmt.executeUpdate("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                );
                """);

            // Create orders table
            stmt.executeUpdate("""
                CREATE TABLE IF NOT EXISTS orders (
                    id SERIAL PRIMARY KEY,
                    user_id INTEGER REFERENCES users(id),
                    product TEXT NOT NULL,
                    amount NUMERIC(10,2),
                    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                );
                """);
        }
    }

    private static void insertSampleData(Connection conn) throws SQLException {
        // Add users (with ON CONFLICT DO NOTHING)
        try (Statement stmt = conn.createStatement()) {
            stmt.executeUpdate("""
                INSERT INTO users (name, email)
                VALUES 
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING;
                """);
        }

        // Get Alice's user ID
        int userIdAlice;
        try (PreparedStatement stmt = conn.prepareStatement(
                "SELECT id FROM users WHERE email = ?")) {
            stmt.setString(1, "alice@example.com");
            ResultSet rs = stmt.executeQuery();
            if (!rs.next()) {
                throw new SQLException("Alice not found in database");
            }
            userIdAlice = rs.getInt("id");
        }

        // Check if Alice has any orders
        int orderCount;
        try (PreparedStatement stmt = conn.prepareStatement(
                "SELECT COUNT(*) AS count FROM orders WHERE user_id = ?")) {
            stmt.setInt(1, userIdAlice);
            ResultSet rs = stmt.executeQuery();
            rs.next();
            orderCount = rs.getInt("count");
        }

        if (orderCount == 0) {
            // Insert multiple orders
            List<Object[]> orders = new ArrayList<>();
            orders.add(new Object[]{userIdAlice, "Laptop", 3200.00});
            orders.add(new Object[]{userIdAlice, "Mouse", 120.00});
            orders.add(new Object[]{userIdAlice, "Keyboard", 90.00});

            try (PreparedStatement stmt = conn.prepareStatement(
                    "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)")) {
                for (Object[] order : orders) {
                    stmt.setInt(1, (Integer) order[0]);
                    stmt.setString(2, (String) order[1]);
                    stmt.setDouble(3, (Double) order[2]);
                    stmt.addBatch();
                }
                stmt.executeBatch();
            }
        }
    }

    private static void queryAndProcess(Connection conn) throws SQLException {
        // Get orders over 100zl
        try (PreparedStatement stmt = conn.prepareStatement("""
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
                """)) {
            stmt.setDouble(1, 100.0);
            ResultSet rs = stmt.executeQuery();

            System.out.println("Orders over 100 zl:\n");
            while (rs.next()) {
                String userName = rs.getString("user_name");
                String email = rs.getString("email");
                String product = rs.getString("product");
                double amount = rs.getDouble("amount");
                Timestamp orderDate = rs.getTimestamp("order_date");

                System.out.printf("%s (%s) ordered %s for %.2f zł on %s%n",
                        userName, email, product, amount, orderDate);
            }
        }
    }

    public static void main(String[] args) {
        Connection conn = null;
        try {
            // Load PostgreSQL JDBC driver
            Class.forName("org.postgresql.Driver");

            conn = connectToDb();
            conn.setAutoCommit(true); // Equivalent to autocommit=True

            setupSchema(conn);
            insertSampleData(conn);
            queryAndProcess(conn);

        } catch (ClassNotFoundException e) {
            System.err.println("PostgreSQL JDBC driver not found");
            e.printStackTrace();
        } catch (SQLException e) {
            System.err.println("Database error: " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (conn != null) {
                try {
                    conn.close();
                } catch (SQLException e) {
                    System.err.println("Error closing connection: " + e.getMessage());
                }
            }
        }
    }
}