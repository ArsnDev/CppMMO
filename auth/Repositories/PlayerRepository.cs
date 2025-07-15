using AuthServer.Models;
using System.Collections.Concurrent;
using System.Threading.Tasks;

namespace AuthServer.Repositories
{
    public class PlayerRepository : IPlayerRepository
    {
        private static readonly ConcurrentDictionary<int, Player> _players = new ConcurrentDictionary<int, Player>();
        private static long _nextPlayerId = 1;

        public Task<Player?> GetPlayerByUserIdAsync(int userId)
        {
            _players.TryGetValue(userId, out var player);
            return Task.FromResult(player);
        }

        public Task<Player> CreatePlayerAsync(int userId, string playerName)
        {
            var player = new Player
            {
                PlayerId = Interlocked.Increment(ref _nextPlayerId),
                UserId = userId,
                Name = playerName,
                PosX = 0,
                PosY = 0,
                Hp = 100,
                MaxHp = 100,
                CreatedAt = DateTime.UtcNow,
                UpdatedAt = DateTime.UtcNow
            };
            _players.TryAdd(userId, player);
            return Task.FromResult(player);
        }
    }
}