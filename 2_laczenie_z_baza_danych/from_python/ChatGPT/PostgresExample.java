import java.sql.*;

public class PostgresExample {

    private static Connection connectToDb() throws SQLException {
        String url = "jdbc:postgresql://localhost:5432/master_thesis";
        String user = "postgres";
        String password = "9";
        return DriverManager.getConnection(url, user, password);
    }

    private static void setupSchema(Connection conn) throws SQLException {
        try (Statement stmt = conn.createStatement()) {
            stmt.executeUpdate("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                )
            """);

            stmt.executeUpdate("""
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
        try (Statement stmt = conn.createStatement()) {
            stmt.executeUpdate("""
                INSERT INTO users (name, email)
                VALUES
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING
            """);
        }

        int userIdAlice;
        try (PreparedStatement ps = conn.prepareStatement(
                "SELECT id FROM users WHERE email = 'alice@example.com'")) {
            ResultSet rs = ps.executeQuery();
            if (!rs.next()) return;
            userIdAlice = rs.getInt("id");
        }

        int orderCount;
        try (PreparedStatement ps = conn.prepareStatement(
                "SELECT COUNT(*) FROM orders WHERE user_id = ?")) {
            ps.setInt(1, userIdAlice);
            ResultSet rs = ps.executeQuery();
            rs.next();
            orderCount = rs.getInt(1);
        }

        if (orderCount == 0) {
            try (PreparedStatement ps = conn.prepareStatement(
                    "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)")) {
                ps.setInt(1, userIdAlice);
                ps.setString(2, "Laptop");
                ps.setDouble(3, 3200.00);
                ps.addBatch();

                ps.setInt(1, userIdAlice);
                ps.setString(2, "Mouse");
                ps.setDouble(3, 120.00);
                ps.addBatch();

                ps.setInt(1, userIdAlice);
                ps.setString(2, "Keyboard");
                ps.setDouble(3, 90.00);
                ps.addBatch();

                ps.executeBatch();
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
            ps.setDouble(1, 100.0);
            ResultSet rs = ps.executeQuery();

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
