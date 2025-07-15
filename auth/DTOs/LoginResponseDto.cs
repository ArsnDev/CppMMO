namespace AuthServer.DTOs
{
    public class LoginResponseDto
    {
        public bool Success { get; set; }
        public string? SessionTicket { get; set; }
        public PlayerInfoDto? PlayerInfo { get; set; }
        public string Message { get; set; } = string.Empty;
    }
}
