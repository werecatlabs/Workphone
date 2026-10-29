using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Threading;
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
        private bool _busy;
        private bool _disposed;
        private readonly CancellationTokenSource _lifetime = new CancellationTokenSource();
        private readonly JavaScriptSerializer _json = new JavaScriptSerializer { MaxJsonLength = 65536 };
        public LuaDebuggerControl()
        {
            InitializeComponent();
            Loaded += (sender, args) =>
            {
                if (!_disposed) LoadEndpointButton.Focus();
            };
        }
        private void LoadEndpoint_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                var endpointPath = EndpointPathTextBox.Text.Trim().Trim('"');
                if (endpointPath.Length == 0)
                {
                    var dialog = new Microsoft.Win32.OpenFileDialog { Filter = "Workphone endpoint|lua-debug.json|JSON|*.json", CheckFileExists = true };
                    if (dialog.ShowDialog() != true) return;
                    endpointPath = dialog.FileName;
                }
                endpointPath = Path.GetFullPath(endpointPath);
                EndpointPathTextBox.Text = endpointPath;
                if (!File.Exists(endpointPath)) throw new FileNotFoundException("Editor endpoint file does not exist.", endpointPath);
                if (new FileInfo(endpointPath).Length > 65536) throw new InvalidDataException("Endpoint file is too large.");
                var endpoint = _json.Deserialize<Dictionary<string, object>>(File.ReadAllText(endpointPath));
                PortTextBox.Text = Convert.ToString(endpoint["port"]);
                TokenTextBox.Password = Convert.ToString(endpoint["token"]);
                if (endpoint.TryGetValue("sourceRoot", out object root)) WorkspaceTextBox.Text = Convert.ToString(root);
                StatusText.Text = "Endpoint loaded. Attach, set breakpoints, then enter Play in the Editor.";
            }
            catch (Exception error) { StatusText.Text = error.Message; }
        }
        private void Attach_Click(object sender, RoutedEventArgs args)
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            StartAction(AttachAsync);
        }
        private void LaunchEditor_Click(object sender, RoutedEventArgs args)
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            StartAction(LaunchEditorAsync);
        }
        private void StartAction(Func<CancellationToken, Task> action)
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            if (_busy || _disposed) return;
            var package = LuaDebuggerPackage.Instance;
            if (package == null) { StatusText.Text = "Package is unavailable."; return; }
            _busy = true;
            _ = package.JoinableTaskFactory.RunAsync(async () =>
            {
                using (var cancellation = CancellationTokenSource.CreateLinkedTokenSource(_lifetime.Token, package.ShutdownToken))
                {
                try { await action(cancellation.Token); }
                catch (OperationCanceledException) { }
                catch (Exception error)
                {
                    await package.JoinableTaskFactory.SwitchToMainThreadAsync(package.ShutdownToken);
                    if (!_disposed) StatusText.Text = error.Message;
                }
                finally { _busy = false; }
                }
            });
        }
        private async Task LaunchEditorAsync(CancellationToken cancellationToken)
        {
            var executable = EditorPathTextBox.Text.Trim().Trim('"');
            if (executable.Length == 0)
            {
                var dialog = new Microsoft.Win32.OpenFileDialog { Filter = "Workphone Editor|Editor.exe", CheckFileExists = true };
                if (dialog.ShowDialog() != true) return;
                executable = dialog.FileName;
            }
            executable = Path.GetFullPath(executable);
            EditorPathTextBox.Text = executable;
            if (!File.Exists(executable)) throw new FileNotFoundException("Editor executable does not exist.", executable);
            if (!string.Equals(Path.GetFileName(executable), "Editor.exe", StringComparison.OrdinalIgnoreCase))
                throw new ArgumentException("Choose the Workphone Editor.exe executable.");
            var executableFolder = Path.GetDirectoryName(executable);
            var packageRoot = Path.GetDirectoryName(executableFolder) ?? executableFolder;
            var installed = File.Exists(Path.Combine(packageRoot, "editor-package.cfg"));
            string arguments = "";
            if (installed)
            {
                if (!Directory.Exists(WorkspaceTextBox.Text) || WorkspaceTextBox.Text.Contains("\""))
                    throw new ArgumentException("Enter an existing workspace folder for the packaged Editor.");
                arguments = "--package-root \"" + packageRoot.Replace('\\', '/') +
                    "\" --workspace \"" + Path.GetFullPath(WorkspaceTextBox.Text).Replace('\\', '/') + "\"";
            }
            var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Workphone", "LuaDebugger");
            Directory.CreateDirectory(directory);
            var endpointPath = Path.Combine(directory, "endpoint-" + Guid.NewGuid().ToString("N") + ".json");
            var start = new ProcessStartInfo(executable, arguments)
            { UseShellExecute = false, WorkingDirectory = executableFolder };
            start.EnvironmentVariables["WP_LUA_DEBUGGER"] = "1";
            start.EnvironmentVariables["WP_LUA_DEBUG_ENDPOINT"] = endpointPath;
            var since = DateTime.UtcNow;
            using (var process = Process.Start(start) ?? throw new InvalidOperationException("Editor did not start."))
            {
                StatusText.Text = "Starting the Editor...";
                for (int attempt = 0; attempt < 360 && !_disposed; ++attempt)
                {
                    if (process.HasExited) throw new InvalidOperationException("Editor exited before publishing its endpoint. Check Application.log.");
                    if (File.Exists(endpointPath) && File.GetLastWriteTimeUtc(endpointPath) >= since)
                    {
                        if (new FileInfo(endpointPath).Length > 65536) throw new InvalidDataException("Endpoint file exceeds 64 KiB.");
                        var endpoint = _json.Deserialize<Dictionary<string, object>>(File.ReadAllText(endpointPath));
                        EndpointPathTextBox.Text = endpointPath;
                        PortTextBox.Text = Convert.ToString(endpoint["port"]);
                        TokenTextBox.Password = Convert.ToString(endpoint["token"]);
                        await AttachAsync(cancellationToken); return;
                    }
                    await Task.Delay(250, cancellationToken);
                }
                if (!_disposed) throw new TimeoutException("Editor startup exceeded 90 seconds. Load its endpoint manually once startup completes.");
            }
        }
        private async Task AttachAsync(CancellationToken cancellationToken)
        {
            if (!int.TryParse(PortTextBox.Text, out int port) || port < 1 || port > 65535)
                throw new ArgumentException("Enter a valid port.");
            var token = TokenTextBox.Password;
            if (token.Length < 16) throw new ArgumentException("Load the Editor's session token.");
            var package = LuaDebuggerPackage.Instance ?? throw new InvalidOperationException("Package is unavailable.");
            var automation = await package.GetServiceAsync(typeof(Microsoft.VisualStudio.Shell.Interop.SDTE));
            await package.JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);
            var dte = automation as EnvDTE.DTE;
            if (dte == null) throw new InvalidOperationException("Visual Studio automation is unavailable.");
            if (dte.Debugger.CurrentMode != EnvDTE.dbgDebugMode.dbgDesignMode)
                throw new InvalidOperationException("Stop the active Visual Studio debug session before attaching Lua.");
            var adapter = Path.Combine(Path.GetDirectoryName(typeof(LuaDebuggerControl).Assembly.Location), "adapter", "WorkphoneLuaDebugAdapter.exe");
            if (!File.Exists(adapter)) throw new FileNotFoundException("Packaged Lua debug adapter is missing.", adapter);
            var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Workphone", "LuaDebugger");
            Directory.CreateDirectory(directory);
            if (_launchPath != null) File.Delete(_launchPath);
            _launchPath = Path.Combine(directory, "launch-" + Guid.NewGuid().ToString("N") + ".json");
            File.WriteAllText(_launchPath, _json.Serialize(new Dictionary<string, object> {
                { "$adapter", adapter }, { "$adapterArgs", port.ToString(System.Globalization.CultureInfo.InvariantCulture) },
                { "type", "workphone-lua" }, { "request", "attach" }, { "name", "Workphone Lua" },
                { "token", token }, { "stopOnEntry", StopOnEntry.IsChecked == true },
                { "$localsScopeName", "Locals" }
            }));
            dte.Commands.Raise("0ddba113-7ac1-4c6e-a2ef-dcac3f9e731e", 0x0101,
                "/LaunchJson:\"" + _launchPath + "\" /EngineGuid:\"8BC205D5-3836-4E05-A729-A6D15A6E3884\"", IntPtr.Zero);
            StatusText.Text = "Attach requested. Use Visual Studio's standard debugger windows.";
        }
        public void Dispose()
        {
            _disposed = true;
            _lifetime.Cancel();
            if (_launchPath != null) { try { File.Delete(_launchPath); } catch (IOException) { } }
        }
    }
}
