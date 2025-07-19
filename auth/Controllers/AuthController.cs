using AuthServer.DTOs;
using AuthServer.Models;
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
        private readonly ILogger<AuthController> _logger;

        public AuthController(IUserRepository userRepository, IPlayerRepository playerRepository, IConnectionMultiplexer redis, ILogger<AuthController> logger)
        {
            _userRepository = userRepository;
            _playerRepository = playerRepository;
            _redis = redis;
            _logger = logger;
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

            var sessionTicket = Guid.NewGuid().ToString();
            var redisDb = _redis.GetDatabase();
            await redisDb.StringSetAsync($"session:{sessionTicket}", newUser.Id, TimeSpan.FromHours(1));

            var response = new LoginResponseDto
            {
                Success = true,
                SessionTicket = sessionTicket,
                PlayerInfo = null,
                Message = "Registration successful."
            };

            return Ok(response);
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

            var sessionTicket = Guid.NewGuid().ToString();
            var redisDb = _redis.GetDatabase();
            
            await redisDb.StringSetAsync($"session:{sessionTicket}", user.Id, TimeSpan.FromHours(1));

            var response = new LoginResponseDto
            {
                Success = true,
                SessionTicket = sessionTicket,
                PlayerInfo = null,
                Message = "Login successful."
            };

            return Ok(response);
        }

        [HttpPost("verify")]
        public async Task<IActionResult> Verify(VerifyRequestDto request)
        {
            if (string.IsNullOrEmpty(request.SessionTicket))
            {
                return BadRequest(new VerifyResponseDto { Success = false, Message = "Session ticket is required." });
            }

            if (request.PlayerId <= 0)
            {
                return BadRequest(new VerifyResponseDto { Success = false, Message = "PlayerId is required." });
            }

            var redisDb = _redis.GetDatabase();
            var userIdString = await redisDb.StringGetAsync($"session:{request.SessionTicket}");

            if (!userIdString.HasValue || !int.TryParse(userIdString, out int userId))
            {
                return Unauthorized(new VerifyResponseDto { Success = false, Message = "Invalid or expired session ticket." });
            }

            // PlayerId 소유권 검증
            var player = await _playerRepository.GetPlayerByIdAsync(request.PlayerId);
            
            if (player == null)
            {
                return NotFound(new VerifyResponseDto { Success = false, Message = "Player not found." });
            }

            if (player.UserId != userId)
            {
                return Unauthorized(new VerifyResponseDto { Success = false, Message = "Player does not belong to this user." });
            }

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
                Message = "Session and player verification successful."
            });
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

        [HttpGet("characters")]
        public async Task<IActionResult> GetCharacters([FromHeader] string sessionTicket)
        {
            if (string.IsNullOrEmpty(sessionTicket))
            {
                return BadRequest(new CharacterListResponseDto { Success = false, Message = "Session ticket is required." });
            }

            var redisDb = _redis.GetDatabase();
            var userIdString = await redisDb.StringGetAsync($"session:{sessionTicket}");

            if (!userIdString.HasValue || !int.TryParse(userIdString, out int userId))
            {
                return Unauthorized(new CharacterListResponseDto { Success = false, Message = "Invalid or expired session ticket." });
            }

            var players = await _playerRepository.GetPlayersByUserIdAsync(userId);
            var characterList = players.Select(p => new PlayerInfoDto
            {
                PlayerId = p.PlayerId,
                Name = p.Name,
                PosX = p.PosX,
                PosY = p.PosY,
                Hp = p.Hp,
                MaxHp = p.MaxHp
            }).ToList();

            return Ok(new CharacterListResponseDto
            {
                Success = true,
                Characters = characterList,
                Message = "Characters retrieved successfully."
            });
        }

        [HttpPost("characters")]
        public async Task<IActionResult> CreateCharacter([FromHeader] string sessionTicket, [FromBody] CharacterRequestDto request)
        {
            if (string.IsNullOrEmpty(sessionTicket))
            {
                return BadRequest(new CharacterCreateResponseDto { Success = false, Message = "Session ticket is required." });
            }

            if (string.IsNullOrEmpty(request.CharacterName) || request.CharacterName.Length < 2 || request.CharacterName.Length > 16)
            {
                return BadRequest(new CharacterCreateResponseDto { Success = false, Message = "Character name must be between 2 and 16 characters." });
            }

            var redisDb = _redis.GetDatabase();
            var userIdString = await redisDb.StringGetAsync($"session:{sessionTicket}");

            if (!userIdString.HasValue || !int.TryParse(userIdString, out int userId))
            {
                return Unauthorized(new CharacterCreateResponseDto { Success = false, Message = "Invalid or expired session ticket." });
            }

            if (await _playerRepository.IsPlayerNameExistsAsync(request.CharacterName))
            {
                return Conflict(new CharacterCreateResponseDto { Success = false, Message = "Character name already exists." });
            }

            var existingPlayers = await _playerRepository.GetPlayersByUserIdAsync(userId);
            if (existingPlayers.Count >= 3)
            {
                return BadRequest(new CharacterCreateResponseDto { Success = false, Message = "Maximum character limit reached (3 characters)." });
            }

            var newPlayer = await _playerRepository.CreatePlayerAsync(userId, request.CharacterName);

            return Ok(new CharacterCreateResponseDto
            {
                Success = true,
                Character = new PlayerInfoDto
                {
                    PlayerId = newPlayer.PlayerId,
                    Name = newPlayer.Name,
                    PosX = newPlayer.PosX,
                    PosY = newPlayer.PosY,
                    Hp = newPlayer.Hp,
                    MaxHp = newPlayer.MaxHp
                },
                Message = "Character created successfully."
            });
        }

        [HttpDelete("user")]
        public async Task<IActionResult> DeleteUser([FromBody] DeleteUserRequestDto request)
        {
            if (string.IsNullOrEmpty(request.SessionTicket) || string.IsNullOrEmpty(request.Password))
            {
                return BadRequest(new DeleteUserResponseDto { Success = false, Message = "Session ticket and password are required." });
            }

            var redisDb = _redis.GetDatabase();
            var userIdString = await redisDb.StringGetAsync($"session:{request.SessionTicket}");

            if (!userIdString.HasValue || !int.TryParse(userIdString, out int userId))
            {
                return Unauthorized(new DeleteUserResponseDto { Success = false, Message = "Invalid or expired session ticket." });
            }

            // Verify password before deletion
            var user = await _userRepository.GetUserByIdAsync(userId);
            if (user == null || !BCrypt.Net.BCrypt.Verify(request.Password, user.PasswordHash))
            {
                return Unauthorized(new DeleteUserResponseDto { Success = false, Message = "Invalid password." });
            }

            try
            {
                int deletedCharactersCount = await _playerRepository.DeletePlayersByUserIdAsync(userId);
                
                bool userDeleted = await _userRepository.DeleteUserAsync(userId);
                
                if (!userDeleted)
                {
                    return BadRequest(new DeleteUserResponseDto { Success = false, Message = "Failed to delete user account." });
                }
                
                await redisDb.KeyDeleteAsync($"session:{request.SessionTicket}");
                
                return Ok(new DeleteUserResponseDto
                {
                    Success = true,
                    Message = $"Account deleted successfully. {deletedCharactersCount} characters were also removed.",
                    DeletedCharactersCount = deletedCharactersCount
                });
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Failed to delete user account for userId: {UserId}", userId);
                return StatusCode(500, new DeleteUserResponseDto 
                { 
                    Success = false, 
                    Message = "An error occurred while deleting account. Please try again later." 
                });
            }
        }
    }
}
