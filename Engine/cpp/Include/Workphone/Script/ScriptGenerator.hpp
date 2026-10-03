#ifndef ScriptGenerator_h__
#define ScriptGenerator_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Script/ScriptClass.hpp>
#include <Workphone/Script/ScriptFunction.hpp>

namespace workphone
{

    /**
     * @brief A versatile script generation utility for cross-language code generation
     *
     * The ScriptGenerator class provides comprehensive functionality for generating script files
     * from C++ classes across multiple target languages. It supports automated conversion of
     * C# source code to C++ equivalent structures, and can generate script bindings for
     * C++, C#, and Lua scripting environments.
     *
     * Key features:
     * - Cross-language script generation (C++, C#, Lua)
     * - C# to C++ code conversion and mapping
     * - Automatic class and function extraction from source files
     * - Configurable namespace and include management
     * - Template-based code generation with customizable output paths
     *
     * @note This class is designed to work with the workphone framework's type system
     *       and follows the ISharedObject interface pattern for memory management.
     */
    class WPCore_API ScriptGenerator : public ISharedObject
    {
    public:
        /**
         * @brief Constructs a ScriptGenerator with default configuration
         *
         * Initializes the generator with predefined type mappings for common
         * conversions between C# and C++ types, and sets up default include paths.
         */
        ScriptGenerator();

        /**
         * @brief Virtual destructor for proper cleanup of derived instances
         */
        ~ScriptGenerator() override;

        /**
         * @brief Creates a basic script template for the specified language at the given path
         *
         * @param type The target language for script generation
         * @param path The output file path where the generated script will be saved
         */
        void createScript( LanguageType type, const String &path );

        /**
         * @brief Creates a script from an existing ScriptClass definition
         *
         * @param type The target language for script generation
         * @param path The output file path where the generated script will be saved
         * @param pClass Smart pointer to a ScriptClass containing the class definition
         */
        void createScript( LanguageType type, const String &path, SmartPtr<ScriptClass> pClass );

        /**
         * @brief Retrieves the C++ equivalent class name for a given parent class
         *
         * Uses the internal class mapping to convert C# class names to their
         * corresponding C++ equivalents.
         *
         * @param parent The original class name (typically from C#)
         * @return String containing the mapped C++ class name, or original if no mapping exists
         */
        String getClassCPP( const String &parent );

        /**
         * @brief Converts C# source files to equivalent C++ implementations
         *
         * Recursively processes all C# files in the source directory and generates
         * corresponding C++ header and source files in the destination directory.
         *
         * @param csharpPath Source directory containing C# files to convert
         * @param cppPath Destination directory for generated C++ files
         */
        void convertCSharp( const String &csharpPath, const String &cppPath );

        /**
         * @brief Removes non-printable characters from a string, preserving spaces
         *
         * @param str The input string to clean
         * @return String with non-printable characters removed
         */
        String cleanString( const String &str );

        /**
         * @brief Extracts complete lines from a buffer string into an array
         *
         * @param buf The source buffer containing newline-separated content
         * @param lines Output array that will be populated with individual lines
         */
        void extractCompleteLines( String buf, Array<String> &lines );

        /**
         * @brief Removes C-style comments from source code
         *
         * Properly handles comment removal while preserving string literals
         * and nested comment structures.
         *
         * @param input The source code string containing comments
         * @return String with comments removed
         */
        String removeComments( const String &input );

        /**
         * @brief Determines if a line contains a C# attribute declaration
         *
         * @param line The source code line to analyze
         * @return true if the line contains a C# attribute (enclosed in [])
         */
        bool isCSharpAttrib( const String &line );

        /**
         * @brief Determines if a line contains a C# variable declaration
         *
         * @param line The source code line to analyze
         * @return true if the line contains a valid C# variable declaration
         */
        bool isCSharpVariable( const String &line );

        /**
         * @brief Determines if a line contains a C# function declaration
         *
         * @param line The source code line to analyze
         * @return true if the line contains a valid C# function signature
         */
        bool isCSharpFunction( const String &line );

        /**
         * @brief Extracts the function name from a function declaration line
         *
         * @param line The function declaration line
         * @return String containing the extracted function name
         */
        String getFunctionName( const String &line );

        /**
         * @brief Extracts all function definitions from a C# source file
         *
         * Parses the specified file and creates ScriptFunction objects for each
         * function found within the given class.
         *
         * @param filePath Path to the C# source file to parse
         * @param className Name of the class containing the functions
         * @return Array of ScriptFunction smart pointers representing parsed functions
         */
        Array<SmartPtr<ScriptFunction>> getScriptFunctions( const String &filePath,
                                                            const String &className );

        /**
         * @brief Recursively processes a folder structure to generate C++ equivalents
         *
         * @param folderListing Smart pointer to a folder explorer containing the source structure
         */
        void createSource( SmartPtr<IFolderExplorer> folderListing );

        /**
         * @brief Processes a single source file to generate C++ equivalent
         *
         * @param filePath Path to the individual source file to process
         */
        void createSource( const String &filePath );

        /**
         * @brief Creates a C++ struct from a C# struct declaration line
         *
         * @param line The C# struct declaration line
         * @param filePath Path to the source file containing the struct
         */
        void createStruct( const String &line, const String &filePath );

        /**
         * @brief Creates a C++ class from a C# class declaration line
         *
         * @param line The C# class declaration line
         * @param filePath Path to the source file containing the class
         */
        void createClass( const String &line, const String &filePath );

