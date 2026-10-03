
project(UnitTestsCSharp VERSION 0.1.0 LANGUAGES CSharp)

include(CSharpUtilities)

# define header and source files for the library
file(GLOB_RECURSE SOURCE_FILES "*.config" "*.cs" "*.xaml" "*.resx" "*.settings")

set(WP_HEADER_REL_PATH "${CMAKE_SOURCE_DIR}/Tools/UnitTestsCSharp")
set(WP_SOURCE_REL_PATH "${CMAKE_SOURCE_DIR}/Tools/UnitTestsCSharp")

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_HEADER_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Header Files\\${_group_file_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${SOURCE_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_SOURCE_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Source Files\\${_group_file_path}" FILES "${_source}")
endforeach()

add_executable(UnitTestsCSharp ${SOURCE_FILES})

#csharp_set_designer_cs_properties(
#	Properties/AssemblyInfo.cs
#	Properties/Resources.Designer.cs
#	Properties/Resources.resx
#	Properties/Settings.Designer.cs
#	Properties/Settings.settings)

csharp_set_xaml_cs_properties(
	App.xaml
	App.xaml.cs
	MainWindow.xaml
	MainWindow.xaml.cs)

set_property(SOURCE App.xaml PROPERTY VS_XAML_TYPE "ApplicationDefinition")

set_property(TARGET UnitTestsCSharp PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
set_property(TARGET UnitTestsCSharp PROPERTY WIN32_EXECUTABLE TRUE)

set_property(TARGET UnitTestsCSharp PROPERTY 
    VS_DOTNET_REFERENCE_InteractiveDataDisplay.WPF
    ${CMAKE_SOURCE_DIR}/Tools/UnitTestsCSharp/packages/InteractiveDataDisplay.WPF.1.0.0/lib/net452/InteractiveDataDisplay.WPF.dll)

set_property(TARGET UnitTestsCSharp PROPERTY 
    VS_DOTNET_REFERENCE_System.Data.SQLite
    ${CMAKE_SOURCE_DIR}/Tools/UnitTestsCSharp/packages/System.Data.SQLite.dll)



set_property(TARGET UnitTestsCSharp PROPERTY VS_DOTNET_REFERENCES
	"InteractiveDataDisplay.WPF"
	"Microsoft.CSharp"
	"Microsoft.Maps.MapControl.WPF"
	"Microsoft.VisualStudio.QualityTools.UnitTestFramework"
	"Newtonsoft.Json"
	"PresentationCore"
	"PresentationFramework"
	"System"
	"System.Core"
	"System.Data"
	"System.Data.DataSetExtensions"
	#"System.Data.SQLite"
	"System.Net.Http"
	"System.Numerics"
	"System.Numerics.Vectors"
	"System.Xaml"
	"System.Xml"
	"System.Xml.Linq"
	"System.Web.Extensions"
	"System.Windows.Forms"
	"WindowsBase")

set_target_properties (UnitTestsCSharp PROPERTIES FOLDER Tests/CSharp)