#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ScriptGenerator.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Core/ParameterUtil.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IFolderExplorer.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Script/ScriptVariable.hpp>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <cctype>
#include <filesystem>

using std::endl;
using std::ofstream;

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ScriptGenerator, ISharedObject );

    auto ScriptGenerator::getClassCPP( const String &parent ) -> String
    {
        for( auto &map : m_classMap )
        {
            if( map.first == parent )
            {
                return map.second;
            }
        }

        return parent;
    }

    ScriptGenerator::ScriptGenerator()
    {
        m_classMap.emplace_back( "MonoBehaviour", "scene::BaseComponent" );
        m_classMap.emplace_back( "ScriptableObject", "CSharedObject<ISharedObject>" );

        m_classMap.emplace_back( "string", "String" );

        m_classMap.emplace_back( "Object", "SmartPtr<ISharedObject>" );
        m_classMap.emplace_back( "GameObject", "SmartPtr<scene::IGameActor>" );
        m_classMap.emplace_back( "UnityEngine.Object", "SmartPtr<ISharedObject>" );

        m_classMap.emplace_back( "int", "s32" );
        m_classMap.emplace_back( "float", "f32" );
        m_classMap.emplace_back( "double", "f64" );

        m_classMap.emplace_back( "List<int>", "Array<s32>" );
        m_classMap.emplace_back( "List<float>", "Array<f32>" );
        m_classMap.emplace_back( "List<string>", "Array<String>" );
        m_classMap.emplace_back( "List<Object>", "Array<Object>" );
        m_classMap.emplace_back( "List<Vector2>", "Array<Vector2<real_Num>>" );
        m_classMap.emplace_back( "List<Vector3>", "Array<Vector3<real_Num>>" );
        m_classMap.emplace_back( "List<Quaternion>", "Array<Quaternion<real_Num>>" );
        m_classMap.emplace_back( "List<GameObject>", "Array<SmartPtr<scene::IGameActor>>" );

        m_classMap.emplace_back( "string[]", "Array<String>" );
        m_classMap.emplace_back( "int[]", "Array<s32>" );
        m_classMap.emplace_back( "float[]", "Array<f32>" );
        m_classMap.emplace_back( "Object[]", "Array<SmartPtr<ISharedObject>>" );
        m_classMap.emplace_back( "Vector2[]", "Array<Vector2<real_Num>>" );
        m_classMap.emplace_back( "Vector3[]", "Array<Vector3<real_Num>>" );
        m_classMap.emplace_back( "Quaternion[]", "Array<Quaternion<real_Num>>" );
        m_classMap.emplace_back( "GameObject[]", "Array<SmartPtr<scene::IGameActor>>" );

        m_classMap.emplace_back( "System.Type", "TypeInfo" );
        m_classMap.emplace_back( "Vector2", "Vector2<real_Num>" );
        m_classMap.emplace_back( "Vector3", "Vector3<real_Num>" );
        m_classMap.emplace_back( "Quaternion", "Quaternion<real_Num>" );
        m_classMap.emplace_back( "Matrix4x4", "Matrix4F" );
        m_classMap.emplace_back( "Transform", "SmartPtr<ITransform>" );

        m_precompiledHeader = "WPHoudini/WPHoudiniPCH.h";

        m_headerIncludes.emplace_back( "WPHoudini/WPHoudiniPrerequisites.h" );
        m_headerIncludes.emplace_back( "WPApplication/Components/BaseComponent.h" );

        m_sourceIncludes.emplace_back( "Workphone/WorkphoneHeaders.h" );
        m_sourceIncludes.emplace_back( "Workphone/WorkphoneInterface.hpp" );
        m_sourceIncludes.emplace_back( "WPApplication/WPApplicationHeaders.h" );
    }

    ScriptGenerator::~ScriptGenerator() = default;

    void ScriptGenerator::createScript( LanguageType type, const String &path )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        switch( type )
        {
        case LanguageType::LUA:
        {
            auto sourceStr = String();

            auto className = std::filesystem::u8path( path ).stem().u8string();
            for( auto &character : className )
                if( !std::isalnum( static_cast<unsigned char>( character ) ) && character != '_' ) character = '_';
            if( className.empty() ) className = "NewScript";
            if( std::isdigit( static_cast<unsigned char>( className[0] ) ) ) className.insert( 0, "Script_" );

            sourceStr = "class '" + className +
                        "'\n"
                        "\n";

            sourceStr += "function " + className +
                         ":__init(window)\n"
                         "end\n"
                         "\n"
                         "function " +
                         className +
                         ":__finalize()\n"
                         "end\n"
                         "\n"
                         "function " +
                         className +
                         ":load()\n"
                         "end\n"
                         "\n"
                         "function " +
                         className +
                         ":unload()\n"
                         "end\n"
                         "\n"
                         "function " +
                         className +
                         ":initialise()\n"
                         "end\n"
                         "\n"
                         "function " +
                         className +
                         ":update()\n"
                         "end\n"
                         "\n"
                         "function " +
                         className +
                         ":show()\n"
                         "end\n"
                         "\n"
                         "function " +
                         className +
                         ":hide()\n"
                         "end";

            auto sourcePath = Path::getFilePath( path );
            fileSystem->createDirectories( sourcePath );

            fileSystem->writeAllText( path, sourceStr );
        }
        break;
        default:
        {
        }
        }
    }

    void ScriptGenerator::createScript( LanguageType type, const String &path,
                                        SmartPtr<ScriptClass> pClass )
    {
    }

    void ScriptGenerator::convertCSharp( const String &csharpPath, const String &cppPath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        m_sourcePath = csharpPath;
        m_destinationPath = cppPath;

        if( auto folderListing = fileSystem->getFolderListing( csharpPath ) )
        {
            createSource( folderListing );
        }
    }

    auto ScriptGenerator::cleanString( const String &str ) -> String
    {
        auto line = str;
        line.erase( std::remove_if( line.begin(), line.end(),
                                    []( char c ) { return c > -1 && !std::isgraph( c ) && c != ' '; } ),
                    line.end() );
        return line;
    }

    void ScriptGenerator::extractCompleteLines( String buf, Array<String> &lines )
    {
        String::size_type pos;
        while( ( pos = buf.find( '\n' ) ) != String::npos )
        {
            auto line = buf.substr( 0, pos );
            lines.push_back( line );
            buf.erase( 0, pos + 1 );
        }
    }

    auto ScriptGenerator::removeComments( const String &inputStr ) -> String
    {
        std::stringstream write;
        Array<String> lines;
        extractCompleteLines( inputStr, lines );

        u32 number = 0, position, flag1 = 0, flag2 = 0,
            flag3 = 0;  // I am using flags to check the different conditions we encounter

        for( auto s : lines )
        {
            unsigned int i;
            String s1;
            position = static_cast<u32>( s.size() );
            flag2 = 0;
            flag3 = 0;
            for( i = 0; i < s.size(); i++ )
            {
                // this checks for the double quotes "
                if( s[i] == '"' && flag1 == 0 && flag2 == 0 && number == 0 )
                {
                    flag3 = 1;
                    number = 1;
                    s1 += '"';
                    i++;
                }
                if( s[i] == '"' && flag1 == 0 && flag2 == 0 && number == 1 )
                {
                    flag3 = 0;
                    number = 0;
                    s1 += '"';
                    i++;
                }

                // this checks for Double-slash //
                if( s[i] == '/' && s[i + 1] == '/' && flag3 == 0 && flag1 == 0 )
                {
                    flag2 = 1;
                    position = i;
                }

                // This checks for slash-star /*
                if( s[i] == '/' && s[i + 1] == '*' && flag2 == 0 && flag3 == 0 )
                {
                    flag1 = 1;
                    i += 2;
                }
                if( s[i] == '*' && s[i + 1] == '/' && flag2 == 0 && flag3 == 0 )
                {
                    flag1 = 0;
                    i += 2;
                    if( s[i] == '/' && s[i + 1] == '*' )
                    {
                        flag1 = 1;
                    }
                }

                if( flag1 == 0 )
                {
                    if( i < position )
                    {
                        s1 += s[i];
                    }
                }
            }
            write << s1 << endl;
        }

        return write.str().c_str();
    }

    auto ScriptGenerator::isCSharpAttrib( const String &line ) -> bool
    {
        if( line.find( "[" ) != String::npos && line.find( "]" ) != String::npos )
        {
            auto attribStr = cleanString( line );
            if( attribStr.front() == '[' && attribStr.back() == ']' )
            {
                return true;
            }
        }

        return false;
    }

    auto ScriptGenerator::isCSharpVariable( const String &line ) -> bool
    {
        if( line.find( ";" ) != String::npos )
        {
            auto variableString = StringUtil::replaceAll( line, ";", "" );
            variableString = cleanString( variableString );

            auto variableDecArray = StringUtil::split( variableString, "=", 30 );
            if( !variableDecArray.empty() )
            {
                auto variableArray = StringUtil::split( variableDecArray[0], " ", 30 );
                if( variableArray.size() >= 2 )
                {
                    auto variableIndex = 0;
                    auto variableType = variableArray[0];

                    // check for keywords
                    if( variableType.find( "public" ) != String::npos )
                    {
                        variableIndex++;
                    }

                    if( variableIndex < variableArray.size() )
                    {
                        variableType = variableArray[variableIndex];

                        // check for keywords
                        if( variableType.find( "const" ) != String::npos )
                        {
                            variableIndex++;
                        }
                    }

                    if( variableIndex < variableArray.size() )
                    {
                        auto variableType = variableArray[variableIndex];
                        auto variableName = variableArray.back();

                        if( !StringUtil::isNullOrEmpty( variableType ) &&
                            !StringUtil::isNullOrEmpty( variableName ) )
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    auto ScriptGenerator::isCSharpFunction( const String &line ) -> bool
    {
        if( isCSharpAttrib( line ) )
        {
            return false;
        }
        if( isCSharpVariable( line ) )
        {
            return false;
        }
        if( line.find( "();" ) != String::npos )
        {
            return false;
        }
        if( line.find( "=" ) != String::npos )
        {
            return false;
        }
        if( line.find( "if" ) != String::npos )
        {
            return false;
        }
        if( line.find( "else if" ) != String::npos )
        {
            return false;
        }
        if( line.find( "#if" ) != String::npos )
        {
            return false;
        }
        if( line.find( "#elif" ) != String::npos )
        {
            return false;
        }
        if( line.find( "namespace" ) != String::npos )
        {
            return false;
        }

        return line.find( '(' ) != String::npos && line.find( ')' ) != String::npos;
    }

    auto ScriptGenerator::getFunctionName( const String &line ) -> String
    {
        auto openBracketPos = line.find( '(' );
        return line;
    }

    auto ScriptGenerator::getScriptFunctions( const String &filePath, const String &className )
        -> Array<SmartPtr<ScriptFunction>>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto fileSystem = applicationManager->getFileSystem();

        auto classStr = fileSystem->readAllText( filePath );
        classStr = removeComments( classStr );
        classStr = StringUtil::trim( classStr );

        Array<String> lines;
        extractCompleteLines( classStr, lines );

        Array<SmartPtr<ScriptFunction>> functions;

        for( auto &line : lines )
        {
            if( isCSharpFunction( line ) )
            {
                auto functionSig = cleanString( line );
                auto functionArgStartPos = functionSig.find( '(' );
                auto functionArgEndPos = functionSig.find( ')' );

                auto functionDec = functionSig.substr( 0, functionArgStartPos );
                auto functionArgs = functionSig.substr( functionArgStartPos, functionArgEndPos );
                functionArgs = StringUtil::replaceAll( functionArgs, "(", "" );
                functionArgs = StringUtil::replaceAll( functionArgs, ")", "" );

                auto functionDecArray = StringUtil::split( functionDec, " ", 30 );
                if( functionDecArray.size() >= 2 )
                {
                    auto returnType = functionDecArray[functionDecArray.size() - 2];

                    if( !StringUtil::isNullOrEmpty( m_replaceFileName ) )
                    {
                        returnType = StringUtil::replaceAll( returnType, m_replaceFileName,
                                                             m_replacementFileName );
                    }

                    auto functionName = functionDecArray.back();

                    auto functionArgsArray = StringUtil::split( functionArgs, ",", 30 );
                    Array<SmartPtr<IScriptVariable>> args;

                    for( auto arg : functionArgsArray )
                    {
                        auto argArray = StringUtil::split( arg, " ", 30 );
                        if( argArray.size() >= 2 )
                        {
                            auto argType = argArray[argArray.size() - 2];

                            if( !StringUtil::isNullOrEmpty( m_replaceFileName ) )
                            {
                                argType = StringUtil::replaceAll( argType, m_replaceFileName,
                                                                  m_replacementFileName );
                            }

                            auto argName = argArray.back();

                            auto pFucntionArg = factoryManager->make_ptr<ScriptVariable>();
                            pFucntionArg->setType( ParameterUtil::getParamTypeFromString( argType ) );
                            pFucntionArg->setName( argName );
                            args.emplace_back( pFucntionArg );
                        }
                    }

                    auto pFunction = factoryManager->make_ptr<ScriptFunction>();

                    returnType = getClassCPP( returnType );
                    pFunction->setReturnType( ParameterUtil::getParamTypeFromString( returnType ) );
                    pFunction->setFunctionName( functionName );
                    pFunction->setArguments( args );
                    functions.push_back( pFunction );
                }
            }
        }

        return functions;
    }

    void ScriptGenerator::createSource( const String &filePath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            static const auto csharpExtension = ".cs";
            auto fileExtension = Path::getFileExtension( filePath );
            auto fileNameWithoutExtension = Path::getFileNameWithoutExtension( filePath );

            if( filePath.find( "HAPI_Transform" ) != String::npos )
            {
                int stop = 0;
                stop = 0;
            }

            if( fileExtension == csharpExtension )
            {
                auto classStr = fileSystem->readAllText( filePath );
                classStr = removeComments( classStr );
                classStr = StringUtil::trim( classStr );

                Array<String> lines;
                extractCompleteLines( classStr, lines );

                for( auto &line : lines )
                {
                    auto classPos = line.find( "class" );
                    if( classPos != String::npos )
                    {
                        createClass( line, filePath );
                    }

                    auto structPos = line.find( "struct" );
                    if( structPos != String::npos )
                    {
                        createStruct( line, filePath );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ScriptGenerator::createSource( SmartPtr<IFolderExplorer> folderListing )
    {
        auto folderName = folderListing->getFolderName();
        if( folderName.find( "HAPI" ) != String::npos )
        {
            int stop = 0;
            stop = 0;
        }

        auto files = folderListing->getFiles();
        for( auto file : files )
        {
            createSource( file );
        }

        auto subFolders = folderListing->getSubFolders();
        for( auto subFolder : subFolders )
        {
            createSource( subFolder );
        }
    }

    void ScriptGenerator::createStruct( const String &line, const String &filePath )
    {
        auto classPos = line.find( "struct" );
        auto className = String( "" );
        auto fileNameWithoutExtension = Path::getFileNameWithoutExtension( filePath );

        auto cppClassName = String( "" );

        if( !StringUtil::isNullOrEmpty( m_replaceFileName ) )
        {
            cppClassName = StringUtil::replaceAll( fileNameWithoutExtension, m_replaceFileName,
                                                   m_replacementFileName );
        }
        else
        {
            cppClassName = fileNameWithoutExtension;
        }

        auto classDecSubStr = line.substr( classPos, String::npos );
        auto classDecEnd = classDecSubStr.find( "{" );
        auto classDec = classDecSubStr.substr( 0, classDecEnd );
        auto newLinePos = classDec.find( "\n" );

        Array<String> newClassParentClasses;

        auto colon = classDec.find( ":" );
        if( colon == String::npos )
        {
            className = classDec.substr( 0, newLinePos );
            className = StringUtil::replaceAll( className, "struct ", "" );
            className = StringUtil::trim( className );
        }
        else
        {
            className = classDec.substr( 0, colon );
            className = StringUtil::replaceAll( className, "struct ", "" );
            className = StringUtil::replaceAll( className, ":", "" );
            className = StringUtil::trim( className );

            auto parentClasses = classDec.substr( colon + 1, String::npos );
            parentClasses = StringUtil::trim( parentClasses );
            if( parentClasses.find( '<' ) != String::npos && parentClasses.find( '>' ) != String::npos )
            {
                // todo deal with template args
                newClassParentClasses.push_back( parentClasses );
            }
            else
            {
                auto parentClassArray = StringUtil::split( parentClasses, ", ", 10 );
                for( auto parentClassName : parentClassArray )
                {
                    newClassParentClasses.push_back( parentClassName );
                }
            }
        }

        if( StringUtil::isNullOrEmpty( className ) )
        {
            return;
        }

        if( className.find( ' ' ) != String::npos )
        {
            return;
        }

        if( !StringUtil::isNullOrEmpty( m_replaceFileName ) )
        {
            cppClassName = StringUtil::replaceAll( className, m_replaceFileName, m_replacementFileName );
        }
        else
        {
            cppClassName = className;
        }

        auto pClass = workphone::make_ptr<ScriptClass>();
        pClass->setClassName( cppClassName );
        pClass->setParentClasses( newClassParentClasses );

        pClass->setNamespaceNames( m_namespaceNames );

        Array<SmartPtr<IScriptFunction>> functions;

        auto pConstructor = workphone::make_ptr<ScriptFunction>();
        pConstructor->setFunctionName( cppClassName );
        pConstructor->setConstructor( true );
        functions.emplace_back( pConstructor );

        auto pDestructor = workphone::make_ptr<ScriptFunction>();
        pDestructor->setFunctionName( "~" + cppClassName );
        pDestructor->setDestructor( true );
        functions.emplace_back( pDestructor );

        auto memberFunctions = getScriptFunctions( filePath, className );
        functions.insert( functions.end(), memberFunctions.begin(), memberFunctions.end() );

        pClass->setFunctions( functions );

        createCPlusPlusClass( m_destinationPath, pClass );
    }

    auto ScriptGenerator::createClass( const String &className ) -> String
    {
        std::stringstream scriptStream;

        scriptStream << String( "--" ) << className.c_str() << ".lua"
                     << "\n";
        scriptStream << String( "class " ) << "'" << className.c_str() << "'\n";

        Array<SmartPtr<ScriptFunction>> scriptFunctions;
        createClassFunctions( scriptFunctions, className );

        // write functions
        writeScriptFunctions( scriptFunctions, scriptStream );

        return String( scriptStream.str().c_str() );
    }

    void ScriptGenerator::createClass( const String &line, const String &filePath )
    {
        auto classPos = line.find( "class" );
        auto className = String( "" );
        auto fileNameWithoutExtension = Path::getFileNameWithoutExtension( filePath );

        auto cppClassName = String( "" );

        if( !StringUtil::isNullOrEmpty( m_replaceFileName ) )
        {
            cppClassName = StringUtil::replaceAll( fileNameWithoutExtension, m_replaceFileName,
                                                   m_replacementFileName );
        }
        else
        {
            cppClassName = fileNameWithoutExtension;
        }

        auto classDecSubStr = line.substr( classPos, String::npos );
        auto classDecEnd = classDecSubStr.find( "{" );
        auto classDec = classDecSubStr.substr( 0, classDecEnd );
        auto newLinePos = classDec.find( "\n" );

        Array<String> newClassParentClasses;

        auto colon = classDec.find( ":" );
        if( colon == String::npos )
        {
            className = classDec.substr( 0, newLinePos );
            className = StringUtil::replaceAll( className, "class ", "" );
            className = StringUtil::trim( className );
        }
        else
        {
            className = classDec.substr( 0, colon );
            className = StringUtil::replaceAll( className, "class ", "" );
            className = StringUtil::replaceAll( className, ":", "" );
            className = StringUtil::trim( className );

            auto parentClasses = classDec.substr( colon + 1, String::npos );
            parentClasses = StringUtil::trim( parentClasses );
            if( parentClasses.find( '<' ) != String::npos && parentClasses.find( '>' ) != String::npos )
            {
                // todo deal with template args
                newClassParentClasses.push_back( parentClasses );
            }
            else
            {
                auto parentClassArray = StringUtil::split( parentClasses, ", ", 10 );
                for( auto parentClassName : parentClassArray )
                {
                    newClassParentClasses.push_back( parentClassName );
                }
            }
        }

        if( StringUtil::isNullOrEmpty( className ) )
        {
            return;
        }

        if( className.find( ' ' ) != String::npos )
        {
            return;
        }

        if( !StringUtil::isNullOrEmpty( m_replaceFileName ) )
        {
            cppClassName = StringUtil::replaceAll( className, m_replaceFileName, m_replacementFileName );
        }
        else
        {
            cppClassName = className;
        }

        auto pClass = workphone::make_ptr<ScriptClass>();
        pClass->setClassName( cppClassName );
        pClass->setParentClasses( newClassParentClasses );

        pClass->setNamespaceNames( m_namespaceNames );

        Array<SmartPtr<IScriptFunction>> functions;

        auto pConstructor = workphone::make_ptr<ScriptFunction>();
        pConstructor->setFunctionName( cppClassName );
        pConstructor->setConstructor( true );
        functions.emplace_back( pConstructor );

        auto pDestructor = workphone::make_ptr<ScriptFunction>();
        pDestructor->setFunctionName( "~" + cppClassName );
        pDestructor->setDestructor( true );
        functions.emplace_back( pDestructor );

        auto memberFunctions = getScriptFunctions( filePath, className );
        functions.insert( functions.end(), memberFunctions.begin(), memberFunctions.end() );

        pClass->setFunctions( functions );

        createCPlusPlusClass( m_destinationPath, pClass );
    }

    auto ScriptGenerator::createFunction( const String &className, const String &functionName,
                                          const Array<String> &parameters ) -> String
    {
        SmartPtr<ScriptFunction> function( new ScriptFunction );
        function->setClassName( className );
        function->setFunctionName( functionName );
        // function->setParameters(parameters);

        Array<SmartPtr<ScriptFunction>> scriptFunctions;
        scriptFunctions.push_back( function );

        std::stringstream scriptStream;
        writeScriptFunctions( scriptFunctions, scriptStream );

        return String( scriptStream.str().c_str() );
    }

    void ScriptGenerator::createCPlusPlusScript( const String &path, const String &className )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        const auto newLineStr = String( "\n" );

        auto headerStr = static_cast<String>( "#ifndef " + className + "_h__\n" );
        headerStr += static_cast<String>( "#define " + className + "_h__\n" );
        headerStr += newLineStr;
        headerStr += "#include <IUpdateable.hpp>" + newLineStr;
        headerStr += "#include <InterfaceIds.hpp>" + newLineStr;
        headerStr += newLineStr;

        headerStr +=
            "class " + className + " : public TInterface<IID_IUPDATEABLE, IUpdateable>" + newLineStr;
        headerStr += "{" + newLineStr;
        headerStr += "public:" + newLineStr;
        headerStr += "	" + className + "();" + newLineStr;
        headerStr += "	~" + className + "();" + newLineStr;
        headerStr += newLineStr;
        headerStr += "	void update();" + newLineStr;
        headerStr += "};" + newLineStr;

        headerStr += newLineStr;
        headerStr += static_cast<String>( "#endif // " + className + "_h__\n" );

        auto headerFilePath = path + "/" + className + ".h";

        auto headerPath = Path::getFilePath( headerFilePath );
        fileSystem->createDirectories( headerPath );

        fileSystem->writeAllText( headerFilePath, headerStr );

        auto sourceStr = "#include \"" + className + ".h\"" + newLineStr;
        sourceStr += "#include <RuntimeObjectSystem/ObjectInterfacePerModule.hpp>" + newLineStr;
        sourceStr += "#include <RuntimeObjectSystem/IObject.hpp>" + newLineStr;
        sourceStr += "#include <IUpdateable.hpp>" + newLineStr;
        sourceStr += "#include <InterfaceIds.hpp>" + newLineStr;
        sourceStr += "#include <iostream>" + newLineStr;

        sourceStr += "#include <Workphone/WorkphoneHeaders.hpp>" + newLineStr;
        sourceStr += "#include <Workphone/WorkphoneHeaders.hpp>" + newLineStr;
        sourceStr += "#include <WPApplication/WPApplicationHeaders.hpp>" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "" + className + "::" + className + "()" + newLineStr;
        sourceStr += "{" + newLineStr;
        sourceStr += "}" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "" + className + "::~" + className + "()" + newLineStr;
        sourceStr += "{" + newLineStr;
        sourceStr += "}" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "void " + className + "::update()" + newLineStr;
        sourceStr += "{" + newLineStr;
        sourceStr += "}" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "REGISTERCLASS(" + className + ");" + newLineStr;

        auto sourceFilePath = path + "/" + className + ".cpp";

        auto sourcePath = Path::getFilePath( sourceFilePath );
        fileSystem->createDirectories( sourcePath );

        fileSystem->writeAllText( sourceFilePath, sourceStr );
    }

    void ScriptGenerator::writeScriptFunctions( Array<SmartPtr<ScriptFunction>> &scriptFunctions,
                                                std::ostream &stream )
    {
        for( auto &scriptFunc : scriptFunctions )
        {
            stream << "-"
                   << "\n";

            stream << "function " << scriptFunc->getClassName().c_str() << ":"
                   << scriptFunc->getFunctionName().c_str() << "(";

            Array<String> parameters;  // = scriptFunc->getParameters();
            for( u32 argIdx = 0; argIdx < parameters.size(); ++argIdx )
            {
                const auto &paramName = parameters[argIdx];
                auto pParamName = paramName.c_str() ? paramName.c_str() : "";
                stream << pParamName;

                if( argIdx != parameters.size() - 1 )
                {
                    stream << ", ";
                }
            }

            stream << ")"
                   << "\n";

            stream << "end"
                   << "\n";

            stream << "\n";
            stream << "\n";
            stream << "\n";
        }
    }

    void ScriptGenerator::createClassFunctions( Array<SmartPtr<ScriptFunction>> &scriptFunctions,
                                                const String &className )
    {
        // constructor
        Array<String> params( 1 );
        params.emplace_back( "object" );

        SmartPtr<ScriptFunction> scriptFunction( new ScriptFunction );
        scriptFunctions.push_back( scriptFunction );
        scriptFunction->setClassName( className );
        scriptFunction->setFunctionName( "__init" );
        // scriptFunction->setParameters(params);
    }

    auto ScriptGenerator::getReplaceFileName() const -> String
    {
        return m_replaceFileName;
    }

    void ScriptGenerator::setReplaceFileName( const String &replaceFileName )
    {
        m_replaceFileName = replaceFileName;
    }

    auto ScriptGenerator::getReplacementFileName() const -> String
    {
        return m_replacementFileName;
    }

    void ScriptGenerator::setReplacementFileName( const String &replacementFileName )
    {
        m_replacementFileName = replacementFileName;
    }

    void ScriptGenerator::createCPlusPlusClass( const String &path, const String &className )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        const auto newLineStr = String( "\n" );

        auto headerStr = static_cast<String>( "#ifndef " + className + "_h__\n" );
        headerStr += static_cast<String>( "#define " + className + "_h__\n" );
        headerStr += newLineStr;
        headerStr += "#include <IUpdateable.hpp>" + newLineStr;
        headerStr += "#include <InterfaceIds.hpp>" + newLineStr;
        headerStr += newLineStr;

        headerStr +=
            "class " + className + " : public TInterface<IID_IUPDATEABLE, IUpdateable>" + newLineStr;
        headerStr += "{" + newLineStr;
        headerStr += "public:" + newLineStr;
        headerStr += "	" + className + "();" + newLineStr;
        headerStr += "	~" + className + "();" + newLineStr;
        headerStr += newLineStr;
        headerStr += "	void update();" + newLineStr;
        headerStr += "};" + newLineStr;

        headerStr += newLineStr;
        headerStr += static_cast<String>( "#endif // " + className + "_h__\n" );

        auto headerFilePath = path + "/" + className + ".h";

        auto headerPath = Path::getFilePath( headerFilePath );
        fileSystem->createDirectories( headerPath );

        fileSystem->writeAllText( headerFilePath, headerStr );

        auto sourceStr = "#include \"" + className + ".h\"" + newLineStr;
        sourceStr += "#include <RuntimeObjectSystem/ObjectInterfacePerModule.hpp>" + newLineStr;
        sourceStr += "#include <RuntimeObjectSystem/IObject.hpp>" + newLineStr;
        sourceStr += "#include <IUpdateable.hpp>" + newLineStr;
        sourceStr += "#include <InterfaceIds.hpp>" + newLineStr;
        sourceStr += "#include <iostream>" + newLineStr;

        sourceStr += "#include <Workphone/WorkphoneHeaders.hpp>" + newLineStr;
        sourceStr += "#include <Workphone/WorkphoneHeaders.hpp>" + newLineStr;
        sourceStr += "#include <WPApplication/WPApplicationHeaders.hpp>" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "" + className + "::" + className + "()" + newLineStr;
        sourceStr += "{" + newLineStr;
        sourceStr += "}" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "" + className + "::~" + className + "()" + newLineStr;
        sourceStr += "{" + newLineStr;
        sourceStr += "}" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "void " + className + "::update()" + newLineStr;
        sourceStr += "{" + newLineStr;
        sourceStr += "}" + newLineStr;
        sourceStr += newLineStr;
        sourceStr += "REGISTERCLASS(" + className + ");" + newLineStr;

        auto sourceFilePath = path + "/" + className + ".cpp";

        auto sourcePath = Path::getFilePath( sourceFilePath );
        fileSystem->createDirectories( sourcePath );

        fileSystem->writeAllText( sourceFilePath, sourceStr );
    }

    void ScriptGenerator::createCPlusPlusClass( const String &path, SmartPtr<ScriptClass> pClass )
    {
        auto className = pClass->getClassName();
        auto namespaceNames = pClass->getNamespaceNames();
        auto functions = pClass->getFunctions();
        auto parentClasses = pClass->getParentClasses();

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        const auto newLineStr = String( "\n" );

        auto singleTabStr = String( "\t" );
        auto headerTabStr = String( "" );
        auto headerStr = static_cast<String>( "#ifndef " + className + "_h__\n" );
        headerStr += static_cast<String>( "#define " + className + "_h__\n" );
        headerStr += newLineStr;
        for( auto header : m_headerIncludes )
        {
            headerStr += "#include <" + header + ">" + newLineStr;
        }
        headerStr += newLineStr;

        if( !namespaceNames.empty() )
        {
            for( auto namespaceName : namespaceNames )
            {
                headerStr += headerTabStr + "namespace " + namespaceName + newLineStr;
                headerStr += headerTabStr + "{" + newLineStr;
                headerTabStr = headerTabStr + singleTabStr;
            }
        }

        headerStr += newLineStr;

        if( !parentClasses.empty() )
        {
            headerStr += headerTabStr + "class " + className + " : ";

            if( parentClasses.size() > 1 )
            {
                for( size_t parentClassIdx = 0; parentClassIdx < parentClasses.size(); ++parentClassIdx )
                {
                    auto parentClass = parentClasses[parentClassIdx];
                    parentClass = getClassCPP( parentClass );

                    headerStr += headerTabStr + "public " + parentClass;

                    if( parentClassIdx < parentClasses.size() - 1 )
                    {
                        headerStr += ", ";
                    }
                }
            }
            else
            {
                for( auto parentClass : parentClasses )
                {
                    parentClass = getClassCPP( parentClass );
                    headerStr += "public " + parentClass;
                }
            }

            headerStr += headerTabStr + newLineStr;
        }
        else
        {
            headerStr += headerTabStr + "class " + className + newLineStr;
        }

        headerStr += headerTabStr + "{" + newLineStr;
        headerStr += headerTabStr + "public:" + newLineStr;
        headerTabStr = headerTabStr + singleTabStr;

        for( auto function : functions )
        {
            auto returnType = function->getReturnType();
            auto functionName = function->getFunctionName();

            if( !function->isConstructor() && !function->isDestructor() )
            {
                if( !StringUtil::isNullOrEmpty( functionName ) )
                {
                    functionName[0] = std::tolower( functionName[0] );
                }
            }

            auto args = function->getArguments();

            if( !function->isConstructor() && !function->isDestructor() )
            {
                headerStr += headerTabStr + ParameterUtil::getParamTypeAsString( returnType ) + " " +
                             functionName + "(";
            }
            else
            {
                headerStr += headerTabStr + functionName + "(";
            }

            for( size_t argIdx = 0; argIdx < args.size(); ++argIdx )
            {
                auto arg = args[argIdx];
                auto argType = arg->getType();

                auto argName = arg->getName();

                headerStr += ParameterUtil::getParamTypeAsString( argType ) + " " + argName;

                if( argIdx < args.size() - 1 )
                {
                    headerStr += ", ";
                }
            }

            headerStr += "); " + newLineStr;
        }

        headerTabStr = headerTabStr.substr( 0, headerTabStr.size() - 1 );
        headerStr += headerTabStr + "};" + newLineStr;
        headerStr += newLineStr;
        // headerTabStr = singleTabStr;

        if( !namespaceNames.empty() )
        {
            auto endNamespaceNames = namespaceNames;
            std::reverse( endNamespaceNames.begin(), endNamespaceNames.end() );
            for( auto namespaceName : endNamespaceNames )
            {
                headerTabStr = headerTabStr.substr( 0, headerTabStr.size() - 1 );
                headerStr += headerTabStr + "}" + " // end namespace " + namespaceName + newLineStr;
            }
        }

        headerStr += newLineStr;
        headerStr += static_cast<String>( "#endif // " + className + "_h__\n" );

        auto headerFilePath = path + "/" + className + ".h";

        auto headerPath = Path::getFilePath( headerFilePath );
        fileSystem->createDirectories( headerPath );

        fileSystem->writeAllText( headerFilePath, headerStr );

        auto sourceStr = String( "" );
        sourceStr += "#include <" + m_precompiledHeader + ">" + newLineStr;
        sourceStr += "#include \"" + className + ".h\"" + newLineStr;

        for( auto header : m_sourceIncludes )
        {
            sourceStr += "#include <" + header + ">" + newLineStr;
        }

        sourceStr += newLineStr;

        if( !namespaceNames.empty() )
        {
            for( auto namespaceName : namespaceNames )
            {
                sourceStr += headerTabStr + "namespace " + namespaceName + newLineStr;
                sourceStr += headerTabStr + "{" + newLineStr;
                headerTabStr = headerTabStr + singleTabStr;
            }
        }

        sourceStr += newLineStr;

        for( auto function : functions )
        {
            auto returnType = function->getReturnType();
            auto functionName = function->getFunctionName();

            if( !function->isConstructor() && !function->isDestructor() )
            {
                if( !StringUtil::isNullOrEmpty( functionName ) )
                {
                    functionName[0] = std::tolower( functionName[0] );
                }
            }

            auto args = function->getArguments();

            if( !function->isConstructor() && !function->isDestructor() )
            {
                sourceStr += headerTabStr + ParameterUtil::getParamTypeAsString( returnType ) + " " +
                             className + "::" + functionName + "(";
            }
            else
            {
                sourceStr += headerTabStr + className + "::" + functionName + "(";
            }

            for( size_t argIdx = 0; argIdx < args.size(); ++argIdx )
            {
                auto arg = args[argIdx];
                auto argType = arg->getType();

                auto argName = arg->getName();

                sourceStr += ParameterUtil::getParamTypeAsString( argType ) + " " + argName;

                if( argIdx < args.size() - 1 )
                {
                    sourceStr += ", ";
                }
            }

            sourceStr += ") " + newLineStr;
            sourceStr += headerTabStr + "{" + newLineStr;

            auto fn = workphone::static_pointer_cast<ScriptFunction>( function );
            auto functionBody = fn->getFunctionBodyCPP();

            sourceStr += headerTabStr + singleTabStr + functionBody;

            sourceStr += headerTabStr + newLineStr;
            sourceStr += headerTabStr + "}" + newLineStr;
            sourceStr += headerTabStr + newLineStr;
        }

        sourceStr += newLineStr;

        if( !namespaceNames.empty() )
        {
            auto endNamespaceNames = namespaceNames;
            std::reverse( endNamespaceNames.begin(), endNamespaceNames.end() );
            for( auto namespaceName : endNamespaceNames )
            {
                headerTabStr = headerTabStr.substr( 0, headerTabStr.size() - 1 );
                sourceStr += headerTabStr + "}" + " // end namespace " + namespaceName + newLineStr;
            }
        }

        auto sourceFilePath = path + "/" + className + ".cpp";

        auto sourcePath = Path::getFilePath( sourceFilePath );
        fileSystem->createDirectories( sourcePath );

        fileSystem->writeAllText( sourceFilePath, sourceStr );
    }

    auto ScriptGenerator::getHeaderIncludes() const -> Array<String>
    {
        return m_headerIncludes;
    }

    void ScriptGenerator::setHeaderIncludes( const Array<String> &headerIncludes )
    {
        m_headerIncludes = headerIncludes;
    }

    auto ScriptGenerator::getSourceIncludes() const -> Array<String>
    {
        return m_sourceIncludes;
    }

    void ScriptGenerator::setSourceIncludes( const Array<String> &sourceIncludes )
    {
        m_sourceIncludes = sourceIncludes;
    }

    auto ScriptGenerator::getProjectPath() const -> String
    {
        return m_projectPath;
    }

    void ScriptGenerator::setProjectPath( const String &projectPath )
    {
        m_projectPath = projectPath;
    }

    auto ScriptGenerator::getNamespaceNames() const -> Array<String>
    {
        return m_namespaceNames;
    }

    void ScriptGenerator::setNamespaceNames( const Array<String> &namespaceNames )
    {
        m_namespaceNames = namespaceNames;
    }

    void ScriptGenerator::addMapEntry( const String &className, const String &newClassName )
    {
        m_classMap.emplace_back( className, newClassName );
    }
}  // namespace workphone
