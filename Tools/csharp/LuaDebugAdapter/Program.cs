using System;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

internal static class Program
{
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
                    var output = stream.CopyToAsync(Console.OpenStandardOutput());
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
