using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows;
using System.Windows.Controls;
using Microsoft.VisualStudio.Shell;

namespace LuaDebuggerVsix
{
    public partial class LuaDebuggerControl : UserControl, IDisposable
    {
        private string? _launchPath;
        private readonly JavaScriptSerializer _json = new JavaScriptSerializer { MaxJsonLength = 65536 };
        public LuaDebuggerControl() { InitializeComponent(); }
        private void LoadEndpoint_Click(object sender, RoutedEventArgs args)
        {
            var dialog = new Microsoft.Win32.OpenFileDialog { Filter = "Workphone endpoint|lua-debug.json|JSON|*.json" };
            if (dialog.ShowDialog() != true) return;
            try
            {
                if (new FileInfo(dialog.FileName).Length > 65536) throw new InvalidDataException("Endpoint file is too large.");
                var endpoint = _json.Deserialize<Dictionary<string, object>>(File.ReadAllText(dialog.FileName));
                PortTextBox.Text = Convert.ToString(endpoint["port"]);
                TokenTextBox.Password = Convert.ToString(endpoint["token"]);
                StatusText.Text = "Endpoint loaded. Attach, set breakpoints, then enter Play in the Editor.";
            }
            catch (Exception error) { StatusText.Text = error.Message; }
        }
        private void Attach_Click(object sender, RoutedEventArgs args)
        {
            _ = ThreadHelper.JoinableTaskFactory.RunAsync(async () =>
            {
                try { await AttachAsync(); }
                catch (Exception error)
                {
                    await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();
                    StatusText.Text = "Attach failed: " + error.Message;
                }
            });
        }
        private async Task AttachAsync()
        {
            if (!int.TryParse(PortTextBox.Text, out int port) || port < 1 || port > 65535)
                throw new ArgumentException("Enter a valid port.");
            if (TokenTextBox.Password.Length < 16) throw new ArgumentException("Load the Editor's session token.");
            var package = LuaDebuggerPackage.Instance ?? throw new InvalidOperationException("Package is unavailable.");
            var automation = await package.GetServiceAsync(typeof(Microsoft.VisualStudio.Shell.Interop.SDTE));
            await package.JoinableTaskFactory.SwitchToMainThreadAsync();
            var dte = automation as EnvDTE.DTE;
            if (dte == null) throw new InvalidOperationException("Visual Studio automation is unavailable.");
            var adapter = Path.Combine(Path.GetDirectoryName(typeof(LuaDebuggerControl).Assembly.Location), "adapter", "WorkphoneLuaDebugAdapter.exe");
            if (!File.Exists(adapter)) throw new FileNotFoundException("Packaged Lua debug adapter is missing.", adapter);
            var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Workphone", "LuaDebugger");
            Directory.CreateDirectory(directory);
            if (_launchPath != null) File.Delete(_launchPath);
            _launchPath = Path.Combine(directory, "launch-" + Guid.NewGuid().ToString("N") + ".json");
            File.WriteAllText(_launchPath, _json.Serialize(new Dictionary<string, object> {
                { "$adapter", adapter }, { "$adapterArgs", port.ToString(System.Globalization.CultureInfo.InvariantCulture) },
                { "type", "workphone-lua" }, { "request", "attach" }, { "name", "Workphone Lua" },
                { "token", TokenTextBox.Password }, { "stopOnEntry", StopOnEntry.IsChecked == true },
                { "$localsScopeName", "Locals" }
            }));
            dte.Commands.Raise("0ddba113-7ac1-4c6e-a2ef-dcac3f9e731e", 0x0101,
                "/LaunchJson:\"" + _launchPath + "\" /EngineGuid:\"8BC205D5-3836-4E05-A729-A6D15A6E3884\"", IntPtr.Zero);
            StatusText.Text = "Attach requested. Use Visual Studio's standard debugger windows.";
        }
        public void Dispose()
        {
            if (_launchPath != null) { try { File.Delete(_launchPath); } catch (IOException) { } }
        }
    }
}
