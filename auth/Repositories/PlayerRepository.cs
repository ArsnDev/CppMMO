using AuthServer.Models;
using System.Data;
using Dapper;

namespace AuthServer.Repositories
{
    public class PlayerRepository : IPlayerRepository
    {
        private readonly IDbConnection _dbConnection;

        public PlayerRepository(IDbConnection dbConnection)
        {
            _dbConnection = dbConnection;
        }

        public async Task<Player?> GetPlayerByUserIdAsync(int userId)
        {
            var sql = "SELECT * FROM players WHERE UserId = @UserId ORDER BY CreatedAt ASC LIMIT 1";
            return await _dbConnection.QuerySingleOrDefaultAsync<Player>(sql, new { UserId = userId });
        }

        public async Task<List<Player>> GetPlayersByUserIdAsync(int userId)
        {
            var sql = "SELECT * FROM players WHERE UserId = @UserId ORDER BY CreatedAt ASC";
            var players = await _dbConnection.QueryAsync<Player>(sql, new { UserId = userId });
            return players.ToList();
        }

        public async Task<Player?> GetPlayerByIdAsync(long playerId)
        {
            var sql = "SELECT * FROM players WHERE PlayerId = @PlayerId";
            return await _dbConnection.QuerySingleOrDefaultAsync<Player>(sql, new { PlayerId = playerId });
        }

        public async Task<bool> IsPlayerNameExistsAsync(string playerName)
        {
            var sql = "SELECT COUNT(*) FROM players WHERE Name = @Name";
            var count = await _dbConnection.ExecuteScalarAsync<int>(sql, new { Name = playerName });
            return count > 0;
        }

        public async Task<Player> CreatePlayerAsync(int userId, string playerName)
        {
            var sql = @"
                INSERT INTO players (UserId, Name, PosX, PosY, Hp, MaxHp, CreatedAt, UpdatedAt) 
                VALUES (@UserId, @Name, @PosX, @PosY, @Hp, @MaxHp, @CreatedAt, @UpdatedAt);
                SELECT * FROM players WHERE PlayerId = LAST_INSERT_ID();
            ";
            
            var player = new Player
            {
                UserId = userId,
                Name = playerName,
                PosX = 0,
                PosY = 0,
                Hp = 100,
                MaxHp = 100,
                CreatedAt = DateTime.UtcNow,
                UpdatedAt = DateTime.UtcNow
            };

            var result = await _dbConnection.QuerySingleAsync<Player>(sql, player);
            return result;
        }

        public async Task<int> DeletePlayersByUserIdAsync(int userId)
        {
            var sql = "DELETE FROM players WHERE UserId = @UserId";
            var affectedRows = await _dbConnection.ExecuteAsync(sql, new { UserId = userId });
            return affectedRows;
        }
    }
}