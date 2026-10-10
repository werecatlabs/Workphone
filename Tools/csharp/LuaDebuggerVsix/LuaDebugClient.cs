using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace LuaDebuggerVsix
{
    public sealed class LuaDebugClient : IDisposable
    {
        private TcpClient? _client;
        private StreamWriter? _writer;
        private CancellationTokenSource? _cts;
        private Task _readTask = Task.CompletedTask;
        private readonly SemaphoreSlim _sendGate = new SemaphoreSlim(1, 1);
        private int _generation;
        private readonly object _connectionGate = new object();
        private bool _disposed;
        public const int MaximumMessageBytes = 64 * 1024;

        public event Action<LuaDebugMessage>? MessageReceived;
        public event Action<string>? LogReceived;
        public event Action? Disconnected;

        public bool IsConnected { get { lock (_connectionGate) return _client != null; } }

        public async Task ConnectAsync(string host, int port)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(LuaDebugClient));
            if (string.IsNullOrWhiteSpace(host)) throw new ArgumentException("Host is required.", nameof(host));
            if (port < 1 || port > 65535) throw new ArgumentOutOfRangeException(nameof(port));
            Disconnect();
            int generation;
            lock (_connectionGate) generation = ++_generation;
            var client = new TcpClient { NoDelay = true };
            try
            {
                using (var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(10)))
                using (timeout.Token.Register(() => client.Close()))
                    await client.ConnectAsync(host, port);
                lock (_connectionGate)
                {
                if (_disposed || generation != _generation)
                    throw new OperationCanceledException("The connection request was superseded.");
                _client = client;
                var stream = client.GetStream();
                _writer = new StreamWriter(stream, new UTF8Encoding(false), 1024, leaveOpen: true)
                { AutoFlush = true, NewLine = "\n" };
                _cts = new CancellationTokenSource();
                // Capture this stream/token. Reconnect must never redirect an old reader.
                _readTask = ReadLoopAsync(stream, _cts.Token, generation);
                }
                LogReceived?.Invoke($"Connected to Lua debugger at {host}:{port}");
            }
            catch { client.Close(); throw; }
        }

        public void Disconnect()
        {
            bool connected;
            lock (_connectionGate)
            {
            ++_generation;
            connected = _client != null;
            _cts?.Cancel();
            _client?.Close();
            // A send may still own this writer. Closing its socket interrupts it;
            // disposing StreamWriter concurrently with WriteLineAsync is unsafe.
            _cts?.Dispose();
            _cts = null;
            _writer = null;
            _client = null;
            }
            if (connected) Disconnected?.Invoke();
        }

        public async Task DisconnectAsync()
        {
            var reader = _readTask;
            Disconnect();
            await reader;
        }

        public async Task SendAsync(LuaDebugMessage message)
        {
            StreamWriter writer;
            int generation;
            lock (_connectionGate)
            {
                writer = _writer ?? throw new InvalidOperationException("Lua debugger is not connected.");
                generation = _generation;
            }
            string json = LuaDebugCodec.Serialize(message);
            if (Encoding.UTF8.GetByteCount(json) > MaximumMessageBytes)
                throw new InvalidDataException("Debugger message exceeds 64 KiB.");
            await _sendGate.WaitAsync();
            try
            {
                if (generation != Volatile.Read(ref _generation)) throw new OperationCanceledException("Debugger session changed.");
                await writer.WriteLineAsync(json);
            }
            finally { _sendGate.Release(); }
        }

        public Task ContinueAsync()
        {
            return SendAsync(new LuaDebugMessage
            {
                Type = "continue"
            });
        }

        public Task StepAsync()
        {
            return SendAsync(new LuaDebugMessage
            {
                Type = "step"
            });
        }

        public Task SetBreakpointAsync(string file, int line)
        {
            return SendAsync(new LuaDebugMessage
            {
                Type = "setBreakpoint",
                File = file,
                Line = line
            });
        }

        public Task EvaluateAsync(string expression)
        {
            return SendAsync(new LuaDebugMessage
            {
                Type = "evaluate",
                Expression = expression
            });
        }

        private async Task ReadLoopAsync(NetworkStream stream, CancellationToken token, int generation)
        {
            byte[] buffer = new byte[4096];
            using (var frame = new MemoryStream())
            {
                try
                {
                    while (!token.IsCancellationRequested)
                    {
                        int count = await stream.ReadAsync(buffer, 0, buffer.Length, token);
                        if (count == 0)
                        {
                            if (frame.Length != 0) throw new InvalidDataException("Truncated debugger message.");
                            break;
                        }
                        for (int i = 0; i < count; ++i)
                        {
                            if (buffer[i] != (byte)'\n')
                            {
                                if (frame.Length >= MaximumMessageBytes)
                                    throw new InvalidDataException("Debugger message exceeds 64 KiB.");
                                frame.WriteByte(buffer[i]);
                                continue;
                            }
                            string json = new UTF8Encoding(false, true).GetString(frame.GetBuffer(), 0, (int)frame.Length);
                            frame.SetLength(0);
                            if (string.IsNullOrWhiteSpace(json)) continue;
                            var message = LuaDebugCodec.Deserialize(json);
                            if (message == null || string.IsNullOrWhiteSpace(message.Type))
                                throw new InvalidDataException("Debugger message has no type.");
                            if (generation != Volatile.Read(ref _generation)) return;
                            MessageReceived?.Invoke(message);
                        }
                    }
                }
                catch (Exception ex) when (token.IsCancellationRequested &&
                    (ex is OperationCanceledException || ex is IOException || ex is ObjectDisposedException || ex is SocketException)) { }
                catch (Exception ex) { if (generation == Volatile.Read(ref _generation)) LogReceived?.Invoke($"Debugger connection error: {ex.Message}"); }
                finally
                {
                    lock (_connectionGate)
                        if (generation == _generation) Disconnect();
                }
            }
        }

        public void Dispose()
        {
            lock (_connectionGate) _disposed = true;
            Disconnect();
        }
    }
}
