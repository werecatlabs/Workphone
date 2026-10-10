using LuaDebuggerVsix;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;

static class Program
{
    static void Require(bool value, string message)
    {
        if (!value) throw new Exception(message);
    }
    static async Task Within(Task task)
    {
        if (await Task.WhenAny(task, Task.Delay(5000)) != task) throw new TimeoutException("Debugger transport stalled.");
        await task;
    }
    static async Task Main()
    {
        using var listener = new TcpListener(IPAddress.Loopback, 0);
        listener.Start();
        int port = ((IPEndPoint)listener.LocalEndpoint).Port;
        using var client = new LuaDebugClient();
        int disconnected = 0;
        client.Disconnected += () => Interlocked.Increment(ref disconnected);
        for (int round = 0; round < 20; ++round)
        {
            var accept = listener.AcceptTcpClientAsync();
            await Within(client.ConnectAsync("127.0.0.1", port));
            using var peer = await accept;
            Require(client.IsConnected, "Connect did not publish its session.");
            var received = new TaskCompletionSource<LuaDebugMessage>(TaskCreationOptions.RunContinuationsAsynchronously);
            Action<LuaDebugMessage> handler = message => received.TrySetResult(message);
            client.MessageReceived += handler;
            byte[] frame = Encoding.UTF8.GetBytes("{\"type\":\"stopped\",\"file\":\"scripts/café.lua\",\"line\":7}\n");
            // Fragment every byte, including a UTF-8 code point, across network writes.
            foreach (byte value in frame) await peer.GetStream().WriteAsync(new[] { value });
            await Within(received.Task);
            Require(received.Task.Result.File == "scripts/café.lua", "Fragmented UTF-8 was corrupted.");
            client.MessageReceived -= handler;
            using var reader = new StreamReader(peer.GetStream(), Encoding.UTF8, false, 1024, true);
            var commands = Enumerable.Range(0, 32).Select(_ => client.ContinueAsync()).ToArray();
            for (int i = 0; i < commands.Length; ++i)
            {
                string? line = await reader.ReadLineAsync();
                Require(line != null && JsonDocument.Parse(line).RootElement.GetProperty("type").GetString() == "continue", "Concurrent writes interleaved JSON.");
            }
            await Within(Task.WhenAll(commands));
            await Within(client.DisconnectAsync());
            Require(!client.IsConnected, "Disconnect retained its session.");
        }
        Require(disconnected == 20, "Disconnect events duplicated across reconnects.");
        foreach (byte[] invalid in new[] {
            Encoding.UTF8.GetBytes("not-json\n"),
            Encoding.UTF8.GetBytes("{}\n"),
            new byte[] { 0xff, (byte)'\n' },
            Encoding.UTF8.GetBytes(new string('x', LuaDebugClient.MaximumMessageBytes + 1)) })
        {
            var accept = listener.AcceptTcpClientAsync();
            await Within(client.ConnectAsync("127.0.0.1", port));
            using var peer = await accept;
            var ended = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
            Action handler = () => ended.TrySetResult();
            client.Disconnected += handler;
            await peer.GetStream().WriteAsync(invalid);
            await Within(ended.Task);
            client.Disconnected -= handler;
            Require(!client.IsConnected, "Invalid framing did not close its session.");
        }
        // A previous reader finishing after replacement must not close the new socket.
        var firstAccept = listener.AcceptTcpClientAsync();
        await Within(client.ConnectAsync("127.0.0.1", port));
        using var firstPeer = await firstAccept;
        var secondAccept = listener.AcceptTcpClientAsync();
        await Within(client.ConnectAsync("127.0.0.1", port));
        using var secondPeer = await secondAccept;
        await Within(client.ContinueAsync());
        using (var reader = new StreamReader(secondPeer.GetStream()))
            Require((await reader.ReadLineAsync())?.Contains("continue") == true, "Old reader closed replacement session.");
        await Within(client.DisconnectAsync());
        client.Dispose();
        try { await client.ConnectAsync("127.0.0.1", port); throw new Exception("Disposed client reconnected."); }
        catch (ObjectDisposedException) { }
        Console.WriteLine("Lua debugger transport tests: PASS");
    }
}
