namespace AuthServer.DTOs
{
    public class DeleteUserRequestDto
    {
        public string SessionTicket { get; set; } = string.Empty;
        public string Password { get; set; } = string.Empty;
    }

    public class DeleteUserResponseDto
    {
        public bool Success { get; set; }
        public string Message { get; set; } = string.Empty;
        public int DeletedCharactersCount { get; set; } = 0;
    }
}