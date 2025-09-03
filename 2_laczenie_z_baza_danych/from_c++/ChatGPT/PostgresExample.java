import java.sql.*;

public class PostgresExample {

    public static void setupSchema(Connection conn) throws SQLException {
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

    public static void insertSampleData(Connection conn) throws SQLException {
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

        // Get Alicja's ID
        int userIdAlice = -1;
        try (PreparedStatement ps = conn.prepareStatement(
                "SELECT id FROM users WHERE email = ?")) {
            ps.setString(1, "alice@example.com");
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    userIdAlice = rs.getInt("id");
                }
            }
        }
        if (userIdAlice == -1) return;

        // Check if orders exist
        int count = 0;
        try (PreparedStatement ps = conn.prepareStatement(
                "SELECT COUNT(*) FROM orders WHERE user_id = ?")) {
            ps.setInt(1, userIdAlice);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    count = rs.getInt(1);
                }
            }
        }

        if (count == 0) {
            try (PreparedStatement ps = conn.prepareStatement(
                    "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)")) {
                ps.setInt(1, userIdAlice);
                ps.setString(2, "Laptop");
                ps.setBigDecimal(3, new java.math.BigDecimal("3200.00"));
                ps.executeUpdate();

                ps.setInt(1, userIdAlice);
                ps.setString(2, "Mouse");
                ps.setBigDecimal(3, new java.math.BigDecimal("120.00"));
                ps.executeUpdate();

                ps.setInt(1, userIdAlice);
                ps.setString(2, "Keyboard");
                ps.setBigDecimal(3, new java.math.BigDecimal("90.00"));
                ps.executeUpdate();
            }
        }
    }

    public static void queryAndProcess(Connection conn) throws SQLException {
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

        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setBigDecimal(1, new java.math.BigDecimal("100.00"));

            try (ResultSet rs = ps.executeQuery()) {
                System.out.println("Orders over 100 zł:\n");
                while (rs.next()) {
                    String userName = rs.getString("user_name");
                    String email = rs.getString("email");
                    String product = rs.getString("product");
                    String amount = rs.getString("amount");
                    String orderDate = rs.getString("order_date");

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
            conn.setAutoCommit(false);

            setupSchema(conn);
            insertSampleData(conn);
            conn.commit();

            queryAndProcess(conn);
            conn.commit();

        } catch (SQLException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
