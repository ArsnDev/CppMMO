using AuthServer.Models;
using System.Threading.Tasks;

namespace AuthServer.Repositories
{
    public interface IPlayerRepository
    {
        Task<Player?> GetPlayerByUserIdAsync(int userId);
        Task<List<Player>> GetPlayersByUserIdAsync(int userId);
        Task<Player?> GetPlayerByIdAsync(long playerId);
        Task<Player> CreatePlayerAsync(int userId, string playerName);
        Task<bool> IsPlayerNameExistsAsync(string playerName);
        Task<int> DeletePlayersByUserIdAsync(int userId);
    }
}