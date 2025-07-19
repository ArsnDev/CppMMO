using System.ComponentModel.DataAnnotations;

namespace AuthServer.DTOs
{
    public class VerifyRequestDto
    {
        [Required]
        public string SessionTicket { get; set; } = string.Empty;
        
        [Required]
        public long PlayerId { get; set; }
    }
}
