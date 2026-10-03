
project(UnitTestsCppCli VERSION 0.1.0 LANGUAGES CXX)

include(CSharpUtilities)

# define header and source files for the library
file(GLOB_RECURSE SOURCE_FILES "*.config" "*.c" "*.cpp" "*.cs" "*.xaml" "*.resx" "*.settings")

set(WP_HEADER_REL_PATH "${CMAKE_SOURCE_DIR}/Tests/cpp_cli/UnitTests")
set(WP_SOURCE_REL_PATH "${CMAKE_SOURCE_DIR}/Tests/cpp_cli/UnitTests")

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

add_executable(UnitTestsCppCli ${SOURCE_FILES})


set_target_properties(${PROJECT_NAME} PROPERTIES COMMON_LANGUAGE_RUNTIME "")
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6")
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DOTNET_REFERENCES "System")
#set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/clr /EHa")
string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

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

set_property(TARGET UnitTestsCppCli PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
set_property(TARGET UnitTestsCppCli PROPERTY WIN32_EXECUTABLE TRUE)

set_property(TARGET UnitTestsCppCli PROPERTY 
    VS_DOTNET_REFERENCE_InteractiveDataDisplay.WPF
    ${CMAKE_SOURCE_DIR}/Tools/UnitTestsCppCli/packages/InteractiveDataDisplay.WPF.1.0.0/lib/net452/InteractiveDataDisplay.WPF.dll)

set_property(TARGET UnitTestsCppCli PROPERTY 
    VS_DOTNET_REFERENCE_System.Data.SQLite
    ${CMAKE_SOURCE_DIR}/Tools/UnitTestsCppCli/packages/System.Data.SQLite.dll)



set_property(TARGET UnitTestsCppCli PROPERTY VS_DOTNET_REFERENCES
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

set_target_properties (UnitTestsCppCli PROPERTIES FOLDER Tests/CppCli)