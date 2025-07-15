namespace AuthServer.DTOs
{
    public class PlayerInfoDto
    {
        public long PlayerId { get; set; }
        public string Name { get; set; } = string.Empty;
        // fbs.txt의 Vec2에 해당
        public float PosX { get; set; }
        public float PosY { get; set; }
        public int Hp { get; set; }
        public int MaxHp { get; set; }
    }
}
