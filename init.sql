CREATE DATABASE IF NOT EXISTS auth_db;
USE auth_db;

CREATE TABLE IF NOT EXISTS users (
    Id INT AUTO_INCREMENT PRIMARY KEY,
    Username VARCHAR(255) NOT NULL UNIQUE,
    PasswordHash VARCHAR(255) NOT NULL,
    CreatedAt DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS players (
    PlayerId BIGINT AUTO_INCREMENT PRIMARY KEY,
    UserId INT NOT NULL,
    Name VARCHAR(255) NOT NULL,
    PosX FLOAT NOT NULL,
    PosY FLOAT NOT NULL,
    Hp INT NOT NULL,
    MaxHp INT NOT NULL,
    CreatedAt DATETIME DEFAULT CURRENT_TIMESTAMP,
    UpdatedAt DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (UserId) REFERENCES users(Id) ON DELETE CASCADE,
    UNIQUE KEY unique_player_name (Name)
);