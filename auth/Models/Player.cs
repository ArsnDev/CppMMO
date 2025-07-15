namespace AuthServer.Models
{
    public class Player
    {
        public long PlayerId { get; set; }
        public int UserId { get; set; }
        public string Name { get; set; } = string.Empty;
        public float PosX { get; set; }
        public float PosY { get; set; }
        public int Hp { get; set; }
        public int MaxHp { get; set; }
        public DateTime CreatedAt { get; set; }
        public DateTime UpdatedAt { get; set; }
    }
}