# PostgreSQL Database Schema for Web Server

## Users Table

```sql
-- Create users table
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create index on username for faster lookups
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
```

## Initialize Database (run this once)

```bash
psql -h localhost -U your_username -d your_database -f schema.sql
```

## Sample Users (for testing)

```sql
-- Insert test user (password: password123)
INSERT INTO users (username, password, email) 
VALUES ('testuser', 'password123', 'test@example.com');
```
