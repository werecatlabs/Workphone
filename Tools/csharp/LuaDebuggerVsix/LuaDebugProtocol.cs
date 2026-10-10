using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.IO;
using System.Text;

namespace LuaDebuggerVsix
{
    [DataContract]
    public sealed class LuaDebugMessage
    {
        [DataMember(Name = "type", EmitDefaultValue = false)]
        public string Type { get; set; } = "";

        [DataMember(Name = "file", EmitDefaultValue = false)]
        public string? File { get; set; }

        [DataMember(Name = "line", EmitDefaultValue = false)]
        public int? Line { get; set; }

        [DataMember(Name = "reason", EmitDefaultValue = false)]
        public string? Reason { get; set; }

        [DataMember(Name = "command", EmitDefaultValue = false)]
        public string? Command { get; set; }

        [DataMember(Name = "expression", EmitDefaultValue = false)]
        public string? Expression { get; set; }
    }
    internal static class LuaDebugCodec
    {
        public static string Serialize(LuaDebugMessage message)
        {
            using (var stream = new MemoryStream())
            {
                new DataContractJsonSerializer(typeof(LuaDebugMessage)).WriteObject(stream, message);
                return Encoding.UTF8.GetString(stream.ToArray());
            }
        }
        public static LuaDebugMessage? Deserialize(string json)
        {
            using (var stream = new MemoryStream(Encoding.UTF8.GetBytes(json)))
                return new DataContractJsonSerializer(typeof(LuaDebugMessage)).ReadObject(stream) as LuaDebugMessage;
        }
    }

}