using AuthServer.DTOs;
using AuthServer.Models.DTOs;
using AuthServer.Repositories;
using Microsoft.AspNetCore.Mvc;
using StackExchange.Redis;
using BCryptNet = BCrypt.Net.BCrypt;

namespace AuthServer.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class AuthController : ControllerBase
    {
        private readonly IUserRepository _userRepository;
        private readonly IPlayerRepository _playerRepository;
        private readonly IConnectionMultiplexer _redis;

        public AuthController(IUserRepository userRepository, IPlayerRepository playerRepository, IConnectionMultiplexer redis)
        {
            _userRepository = userRepository;
            _playerRepository = playerRepository;
            _redis = redis;
        }

        [HttpPost("register")]
        public async Task<IActionResult> Register(LoginRequestDto request)
        {
            if (!ModelState.IsValid)
            {
                return BadRequest(ModelState);
            }

            var existingUser = await _userRepository.GetUserByUsernameAsync(request.Username);
            if (existingUser != null)
            {
                return Conflict(new { message = "Username already exists." });
            }

            var passwordHash = BCryptNet.HashPassword(request.Password);
            var newUser = await _userRepository.CreateUserAsync(request.Username, passwordHash);

            // 회원가입 시 플레이어(캐릭터)를 바로 생성
            await _playerRepository.CreatePlayerAsync(newUser.Id, request.Username); // 사용자 이름으로 플레이어 이름 설정

            return Ok(new { success = true, message = "User registered successfully." });
        }

        [HttpPost("login")]
        public async Task<IActionResult> Login(LoginRequestDto request)
        {
            if (!ModelState.IsValid)
            {
                return BadRequest(ModelState);
            }

            var user = await _userRepository.GetUserByUsernameAsync(request.Username);
            if (user == null || !BCryptNet.Verify(request.Password, user.PasswordHash))
            {
                return Unauthorized(new LoginResponseDto 
                { 
                    Success = false, 
                    Message = "Invalid username or password." 
                });
            }

            var player = await _playerRepository.GetPlayerByUserIdAsync(user.Id);
            if (player == null)
            {
                // 플레이어가 없는 경우 새로 생성 (예외 처리 또는 자동 생성 로직)
                player = await _playerRepository.CreatePlayerAsync(user.Id, user.Username); // 사용자 이름으로 플레이어 이름 설정
            }

            var sessionTicket = Guid.NewGuid().ToString();
            var redisDb = _redis.GetDatabase();
            
            // 세션 티켓을 키로, 사용자 ID를 값으로 하여 1시간 동안 Redis에 저장
            await redisDb.StringSetAsync($"session:{sessionTicket}", user.Id, TimeSpan.FromHours(1));

            var response = new LoginResponseDto
            {
                Success = true,
                SessionTicket = sessionTicket,
                PlayerInfo = new PlayerInfoDto
                {
                    PlayerId = player.PlayerId,
                    Name = player.Name,
                    PosX = player.PosX,
                    PosY = player.PosY,
                    Hp = player.Hp,
                    MaxHp = player.MaxHp
                },
                Message = "Login successful."
            };

            return Ok(response);
        }

        [HttpPost("verify")]
        public async Task<IActionResult> Verify(VerifyRequestDto request)
        {
            if (string.IsNullOrEmpty(request.SessionTicket))
            {
                return BadRequest(new { message = "Session ticket is required." });
            }

            var redisDb = _redis.GetDatabase();
            var userIdString = await redisDb.StringGetAsync($"session:{request.SessionTicket}");

            if (userIdString.HasValue && int.TryParse(userIdString, out int userId))
            {
                var player = await _playerRepository.GetPlayerByUserIdAsync(userId);
                if (player != null)
                {
                    return Ok(new VerifyResponseDto
                    {
                        Success = true,
                        PlayerInfo = new PlayerInfoDto
                        {
                            PlayerId = player.PlayerId,
                            Name = player.Name,
                            PosX = player.PosX,
                            PosY = player.PosY,
                            Hp = player.Hp,
                            MaxHp = player.MaxHp
                        },
                        Message = "Session ticket is valid."
                    });
                }
                
                // Player not found for a valid user, which is an error condition in the game server's context.
                return NotFound(new VerifyResponseDto { Success = false, Message = "Player not found for this user." });
            }

            return Unauthorized(new VerifyResponseDto { Success = false, Message = "Invalid or expired session ticket." });
        }

        [HttpPost("logout")]
        public async Task<IActionResult> Logout([FromBody] LogoutRequestDto request)
        {
            var redisDb = _redis.GetDatabase();
            bool deleted = await redisDb.KeyDeleteAsync($"session:{request.SessionTicket}");

            if (deleted)
            {
                return Ok(new { message = "Logged out successfully." });
            }
            return BadRequest(new { message = "Failed to logout or session not found." });
        }
    }
}
