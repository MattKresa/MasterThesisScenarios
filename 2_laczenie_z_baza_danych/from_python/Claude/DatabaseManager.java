import java.sql.*;
import java.math.BigDecimal;
import java.util.List;

public class DatabaseManager {
    private static final String URL = "jdbc:postgresql://localhost:5432/master_thesis";
    private static final String USER = "postgres";
    private static final String PASSWORD = "9";

    private Connection connection;

    // Connect to database
    public Connection connectToDb() throws SQLException {
        try {
            Class.forName("org.postgresql.Driver");
            connection = DriverManager.getConnection(URL, USER, PASSWORD);
            connection.setAutoCommit(true);
            return connection;
        } catch (ClassNotFoundException e) {
            throw new SQLException("PostgreSQL JDBC Driver not found", e);
        }
    }

    // Setup database schema
    public void setupSchema() throws SQLException {
        String createUsersTable = """
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                name TEXT NOT NULL,
                email TEXT UNIQUE NOT NULL
            );
            """;

        String createOrdersTable = """
            CREATE TABLE IF NOT EXISTS orders (
                id SERIAL PRIMARY KEY,
                user_id INTEGER REFERENCES users(id),
                product TEXT NOT NULL,
                amount NUMERIC(10,2),
                order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
            """;

        try (Statement stmt = connection.createStatement()) {
            stmt.execute(createUsersTable);
            stmt.execute(createOrdersTable);
        }
    }

    // Insert sample data
    public void insertSampleData() throws SQLException {
        // Add users
        String insertUsers = """
            INSERT INTO users (name, email)
            VALUES 
                ('Alicja', 'alice@example.com'),
                ('Bartek', 'bartek@example.com'),
                ('Celina', 'celina@example.com')
            ON CONFLICT (email) DO NOTHING;
            """;

        try (Statement stmt = connection.createStatement()) {
            stmt.execute(insertUsers);
        }

        // Get Alice's user ID
        String getUserId = "SELECT id FROM users WHERE email = ?";
        int userIdAlice;

        try (PreparedStatement pstmt = connection.prepareStatement(getUserId)) {
            pstmt.setString(1, "alice@example.com");
            try (ResultSet rs = pstmt.executeQuery()) {
                if (rs.next()) {
                    userIdAlice = rs.getInt("id");
                } else {
                    throw new SQLException("Alice not found in database");
                }
            }
        }

        // Check if Alice already has orders
        String countOrders = "SELECT COUNT(*) as count FROM orders WHERE user_id = ?";
        int orderCount;

        try (PreparedStatement pstmt = connection.prepareStatement(countOrders)) {
            pstmt.setInt(1, userIdAlice);
            try (ResultSet rs = pstmt.executeQuery()) {
                rs.next();
                orderCount = rs.getInt("count");
            }
        }

        // Insert orders if Alice has none
        if (orderCount == 0) {
            String insertOrder = "INSERT INTO orders (user_id, product, amount) VALUES (?, ?, ?)";

            // Order data
            List<OrderData> orders = List.of(
                    new OrderData(userIdAlice, "Laptop", new BigDecimal("3200.00")),
                    new OrderData(userIdAlice, "Mouse", new BigDecimal("120.00")),
                    new OrderData(userIdAlice, "Keyboard", new BigDecimal("90.00"))
            );

            try (PreparedStatement pstmt = connection.prepareStatement(insertOrder)) {
                for (OrderData order : orders) {
                    pstmt.setInt(1, order.userId);
                    pstmt.setString(2, order.product);
                    pstmt.setBigDecimal(3, order.amount);
                    pstmt.executeUpdate();
                }
            }
        }
    }

    // Query and process results
    public void queryAndProcess() throws SQLException {
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

        try (PreparedStatement pstmt = connection.prepareStatement(query)) {
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

    // Close connection
    public void close() {
        if (connection != null) {
            try {
                connection.close();
            } catch (SQLException e) {
                System.err.println("Error closing connection: " + e.getMessage());
            }
        }
    }

    // Helper class for order data
    private static class OrderData {
        final int userId;
        final String product;
        final BigDecimal amount;

        OrderData(int userId, String product, BigDecimal amount) {
            this.userId = userId;
            this.product = product;
            this.amount = amount;
        }
    }

    public static void main(String[] args) {
        DatabaseManager db = new DatabaseManager();

        try {
            db.connectToDb();
            db.setupSchema();
            db.insertSampleData();
            db.queryAndProcess();

        } catch (SQLException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        } finally {
            db.close();
        }
    }
}

/*
Maven dependencies (add to pom.xml):
<dependencies>
    <dependency>
        <groupId>org.postgresql</groupId>
        <artifactId>postgresql</artifactId>
        <version>42.7.1</version>
    </dependency>
</dependencies>

Gradle dependencies (add to build.gradle):
dependencies {
    implementation 'org.postgresql:postgresql:42.7.1'
}

Compilation and execution:
1. Ensure PostgreSQL JDBC driver is in classpath
2. Compile: javac -cp "postgresql-42.7.1.jar" DatabaseManager.java
3. Run: java -cp ".:postgresql-42.7.1.jar" DatabaseManager
*/