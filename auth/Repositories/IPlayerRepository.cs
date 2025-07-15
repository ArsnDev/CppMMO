using AuthServer.Models;
using System.Threading.Tasks;

namespace AuthServer.Repositories
{
    public interface IPlayerRepository
    {
        Task<Player?> GetPlayerByUserIdAsync(int userId);
        Task<Player> CreatePlayerAsync(int userId, string playerName);
    }
}