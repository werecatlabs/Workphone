using System.Text.Json.Serialization;

namespace LuaDebuggerVsix
{
    public sealed class LuaDebugMessage
    {
        [JsonPropertyName("type")]
        public string Type { get; set; } = "";

        [JsonPropertyName("file")]
        public string? File { get; set; }

        [JsonPropertyName("line")]
        public int? Line { get; set; }

        [JsonPropertyName("reason")]
        public string? Reason { get; set; }

        [JsonPropertyName("command")]
        public string? Command { get; set; }

        [JsonPropertyName("expression")]
        public string? Expression { get; set; }
    }
}