using Microsoft.VisualStudio.Shell;
using System;
using System.Runtime.InteropServices;
using System.Threading;
using Task = System.Threading.Tasks.Task;

namespace LuaDebuggerVsix
{
    [PackageRegistration(UseManagedResourcesOnly = true, AllowsBackgroundLoading = true)]
    [Guid(PackageGuidString)]
    [ProvideToolWindow(typeof(LuaDebuggerWindow))]
    public sealed class LuaDebuggerPackage : AsyncPackage
    {
        public const string PackageGuidString = "7C9E5E2A-0F9E-45FA-9B8B-61D0717049B2";

        protected override async Task InitializeAsync(
            CancellationToken cancellationToken,
            IProgress<ServiceProgressData> progress)
        {
            await JoinableTaskFactory.SwitchToMainThreadAsync(cancellationToken);

            await LuaDebuggerWindowCommand.InitializeAsync(this);
        }
    }
}