        /**
         * @brief Generates a Lua script class template with the specified name
         *
         * @param className Name of the class to generate
         * @return String containing the complete Lua script template
         */
        String createClass( const String &className );

        /**
         * @brief Creates a function template for Lua scripting
         *
         * @param className Name of the class containing the function
         * @param functionName Name of the function to create
         * @param parameters Array of parameter names for the function
         * @return String containing the generated function template
         */
        String createFunction( const String &className, const String &functionName,
                               const Array<String> &parameters );

        /**
         * @brief Generates a complete C++ script file with basic template structure
         *
         * @param path Output directory for the generated files
         * @param className Name of the class to generate
         */
        void createCPlusPlusScript( const String &path, const String &className );

        /**
         * @brief Writes script function definitions to an output stream
         *
         * @param scriptFunctions Array of script functions to write
         * @param stream Output stream for writing the function definitions
         */
        void writeScriptFunctions( Array<SmartPtr<ScriptFunction>> &scriptFunctions,
                                   std::ostream &stream );

        /**
         * @brief Generates class function templates for the specified class
         *
         * @param scriptFunctions Output array to populate with generated functions
         * @param className Name of the class for which to create functions
         */
        void createClassFunctions( Array<SmartPtr<ScriptFunction>> &scriptFunctions,
                                   const String &className );

        /**
         * @brief Gets the current filename replacement pattern
         *
         * @return String containing the pattern to be replaced in filenames
         */
        String getReplaceFileName() const;

        /**
         * @brief Sets the filename pattern to be replaced during generation
         *
         * @param fileNamePattern The pattern string to search for in filenames during replacement
         */
        void setReplaceFileName( const String &fileNamePattern );

        /**
         * @brief Gets the replacement filename pattern
         *
         * @return String containing the replacement pattern for filenames
         */
        String getReplacementFileName() const;

        /**
         * @brief Sets the replacement pattern for filename transformations
         *
         * @param replacementPattern The string to use as replacement for the search pattern
         */
        void setReplacementFileName( const String &replacementPattern );

        /**
         * @brief Generates a C++ class with simple template structure
         *
         * @param path Output directory for the generated class files
         * @param className Name of the class to generate
         */
        void createCPlusPlusClass( const String &path, const String &className );

        /**
         * @brief Generates a complete C++ class from a ScriptClass definition
         *
         * Creates both header (.h) and source (.cpp) files with proper namespace
         * handling, inheritance, and function implementations.
         *
         * @param path Output directory for the generated class files
         * @param pClass Smart pointer to ScriptClass containing the complete class definition
         */
        void createCPlusPlusClass( const String &path, SmartPtr<ScriptClass> pClass );

        /**
         * @brief Gets the current header include list
         *
         * @return Array of strings containing header file paths to include in generated headers
         */
        Array<String> getHeaderIncludes() const;

        /**
         * @brief Sets the list of header files to include in generated header files
         *
         * @param headerIncludes Array of header file paths to include in generated .h files
         */
        void setHeaderIncludes( const Array<String> &headerIncludes );

        /**
         * @brief Gets the current source include list
         *
         * @return Array of strings containing header file paths to include in generated source files
         */
        Array<String> getSourceIncludes() const;

        /**
         * @brief Sets the list of header files to include in generated source files
         *
         * @param sourceIncludes Array of header file paths to include in generated .cpp files
         */
        void setSourceIncludes( const Array<String> &sourceIncludes );

        /**
         * @brief Gets the current project root path
         *
         * @return String containing the base project directory path
         */
        String getProjectPath() const;

        /**
         * @brief Sets the base project directory for relative path resolution
         *
         * @param projectPath The root directory path for the project
         */
        void setProjectPath( const String &projectPath );

        /**
         * @brief Gets the current namespace hierarchy
         *
         * @return Array of strings representing the nested namespace structure
         */
        Array<String> getNamespaceNames() const;

        /**
         * @brief Sets the namespace hierarchy for generated C++ classes
         *
         * @param namespaceNames Array of namespace names in hierarchical order (outer to inner)
         */
        void setNamespaceNames( const Array<String> &namespaceNames );

        /**
         * @brief Adds a new class name mapping for type conversion
         *
         * Registers a mapping between a source language class name and its
         * target language equivalent for automatic type conversion.
         *
         * @param className The original class name (typically from C#)
         * @param newClassName The target class name (typically C++ equivalent)
         */
        void addMapEntry( const String &className, const String &newClassName );

        WP_CLASS_REGISTER_DECL;

    private:
        ///< Base project directory path
        String m_projectPath;

        ///< Replacement pattern for filename transformations
        String m_replacementFileName;

        ///< Source directory for input files
        String m_sourcePath;

        ///< Destination directory for generated files
        String m_destinationPath;

        ///< Pattern to search for in filename replacements
        String m_replaceFileName;

        ///< Precompiled header file path for generated sources
        String m_precompiledHeader;

        ///< List of includes for generated header files
        Array<String> m_headerIncludes;

        ///< List of includes for generated source files
        Array<String> m_sourceIncludes;

        ///< Hierarchy of namespaces for generated classes
        Array<String> m_namespaceNames;

        ///< Mapping between source and target class names
        Array<Pair<String, String>> m_classMap;
    };
}  // namespace workphone

#endif  // ScriptGenerator_h__
