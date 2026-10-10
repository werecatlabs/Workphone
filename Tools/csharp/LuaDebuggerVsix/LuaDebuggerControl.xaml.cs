using System;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace LuaDebuggerVsix
{
    public partial class LuaDebuggerControl : UserControl, IDisposable
    {
        private readonly LuaDebugClient _client = new LuaDebugClient();

        public LuaDebuggerControl()
        {
            InitializeComponent();

            _client.LogReceived += AppendLog;
            _client.Disconnected += () => AppendLog("Disconnected.");
            _client.MessageReceived += OnLuaMessageReceived;
            Unloaded += (sender, args) => _client.Disconnect();
        }

        private async void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string host = HostTextBox.Text.Trim();

                if (!int.TryParse(PortTextBox.Text.Trim(), out int port))
                {
                    AppendLog("Invalid port.");
                    return;
                }

                await _client.ConnectAsync(host, port);
            }
            catch (Exception ex)
            {
                AppendLog($"Connect failed: {ex.Message}");
            }
        }

        private void DisconnectButton_Click(object sender, RoutedEventArgs e)
        {
            _client.Disconnect();
        }

        private async void ContinueButton_Click(object sender, RoutedEventArgs e)
        {
            await SafeSendAsync(() => _client.ContinueAsync());
        }

        private async void StepButton_Click(object sender, RoutedEventArgs e)
        {
            await SafeSendAsync(() => _client.StepAsync());
        }

        private async void SetBreakpointButton_Click(object sender, RoutedEventArgs e)
        {
            string file = BreakpointFileTextBox.Text.Trim();

            if (!int.TryParse(BreakpointLineTextBox.Text.Trim(), out int line))
            {
                AppendLog("Invalid breakpoint line.");
                return;
            }

            await SafeSendAsync(() => _client.SetBreakpointAsync(file, line));
        }

        private async Task SafeSendAsync(Func<Task> action)
        {
            try
            {
                await action();
            }
            catch (Exception ex)
            {
                AppendLog($"Command failed: {ex.Message}");
            }
        }

        private void OnLuaMessageReceived(LuaDebugMessage message)
        {
            Dispatcher.BeginInvoke(new Action(() =>
            {
                switch (message.Type)
                {
                    case "stopped":
                        AppendLog(
                            $"Stopped: {message.Reason} at {message.File}:{message.Line}");
                        break;

                    case "log":
                        AppendLog(message.Command ?? "");
                        break;

                    default:
                        AppendLog($"Message: {message.Type}");
                        break;
                }
            }));
        }

        private void AppendLog(string text)
        {
            Dispatcher.BeginInvoke(new Action(() =>
            {
                LogTextBox.AppendText(text + Environment.NewLine);
                if (LogTextBox.Text.Length > 128 * 1024)
                    LogTextBox.Text = LogTextBox.Text.Substring(LogTextBox.Text.Length - 64 * 1024);
                LogTextBox.ScrollToEnd();
            }));
        }

        public void Dispose() => _client.Dispose();
    }
}
