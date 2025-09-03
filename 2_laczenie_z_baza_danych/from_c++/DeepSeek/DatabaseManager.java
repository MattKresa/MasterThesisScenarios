import java.sql.*;
import java.math.BigDecimal;

public class DatabaseManager {
    private static final String DB_URL = "jdbc:postgresql://localhost:5432/master_thesis";
    private static final String USER = "postgres";
    private static final String PASS = "9";

    public static void main(String[] args) {
        Connection conn = null;

        try {
            // Register JDBC driver and open connection
            conn = DriverManager.getConnection(DB_URL, USER, PASS);
            conn.setAutoCommit(false); // Enable transactions

            // Setup schema and insert data
            setupSchema(conn);
            insertSampleData(conn);

            // Query and process data
            queryAndProcess(conn);

        } catch (SQLException e) {
            System.err.println("Database error: " + e.getMessage());
            try {
                if (conn != null) conn.rollback();
            } catch (SQLException ex) {
                System.err.println("Rollback failed: " + ex.getMessage());
            }
        } finally {
            try {
                if (conn != null) conn.close();
            } catch (SQLException e) {
                System.err.println("Connection close failed: " + e.getMessage());
            }
        }
    }

    private static void setupSchema(Connection conn) throws SQLException {
        try (Statement stmt = conn.createStatement()) {
            stmt.executeUpdate("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                );
                """);

            stmt.executeUpdate("""
                CREATE TABLE IF NOT EXISTS orders (
                    id SERIAL PRIMARY KEY,
                    user_id INTEGER REFERENCES users(id),
                    product TEXT NOT NULL,
                    amount NUMERIC(10,2),
                    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                );
                """);
            conn.commit();
        } catch (SQLException e) {
            conn.rollback();
            throw e;
        }
    }

    private static void insertSampleData(Connection conn) throws SQLException {
        try (Statement stmt = conn.createStatement()) {
            // Insert users
            stmt.executeUpdate("""
                INSERT INTO users (name, email)
                VALUES 
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING;
                """);

            // Get Alicja's ID
            ResultSet rs = stmt.executeQuery("SELECT id FROM users WHERE email = 'alice@example.com'");
            if (!rs.next()) return;

            int user_id_alice = rs.getInt("id");

            // Check if orders already exist
            rs = stmt.executeQuery("SELECT COUNT(*) FROM orders WHERE user_id = " + user_id_alice);
            rs.next();
            long count = rs.getLong(1);

            if (count == 0) {
                try (PreparedStatement pstmt = conn.prepareStatement(
                        "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)")) {

                    pstmt.setInt(1, user_id_alice);
                    pstmt.setString(2, "Laptop");
                    pstmt.setBigDecimal(3, new BigDecimal("3200.00"));
                    pstmt.executeUpdate();

                    pstmt.setInt(1, user_id_alice);
                    pstmt.setString(2, "Mouse");
                    pstmt.setBigDecimal(3, new BigDecimal("120.00"));
                    pstmt.executeUpdate();

                    pstmt.setInt(1, user_id_alice);
                    pstmt.setString(2, "Keyboard");
                    pstmt.setBigDecimal(3, new BigDecimal("90.00"));
                    pstmt.executeUpdate();
                }
            }
            conn.commit();
        } catch (SQLException e) {
            conn.rollback();
            throw e;
        }
    }

    private static void queryAndProcess(Connection conn) throws SQLException {
        try (PreparedStatement pstmt = conn.prepareStatement("""
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

            pstmt.setBigDecimal(1, new BigDecimal("100.00"));
            ResultSet rs = pstmt.executeQuery();

            System.out.println("Orders over 100 zł:\n");
            while (rs.next()) {
                System.out.printf("%s (%s) ordered %s for %s zł on %s%n",
                        rs.getString("user_name"),
                        rs.getString("email"),
                        rs.getString("product"),
                        rs.getBigDecimal("amount"),
                        rs.getTimestamp("order_date"));
            }
            conn.commit();
        } catch (SQLException e) {
            conn.rollback();
            throw e;
        }
    }
}