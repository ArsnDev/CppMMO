namespace AuthServer.DTOs
{
    public class VerifyResponseDto
    {
        public bool Success { get; set; }
        public PlayerInfoDto? PlayerInfo { get; set; }
        public string? Message { get; set; }
    }
}
