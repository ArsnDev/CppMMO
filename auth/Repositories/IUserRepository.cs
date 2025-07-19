using AuthServer.Models;

namespace AuthServer.Repositories
{
    public interface IUserRepository
    {
        Task<User?> GetUserByUsernameAsync(string username);
        Task<User?> GetUserByIdAsync(int userId);
        Task<User> CreateUserAsync(string username, string passwordHash);
        Task<bool> DeleteUserAsync(int userId);
    }
}
