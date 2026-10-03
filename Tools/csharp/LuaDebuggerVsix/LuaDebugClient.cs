using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace LuaDebuggerVsix
{
    public sealed class LuaDebugClient : IDisposable
    {
        private TcpClient? _client;
        private StreamReader? _reader;
        private StreamWriter? _writer;
        private CancellationTokenSource? _cts;

        public event Action<LuaDebugMessage>? MessageReceived;
        public event Action<string>? LogReceived;
        public event Action? Disconnected;

        public bool IsConnected => _client?.Connected == true;

        public async Task ConnectAsync(string host, int port)
        {
            Disconnect();

            _cts = new CancellationTokenSource();
            _client = new TcpClient();

            await _client.ConnectAsync(host, port);

            NetworkStream stream = _client.GetStream();

            _reader = new StreamReader(stream, Encoding.UTF8, false, 1024, leaveOpen: true);
            _writer = new StreamWriter(stream, new UTF8Encoding(false), 1024, leaveOpen: true)
            {
                AutoFlush = true,
                NewLine = "\n"
            };

            LogReceived?.Invoke($"Connected to Lua debugger at {host}:{port}");

            _ = Task.Run(() => ReadLoopAsync(_cts.Token));
        }

        public void Disconnect()
        {
            try
            {
                _cts?.Cancel();
                _reader?.Dispose();
                _writer?.Dispose();
                _client?.Close();
            }
            catch
            {
                // Ignore shutdown errors.
            }

            _cts = null;
            _reader = null;
            _writer = null;
            _client = null;
        }

        public async Task SendAsync(LuaDebugMessage message)
        {
            if (_writer == null)
                throw new InvalidOperationException("Lua debugger is not connected.");

            string json = JsonSerializer.Serialize(message);
            await _writer.WriteLineAsync(json);
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

        private async Task ReadLoopAsync(CancellationToken token)
        {
            try
            {
                while (!token.IsCancellationRequested && _reader != null)
                {
                    string? line = await _reader.ReadLineAsync();

                    if (line == null)
                        break;

                    if (string.IsNullOrWhiteSpace(line))
                        continue;

                    try
                    {
                        LuaDebugMessage? message =
                            JsonSerializer.Deserialize<LuaDebugMessage>(line);

                        if (message != null)
                            MessageReceived?.Invoke(message);
                    }
                    catch (Exception ex)
                    {
                        LogReceived?.Invoke($"Invalid debugger message: {ex.Message}");
                        LogReceived?.Invoke(line);
                    }
                }
            }
            catch (Exception ex)
            {
                LogReceived?.Invoke($"Debugger connection error: {ex.Message}");
            }
            finally
            {
                Disconnected?.Invoke();
            }
        }

        public void Dispose()
        {
            Disconnect();
        }
    }
}