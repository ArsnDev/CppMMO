using System.Data;
using AuthServer.Repositories;
using MySql.Data.MySqlClient;
using StackExchange.Redis;

namespace AuthServer
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateBuilder(args);

            builder.Services.AddControllers();
            builder.Services.AddEndpointsApiExplorer();
            builder.Services.AddSwaggerGen();

            // MySQL (Dapper) DB Connection
            builder.Services.AddScoped<IDbConnection>(_ => new MySqlConnection(
                builder.Configuration.GetConnectionString("MySql")));

            // Redis Connection
            builder.Services.AddSingleton<IConnectionMultiplexer>(
                ConnectionMultiplexer.Connect(builder.Configuration.GetConnectionString("Redis")));

            // Register Repository
            builder.Services.AddScoped<IUserRepository, UserRepository>();
            builder.Services.AddScoped<IPlayerRepository, PlayerRepository>();

            var app = builder.Build();

            if (app.Environment.IsDevelopment())
            {
                app.UseSwagger();
                app.UseSwaggerUI();
            }

            

            app.UseAuthorization();


            app.MapControllers();

            app.Run();
        }
    }
}

