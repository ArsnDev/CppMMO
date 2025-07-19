namespace AuthServer.DTOs
{
    public class CharacterRequestDto
    {
        public string CharacterName { get; set; } = string.Empty;
    }

    public class CharacterListResponseDto
    {
        public bool Success { get; set; }
        public List<PlayerInfoDto> Characters { get; set; } = new List<PlayerInfoDto>();
        public string Message { get; set; } = string.Empty;
    }

    public class CharacterCreateResponseDto
    {
        public bool Success { get; set; }
        public PlayerInfoDto? Character { get; set; }
        public string Message { get; set; } = string.Empty;
    }
}