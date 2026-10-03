using Microsoft.VisualStudio.Shell;
using System.Runtime.InteropServices;

namespace LuaDebuggerVsix
{
    [Guid("A78D6D02-921B-4D5A-A7E8-89DCA16D9D90")]
    public sealed class LuaDebuggerWindow : ToolWindowPane
    {
        public LuaDebuggerWindow() : base(null)
        {
            Caption = "Lua Debugger";
            Content = new LuaDebuggerControl();
        }
    }
}