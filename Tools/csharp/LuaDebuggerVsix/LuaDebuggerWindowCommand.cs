using Microsoft.VisualStudio.Shell;
using System;
using System.ComponentModel.Design;
using System.Threading;
using System.Threading.Tasks;

namespace LuaDebuggerVsix
{
    internal static class LuaDebuggerWindowCommand
    {
        internal static async Task InitializeAsync(AsyncPackage package)
        {
            var commands = await package.GetServiceAsync(typeof(IMenuCommandService)) as OleMenuCommandService;
            await package.JoinableTaskFactory.SwitchToMainThreadAsync();
            if (commands == null) throw new InvalidOperationException("Visual Studio command service is unavailable.");
            var id = new CommandID(new Guid("2EBFD2BD-111F-4E2D-A607-4E6D8474D40C"), 0x0100);
            commands.AddCommand(new MenuCommand((sender, args) =>
            {
                package.JoinableTaskFactory.RunAsync(async () =>
                {
                    try { await package.ShowToolWindowAsync(typeof(LuaDebuggerWindow), 0, true, CancellationToken.None); }
                    catch (Exception error) { ActivityLog.LogError("Workphone Lua Debugger", error.ToString()); }
                });
            }, id));
        }
    }
}
