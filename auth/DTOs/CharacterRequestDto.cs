using System.ComponentModel.DataAnnotations;

namespace AuthServer.DTOs
{
    public class CharacterRequestDto
    {
        [Required(ErrorMessage = "캐릭터 이름은 필수입니다.")]
        [StringLength(20, MinimumLength = 2, ErrorMessage = "캐릭터 이름은 2-20자 사이여야 합니다.")]
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