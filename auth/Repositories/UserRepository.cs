using System.Data;
using AuthServer.Models;
using Dapper;

namespace AuthServer.Repositories
{
    public class UserRepository : IUserRepository
    {
        private readonly IDbConnection _dbConnection;

        public UserRepository(IDbConnection dbConnection)
        {
            _dbConnection = dbConnection;
        }

        public async Task<User?> GetUserByUsernameAsync(string username)
        {
            var sql = "SELECT Id, Username, PasswordHash, CreatedAt FROM users WHERE Username = @Username";
            return await _dbConnection.QuerySingleOrDefaultAsync<User>(sql, new { Username = username });
        }

        public async Task<User?> GetUserByIdAsync(int userId)
        {
            var sql = "SELECT Id, Username, PasswordHash, CreatedAt FROM users WHERE Id = @UserId";
            return await _dbConnection.QuerySingleOrDefaultAsync<User>(sql, new { UserId = userId });
        }

        public async Task<User> CreateUserAsync(string username, string passwordHash)
        {
            var sql = @"
                INSERT INTO users (Username, PasswordHash, CreatedAt) 
                VALUES (@Username, @PasswordHash, @CreatedAt);
                SELECT * FROM users WHERE Id = LAST_INSERT_ID();
            ";
            
            var user = new { 
                Username = username, 
                PasswordHash = passwordHash, 
                CreatedAt = DateTime.UtcNow 
            };
            
            var result = await _dbConnection.QuerySingleAsync<User>(sql, user);
            return result;
        }

        public async Task<bool> DeleteUserAsync(int userId)
        {
            var sql = "DELETE FROM users WHERE Id = @UserId";
            var affectedRows = await _dbConnection.ExecuteAsync(sql, new { UserId = userId });
            return affectedRows > 0;
        }
    }
}
