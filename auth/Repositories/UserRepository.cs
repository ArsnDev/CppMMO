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

        public async Task<User> CreateUserAsync(string username, string passwordHash)
        {
            var sql = @"
                INSERT INTO users (Username, PasswordHash) 
                VALUES (@Username, @PasswordHash);
                SELECT * FROM users WHERE Id = LAST_INSERT_ID();
            ";
            var newUserId = await _dbConnection.ExecuteScalarAsync<uint>(sql, new { Username = username, PasswordHash = passwordHash });
            
            return (await GetUserByUsernameAsync(username))!;
        }
    }
}
