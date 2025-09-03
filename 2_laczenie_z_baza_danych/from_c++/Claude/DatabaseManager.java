import java.sql.*;
import java.math.BigDecimal;

public class DatabaseManager {

    private static void setupSchema(Connection conn) throws SQLException {
        try (Statement stmt = conn.createStatement()) {
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                );
            """);

            stmt.execute("""
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
        // Insert users
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

        // Get Alice's ID
        int userIdAlice = -1;
        try (PreparedStatement pstmt = conn.prepareStatement(
                "SELECT id FROM users WHERE email = ?")) {
            pstmt.setString(1, "alice@example.com");
            try (ResultSet rs = pstmt.executeQuery()) {
                if (!rs.next()) {
                    return;
                }
                userIdAlice = rs.getInt("id");
            }
        }

        // Check if orders already exist
        try (PreparedStatement pstmt = conn.prepareStatement(
                "SELECT COUNT(*) FROM orders WHERE user_id = ?")) {
            pstmt.setInt(1, userIdAlice);
            try (ResultSet rs = pstmt.executeQuery()) {
                rs.next();
                long count = rs.getLong(1);

                if (count == 0) {
                    // Insert sample orders
                    try (PreparedStatement insertStmt = conn.prepareStatement(
                            "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)")) {

                        // Laptop order
                        insertStmt.setInt(1, userIdAlice);
                        insertStmt.setString(2, "Laptop");
                        insertStmt.setBigDecimal(3, new BigDecimal("3200.00"));
                        insertStmt.executeUpdate();

                        // Mouse order
                        insertStmt.setInt(1, userIdAlice);
                        insertStmt.setString(2, "Mouse");
                        insertStmt.setBigDecimal(3, new BigDecimal("120.00"));
                        insertStmt.executeUpdate();

                        // Keyboard order
                        insertStmt.setInt(1, userIdAlice);
                        insertStmt.setString(2, "Keyboard");
                        insertStmt.setBigDecimal(3, new BigDecimal("90.00"));
                        insertStmt.executeUpdate();
                    }
                }
            }
        }
    }

    private static void queryAndProcess(Connection conn) throws SQLException {
        String query = """
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

        try (PreparedStatement pstmt = conn.prepareStatement(query)) {
            pstmt.setBigDecimal(1, new BigDecimal("100.0"));

            try (ResultSet rs = pstmt.executeQuery()) {
                System.out.println("Orders over 100 zl:\n");

                while (rs.next()) {
                    String userName = rs.getString("user_name");
                    String email = rs.getString("email");
                    String product = rs.getString("product");
                    BigDecimal amount = rs.getBigDecimal("amount");
                    Timestamp orderDate = rs.getTimestamp("order_date");

                    System.out.printf("%s (%s) ordered %s for %s zł on %s%n",
                            userName, email, product, amount, orderDate);
                }
            }
        }
    }

    public static void main(String[] args) {
        String url = "jdbc:postgresql://localhost:5432/master_thesis";
        String user = "postgres";
        String password = "9";

        try (Connection conn = DriverManager.getConnection(url, user, password)) {
            if (conn == null || conn.isClosed()) {
                System.err.println("Cannot connect to database!");
                System.exit(1);
            }

            // First transaction: setup schema and insert data
            conn.setAutoCommit(false);
            try {
                setupSchema(conn);
                insertSampleData(conn);
                conn.commit();
            } catch (SQLException e) {
                conn.rollback();
                throw e;
            }

            // Second transaction: query and process
            try {
                queryAndProcess(conn);
                conn.commit();
            } catch (SQLException e) {
                conn.rollback();
                throw e;
            }

        } catch (SQLException e) {
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        }
    }
}