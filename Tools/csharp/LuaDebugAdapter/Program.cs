using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

internal static class Program
{
    private const int MaxMessageBytes = 65536;

    private static async Task<byte[]> ReadHeaderAsync(Stream stream)
    {
        using (var header = new MemoryStream())
        {
            var next = new byte[1];
            int ending = 0;
            while (header.Length < 4096)
            {
                if (await stream.ReadAsync(next, 0, 1) == 0)
                {
                    if (header.Length == 0) return null;
                    throw new EndOfStreamException("Workphone debugger closed during a DAP header.");
                }
                header.WriteByte(next[0]);
                ending = (ending << 8) | next[0];
                if (ending == 0x0d0a0d0a) return header.ToArray();
            }
            throw new InvalidDataException("Workphone DAP header exceeds 4 KiB.");
        }
    }

    private static int ContentLength(byte[] header)
    {
        int length = -1;
        foreach (var line in Encoding.ASCII.GetString(header).Split(new[] { "\r\n" }, StringSplitOptions.RemoveEmptyEntries))
        {
            var colon = line.IndexOf(':');
            if (colon < 0) throw new InvalidDataException("Invalid Workphone DAP header.");
            if (!string.Equals(line.Substring(0, colon).Trim(), "Content-Length", StringComparison.OrdinalIgnoreCase)) continue;
            if (length != -1 || !int.TryParse(line.Substring(colon + 1).Trim(), NumberStyles.None,
                    CultureInfo.InvariantCulture, out length) || length <= 0 || length > MaxMessageBytes)
                throw new InvalidDataException("Invalid Workphone DAP content length.");
        }
        if (length < 0) throw new InvalidDataException("Workphone DAP content length is missing.");
        return length;
    }

    private static async Task RelayEngineAsync(Stream engine, Stream output)
    {
        var json = new JavaScriptSerializer { MaxJsonLength = MaxMessageBytes, RecursionLimit = 128 };
        var utf8 = new UTF8Encoding(false, true);
        bool terminated = false;
        int sequence = 0;
        // The caller owns the network stream. Keep it open until this task has
        // published termination, so the stdin relay cannot win the shutdown race
        // with a write-to-disposed-stream failure.
        var buffered = new BufferedStream(engine, 8192);
        for (;;)
        {
            var header = await ReadHeaderAsync(buffered);
            if (header == null) break;
            var body = new byte[ContentLength(header)];
            for (int count = 0; count < body.Length;)
            {
                int received = await buffered.ReadAsync(body, count, body.Length - count);
                if (received == 0) throw new EndOfStreamException("Workphone debugger closed during a DAP message.");
                count += received;
            }
            var message = json.Deserialize<Dictionary<string, object>>(utf8.GetString(body));
            if (message == null || !message.TryGetValue("seq", out object number) || !(number is int current) || current < 1)
                throw new InvalidDataException("Workphone DAP message sequence is invalid.");
            sequence = Math.Max(sequence, current);
            if (message.TryGetValue("type", out object type) && Equals(type, "event") &&
                message.TryGetValue("event", out object name) && Equals(name, "terminated")) terminated = true;
            await output.WriteAsync(header, 0, header.Length);
            await output.WriteAsync(body, 0, body.Length);
            await output.FlushAsync();
        }
        // A normal Editor shutdown closes the socket without a disconnect request.
        // Tell the IDE the session ended before closing adapter stdout; otherwise
        // DebugAdapterHost reports an unexpected adapter exit despite status zero.
        if (!terminated)
        {
            if (sequence == int.MaxValue) throw new InvalidDataException("Workphone DAP sequence limit reached.");
            var body = Encoding.UTF8.GetBytes("{\"seq\":" + (sequence + 1).ToString(CultureInfo.InvariantCulture) +
                ",\"type\":\"event\",\"event\":\"terminated\"}");
            var header = Encoding.ASCII.GetBytes("Content-Length: " + body.Length.ToString(CultureInfo.InvariantCulture) + "\r\n\r\n");
            await output.WriteAsync(header, 0, header.Length);
            await output.WriteAsync(body, 0, body.Length);
            await output.FlushAsync();
        }
    }

    // DAP remains framed end-to-end. Never write diagnostics to protocol stdout.
    public static async Task<int> Main(string[] args)
    {
        if (args.Length != 1 || !int.TryParse(args[0], out int port) || port < 1 || port > 65535)
        { Console.Error.WriteLine("Expected a local Workphone debugger port."); return 2; }
        try
        {
            using (var client = new TcpClient { NoDelay = true })
            {
                var connect = client.ConnectAsync(IPAddress.Loopback, port);
                if (await Task.WhenAny(connect, Task.Delay(10000)) != connect)
                    throw new TimeoutException("Workphone debugger did not accept the connection.");
                await connect;
                using (var stream = client.GetStream())
                {
                    var input = Console.OpenStandardInput().CopyToAsync(stream);
                    var output = RelayEngineAsync(stream, Console.OpenStandardOutput());
                    var completed = await Task.WhenAny(input, output);
                    await completed;
                    client.Close();
                }
            }
            return 0;
        }
        catch (Exception error) { Console.Error.WriteLine(error.Message); return 1; }
    }
}
