import java.sql.*;

public class DatabaseManager {

    private static Connection connectToDb() throws SQLException {
        String url = "jdbc:postgresql://localhost:5432/master_thesis";
        String user = "postgres";
        String password = "9";
        return DriverManager.getConnection(url, user, password);
    }

    private static void setupSchema(Connection conn) throws SQLException {
        try (Statement stmt = conn.createStatement()) {
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                )
            """);

            stmt.execute("""
                CREATE TABLE IF NOT EXISTS orders (
                    id SERIAL PRIMARY KEY,
                    user_id INTEGER REFERENCES users(id),
                    product TEXT NOT NULL,
                    amount NUMERIC(10,2),
                    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
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
                ON CONFLICT (email) DO NOTHING
            """);
        }

        int userIdAlice = -1;
        try (PreparedStatement ps = conn.prepareStatement("SELECT id FROM users WHERE email = 'alice@example.com'")) {
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    userIdAlice = rs.getInt("id");
                }
            }
        }

        if (userIdAlice == -1) return;

        long countOrders = 0;
        try (PreparedStatement ps = conn.prepareStatement("SELECT COUNT(*) FROM orders WHERE user_id = ?")) {
            ps.setInt(1, userIdAlice);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    countOrders = rs.getLong(1);
                }
            }
        }

        if (countOrders == 0) {
            try (PreparedStatement ps = conn.prepareStatement(
                    "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)")) {
                ps.setInt(1, userIdAlice);

                ps.setString(2, "Laptop");
                ps.setBigDecimal(3, new java.math.BigDecimal("3200.00"));
                ps.executeUpdate();

                ps.setString(2, "Mouse");
                ps.setBigDecimal(3, new java.math.BigDecimal("120.00"));
                ps.executeUpdate();

                ps.setString(2, "Keyboard");
                ps.setBigDecimal(3, new java.math.BigDecimal("90.00"));
                ps.executeUpdate();
            }
        }
    }

    private static void queryAndProcess(Connection conn) throws SQLException {
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
            ORDER BY o.amount DESC
        """;

        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setBigDecimal(1, new java.math.BigDecimal("100.00"));

            try (ResultSet rs = ps.executeQuery()) {
                System.out.println("Orders over 100 zl:\n");
                while (rs.next()) {
                    String name = rs.getString("user_name");
                    String email = rs.getString("email");
                    String product = rs.getString("product");
                    String amount = rs.getBigDecimal("amount").toPlainString();
                    Timestamp date = rs.getTimestamp("order_date");

                    System.out.printf("%s (%s) ordered %s for %s zł on %s%n",
                            name, email, product, amount, date.toString());
                }
            }
        }
    }

    public static void main(String[] args) {
        try (Connection conn = connectToDb()) {
            conn.setAutoCommit(true);

            setupSchema(conn);
            insertSampleData(conn);
            queryAndProcess(conn);

        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        }
    }
}
