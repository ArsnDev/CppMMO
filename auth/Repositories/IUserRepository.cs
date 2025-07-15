using AuthServer.Models;

namespace AuthServer.Repositories
{
    public interface IUserRepository
    {
        Task<User?> GetUserByUsernameAsync(string username);
        Task<User> CreateUserAsync(string username, string passwordHash);
    }
}
