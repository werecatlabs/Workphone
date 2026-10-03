#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/JobCreatePackage.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/INativeFileDialog.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/IO/ZipUtil.hpp>
#include <algorithm>
#include <fstream>
#include <initializer_list>

namespace workphone
{
    namespace
    {
        struct PackageRoots
        {
            StringW projectPath;
            StringW assetsPath;
            StringW cachePath;
            StringW settingsCachePath;
        };

        String normalisePackageKey( String value )
        {
            value = StringUtil::replaceAll( value, "\\", "/" );
            value = StringUtil::cleanupPath( value );
            value = StringUtil::trim( value );
            value = StringUtil::make_lower( value );

            while( !value.empty() && ( value[0] == '/' || value[0] == '.' ) )
            {
                if( value[0] == '.' && value.size() > 1 && value[1] != '/' )
                {
                    break;
                }

                value.erase( value.begin() );
            }

            return value;
        }

        String normalisePackageKey( const StringW &value )
        {
            return normalisePackageKey( StringUtil::toUTF16to8( value ) );
        }

        String removePackageExtension( const String &value )
        {
            auto slash = value.find_last_of( '/' );
            auto dot = value.find_last_of( '.' );
            if( dot == String::npos )
            {
                return value;
            }

            if( slash != String::npos && dot < slash )
            {
                return value;
            }

            return value.substr( 0, dot );
        }

        String stripKnownPackageRoot( const String &value )
        {
            static const Array<String> prefixes = { "assets/", "cache/", "settingscache/" };
            for( const auto &prefix : prefixes )
            {
                if( value.rfind( prefix, 0 ) == 0 )
                {
                    return value.substr( prefix.length() );
                }
            }

            return value;
        }

        bool containsPackageKey( const Array<String> &keys, const String &key )
        {
            return std::find( keys.begin(), keys.end(), key ) != keys.end();
        }

        void addPackageKey( Array<String> &keys, const String &value )
        {
            auto key = normalisePackageKey( value );
            if( StringUtil::isNullOrEmpty( key ) )
            {
                return;
            }

            if( !containsPackageKey( keys, key ) )
            {
                keys.push_back( key );
            }

            auto noExtension = removePackageExtension( key );
            if( !StringUtil::isNullOrEmpty( noExtension ) && !containsPackageKey( keys, noExtension ) )
            {
                keys.push_back( noExtension );
            }

            auto stripped = stripKnownPackageRoot( key );
            if( stripped != key )
            {
                addPackageKey( keys, stripped );
            }
        }

        void addRequiredResourcePath( Array<String> &requiredKeys, const String &path )
        {
            auto rawKey = StringUtil::replaceAll( path, "\\", "/" );
            rawKey = StringUtil::cleanupPath( StringUtil::trim( rawKey ) );
            auto key = normalisePackageKey( path );
            if( StringUtil::isNullOrEmpty( key ) )
            {
                return;
            }

            addPackageKey( requiredKeys, key );

            if( !StringUtil::isNullOrEmpty( rawKey ) )
            {
                auto rawUuid = StringUtil::toString( StringUtil::getUUID( rawKey ) );
                addPackageKey( requiredKeys, rawUuid );
                addPackageKey( requiredKeys, rawUuid + ".resourcedata" );
            }

            auto uuid = StringUtil::toString( StringUtil::getUUID( key ) );
            addPackageKey( requiredKeys, uuid );
            addPackageKey( requiredKeys, uuid + ".resourcedata" );

            auto stripped = stripKnownPackageRoot( key );
            if( stripped != key )
            {
                auto strippedUuid = StringUtil::toString( StringUtil::getUUID( stripped ) );
                addPackageKey( requiredKeys, strippedUuid );
                addPackageKey( requiredKeys, strippedUuid + ".resourcedata" );
            }

            auto rawStripped = stripKnownPackageRoot( normalisePackageKey( rawKey ) );
            if( rawStripped != key && !StringUtil::isNullOrEmpty( rawStripped ) )
            {
                auto rawStrippedUuid = StringUtil::toString( StringUtil::getUUID( rawStripped ) );
                addPackageKey( requiredKeys, rawStrippedUuid );
                addPackageKey( requiredKeys, rawStrippedUuid + ".resourcedata" );
            }
        }

        String getFirstFieldValue( SmartPtr<IDatabaseQuery> query,
                                   std::initializer_list<const char *> fieldNames )
        {
            if( !query )
            {
                return String();
            }

            for( auto fieldName : fieldNames )
            {
                auto value = query->getFieldValue( String( fieldName ) );
                if( !StringUtil::isNullOrEmpty( value ) )
                {
                    return StringUtil::trim( value );
                }
            }

            return String();
        }

        Array<String> parsePackageTokens( String value )
        {
            value = StringUtil::replaceAll( value, ",", ";" );
            value = StringUtil::replaceAll( value, "|", ";" );
            value = StringUtil::trim( value );

            Array<String> tokens;
            if( StringUtil::isNullOrEmpty( value ) )
            {
                return tokens;
            }

            if( value[value.length() - 1] != ';' )
            {
                value += ";";
            }

            StringUtil::parseArray( value, tokens );
            for( auto &token : tokens )
            {
                token = normalisePackageKey( token );
            }

            return tokens;
        }

        bool tableExists( SmartPtr<IDatabaseManager> databaseManager, const String &tableName )
        {
            if( !databaseManager || StringUtil::isNullOrEmpty( tableName ) )
            {
                return false;
            }

            auto sql = String( "select name from sqlite_master where type='table' and name='" ) +
                       tableName + String( "'" );
            auto query = databaseManager->executeQuery( sql );
            return query && !query->eof();
        }

        bool isRequiredReferenceRow( SmartPtr<IDatabaseQuery> query )
        {
            auto dependencyMode = normalisePackageKey(
                getFirstFieldValue( query, { "dependencyMode", "dependency_mode", "mode" } ) );

            if( dependencyMode == "excluded" || dependencyMode == "exclude" ||
                dependencyMode == "ignore" || dependencyMode == "optional" || dependencyMode == "soft" ||
                dependencyMode == "weak" )
            {
                return false;
            }

            auto required = getFirstFieldValue( query, { "required", "is_required" } );
            if( !StringUtil::isNullOrEmpty( required ) )
            {
                return StringUtil::parseBool( required, true );
            }

            return true;
        }

        Array<String> getTargetPlatformNames( const Array<bool> &targetPlatforms )
        {
            static const Array<String> names = { "windows", "linux",        "macos",     "android",
                                                 "ios",     "windowsphone", "windowsrt", "xboxone",
                                                 "ps4",     "switch",       "html5",     "custom" };

            Array<String> enabledPlatforms;
            auto count = std::min( targetPlatforms.size(), names.size() );
            for( size_t i = 0; i < count; ++i )
            {
                if( targetPlatforms[i] )
                {
                    enabledPlatforms.push_back( names[i] );
                }
            }

            return enabledPlatforms;
        }

        bool supportsTargetPlatform( SmartPtr<IDatabaseQuery> query, const Array<String> &platforms )
        {
            auto platformValue = getFirstFieldValue( query, { "platforms", "platform" } );
            if( StringUtil::isNullOrEmpty( platformValue ) || platforms.empty() )
            {
                return true;
            }

            auto referencePlatforms = parsePackageTokens( platformValue );
            for( const auto &referencePlatform : referencePlatforms )
            {
                if( referencePlatform == "*" || referencePlatform == "all" )
                {
                    return true;
                }

                if( containsPackageKey( platforms, referencePlatform ) )
                {
                    return true;
                }
            }

            return false;
        }

        String getResourcePathForUUID( SmartPtr<IDatabaseManager> databaseManager, const String &uuid )
        {
            if( !databaseManager || StringUtil::isNullOrEmpty( uuid ) )
            {
                return String();
            }

            auto sql = String( "select * from 'resources' where uuid='" ) + uuid + String( "'" );
            auto query = databaseManager->executeQuery( sql );
            if( query && !query->eof() )
            {
                return query->getFieldValue( "path" );
            }

            return String();
        }

        String buildFallbackReferencePath( const String &path, const String &name )
        {
            auto value = StringUtil::trim( path );
            if( !StringUtil::isNullOrEmpty( value ) )
            {
                return value;
            }

            value = StringUtil::trim( name );
            if( StringUtil::isNullOrEmpty( value ) )
            {
                return String();
            }

            return String( "reference/" ) + value;
        }

        void collectRequiredRows( SmartPtr<IDatabaseQuery> query, Array<String> &requiredKeys,
                                  SmartPtr<IDatabaseManager> databaseManager,
                                  const Array<String> &platforms, bool configuredActorFallback )
        {
            if( !query )
            {
                return;
            }

            while( !query->eof() )
            {
                if( isRequiredReferenceRow( query ) && supportsTargetPlatform( query, platforms ) )
                {
                    auto path = getFirstFieldValue( query, { "resourcePath", "resource_path", "path" } );

                    if( configuredActorFallback )
                    {
                        auto name = getFirstFieldValue( query, { "name", "resource_name" } );
                        path = buildFallbackReferencePath( path, name );
                    }
                    else if( StringUtil::isNullOrEmpty( path ) )
                    {
                        auto uuid = getFirstFieldValue(
                            query, { "resourceUUID", "resource_uuid", "resource", "uuid" } );
                        path = getResourcePathForUUID( databaseManager, uuid );
                    }

                    addRequiredResourcePath( requiredKeys, path );
                }

                query->nextRow();
            }
        }

        Array<String> collectRequiredResourceKeys(
            SmartPtr<core::IApplicationManager> applicationManager, const Array<bool> &targetPlatforms )
        {
            Array<String> requiredKeys;
            if( !applicationManager )
            {
                return requiredKeys;
            }

            auto databaseManager = workphone::make_ptr<AssetDatabaseManager>();
            auto databasePath = applicationManager->getMediaPath() + "/AssetDatabase.db";
            databaseManager->loadFromFile( databasePath );

            auto platforms = getTargetPlatformNames( targetPlatforms );
            auto foundReferenceTable = false;

            static const Array<String> referenceTables = { "resource_references", "resourceReferences",
                                                           "references" };
            for( const auto &tableName : referenceTables )
            {
                if( !tableExists( databaseManager, tableName ) )
                {
                    continue;
                }

                foundReferenceTable = true;
                auto sql = String( "select * from '" ) + tableName + String( "'" );
                collectRequiredRows( databaseManager->executeQuery( sql ), requiredKeys, databaseManager,
                                     platforms, false );
            }

            if( !foundReferenceTable )
            {
                auto sql = String( "select * from 'configured_actors' where parent_id=1" );
                collectRequiredRows( databaseManager->executeQuery( sql ), requiredKeys, databaseManager,
                                     platforms, true );
            }

            return requiredKeys;
        }

        bool keyEndsWithPath( const String &value, const String &suffix )
        {
            if( suffix.size() > value.size() )
            {
                return false;
            }

            auto offset = value.size() - suffix.size();
            if( value.compare( offset, suffix.size(), suffix ) != 0 )
            {
                return false;
            }

            return offset == 0 || value[offset - 1] == '/';
        }

        bool packageKeysMatch( const String &candidate, const String &required )
        {
            if( candidate == required )
            {
                return true;
            }

            auto candidateNoExtension = removePackageExtension( candidate );
            auto requiredNoExtension = removePackageExtension( required );
            if( candidateNoExtension == requiredNoExtension )
            {
                return true;
            }

            return keyEndsWithPath( candidate, required ) ||
                   keyEndsWithPath( candidateNoExtension, requiredNoExtension );
        }

        void addCandidateFileKeys( Array<String> &keys, const StringW &filePath,
                                   const PackageRoots &roots )
        {
            addPackageKey( keys, normalisePackageKey( filePath ) );
            addPackageKey( keys, normalisePackageKey( PathW::getFileName( filePath ) ) );

            if( !StringUtilW::isNullOrEmpty( roots.projectPath ) )
            {
                addPackageKey(
                    keys, normalisePackageKey( PathW::getRelativePath( roots.projectPath, filePath ) ) );
            }

            if( !StringUtilW::isNullOrEmpty( roots.assetsPath ) )
            {
                addPackageKey(
                    keys, normalisePackageKey( PathW::getRelativePath( roots.assetsPath, filePath ) ) );
            }

            if( !StringUtilW::isNullOrEmpty( roots.cachePath ) )
            {
                addPackageKey(
                    keys, normalisePackageKey( PathW::getRelativePath( roots.cachePath, filePath ) ) );
            }

            if( !StringUtilW::isNullOrEmpty( roots.settingsCachePath ) )
            {
                addPackageKey( keys, normalisePackageKey(
                                         PathW::getRelativePath( roots.settingsCachePath, filePath ) ) );
            }
        }

        bool isRequiredPackageFile( const StringW &filePath, const Array<String> &requiredKeys,
                                    const PackageRoots &roots )
        {
            if( requiredKeys.empty() )
            {
                return false;
            }

            Array<String> candidateKeys;
            addCandidateFileKeys( candidateKeys, filePath, roots );

            for( const auto &candidateKey : candidateKeys )
            {
                for( const auto &requiredKey : requiredKeys )
                {
                    if( packageKeysMatch( candidateKey, requiredKey ) )
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        Array<StringW> filterRequiredPackageFiles( const Array<StringW> &files,
                                                   const Array<String> &requiredKeys,
                                                   const PackageRoots &roots )
        {
            Array<StringW> filtered;
            filtered.reserve( files.size() );

            for( const auto &file : files )
            {
                if( isRequiredPackageFile( file, requiredKeys, roots ) )
                {
                    filtered.push_back( file );
                }
            }

            return filtered;
        }

        Array<StringW> appendFilesByExtension( const StringW &root, const Array<StringW> &extensions )
        {
            Array<StringW> files;
            for( const auto &extension : extensions )
            {
                auto result = PathW::getFilesAsAbsolutePaths( root, extension, true );
                files.insert( files.end(), result.begin(), result.end() );
            }

            return files;
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, JobCreatePackage, Job );

    JobCreatePackage::JobCreatePackage() = default;

    JobCreatePackage::~JobCreatePackage() = default;

    void JobCreatePackage::execute()
    {
        using namespace workphone;
        WP_DEBUG_TRACE;

#if defined WP_PLATFORM_WIN32
        WP_LOG( "Starting packaging" );

        auto applicationManager = core::IApplicationManager::instance();

        if( !applicationManager )
        {
            WP_LOG_ERROR( "Application manager is not available." );
            return;
        }
        auto jobQueue = applicationManager->getJobQueue();

        if( !jobQueue )
        {
            WP_LOG_ERROR( "Job queue is not available." );
            return;
        }

        auto dst = getDst();
        auto projectPath = applicationManager->getProjectPath();
        auto rootPath = PathW::getWorkingDirectory();
        auto projectPathW = StringUtil::toUTF8to16( projectPath );

        auto assetsPath = projectPathW + L"/Assets/";
        auto cachePath = projectPathW + L"/Cache/";
        auto settingsCachePath = projectPathW + L"/SettingsCache/";
        auto packageRoots = PackageRoots{ projectPathW, assetsPath, cachePath, settingsCachePath };

        auto requiredKeys = collectRequiredResourceKeys( applicationManager, getBuildTargetPlatform() );
        if( requiredKeys.empty() )
        {
            WP_LOG_WARNING( "No required resources were found. Package archives will be empty." );
        }
        else
        {
            WP_LOG( "Packaging required resources: " +
                    StringUtil::toString( static_cast<u32>( requiredKeys.size() ) ) );
        }

        auto sceneFiles = PathW::getFilesAsAbsolutePaths( assetsPath, StringW( L".fbscene" ), true );
        auto binarySceneFiles =
            PathW::getFilesAsAbsolutePaths( assetsPath, StringW( L".fbscenebin" ), true );
        auto xmlSceneFiles =
            PathW::getFilesAsAbsolutePaths( assetsPath, StringW( L".fbscenexml" ), true );
        sceneFiles.insert( sceneFiles.end(), binarySceneFiles.begin(), binarySceneFiles.end() );
        sceneFiles.insert( sceneFiles.end(), xmlSceneFiles.begin(), xmlSceneFiles.end() );
        sceneFiles = filterRequiredPackageFiles( sceneFiles, requiredKeys, packageRoots );

        auto scenesJob = jobQueue->startJob( [dst, sceneFiles] {
            ZipUtil::createObfuscatedZipFileFromPath( dst + L"/scenes.fbpak", sceneFiles );
        } );

        auto resourceFiles = PathW::getFilesAsAbsolutePaths( assetsPath, L".resource", true );
        auto lightingPresets = PathW::getFilesAsAbsolutePaths( assetsPath, L".lightingpreset", true );
        auto materialFiles = PathW::getFilesAsAbsolutePaths( assetsPath, L".mat", true );
        auto settingsFiles =
            PathW::getFilesAsAbsolutePaths( settingsCachePath, StringW( L".resourcedata" ), true );

        resourceFiles.insert( resourceFiles.end(), lightingPresets.begin(), lightingPresets.end() );
        resourceFiles.insert( resourceFiles.end(), materialFiles.begin(), materialFiles.end() );
        resourceFiles.insert( resourceFiles.end(), settingsFiles.begin(), settingsFiles.end() );
        resourceFiles = filterRequiredPackageFiles( resourceFiles, requiredKeys, packageRoots );

        auto resourcesJob = jobQueue->startJob( [dst, resourceFiles, projectPathW] {
            ZipUtil::createObfuscatedZipFileFromPath( dst + L"/resources.fbpak", resourceFiles,
                                                      projectPathW, true );
        } );

        auto meshFiles = PathW::getFilesAsAbsolutePaths( cachePath, StringW( L".fbmeshbin" ), true );
        auto physicsMeshFiles =
            PathW::getFilesAsAbsolutePaths( cachePath, StringW( L".pxtrianglemesh" ), true );
        meshFiles.insert( meshFiles.end(), physicsMeshFiles.begin(), physicsMeshFiles.end() );
        meshFiles = filterRequiredPackageFiles( meshFiles, requiredKeys, packageRoots );

        auto meshJob = jobQueue->startJob( [dst, projectPathW, meshFiles] {
            ZipUtil::createObfuscatedZipFileFromPath( dst + L"/mesh.fbpak", meshFiles, projectPathW,
                                                      true );
        } );

        auto meshSourceFiles = appendFilesByExtension( assetsPath, { L".fbx", L".FBX" } );
        meshSourceFiles = filterRequiredPackageFiles( meshSourceFiles, requiredKeys, packageRoots );

        auto meshSourceJob = jobQueue->startJob( [dst, meshSourceFiles] {
            ZipUtil::createObfuscatedZipFileFromPath( dst + L"/meshSource.fbpak", meshSourceFiles );
        } );

        //jobQueue->startJob( [dst, projectPathW] {
        //    auto settingsFiles = PathW::getFilesAsAbsolutePaths( projectPathW + L"/SettingsCache/",
        //                                                         StringW( L".meshdata" ), true );
        //    ZipUtil::createObfuscatedZipFileFromPath( dst + L"/settings.fbpak", settingsFiles );
        //} );

        //jobQueue->startJob( [dst, assetsPath, projectPathW] {
        //    auto materialFiles = PathW::getFilesAsAbsolutePaths( assetsPath, StringW( L".mat" ), true );
        //    ZipUtil::createObfuscatedZipFileFromPath( dst + L"/material.fbpak", materialFiles,
        //                                              projectPathW, true );
        //} );

        auto scriptFiles = appendFilesByExtension( assetsPath, { L".py", L".lua" } );
        scriptFiles = filterRequiredPackageFiles( scriptFiles, requiredKeys, packageRoots );

        auto scriptsJob = jobQueue->startJob( [dst, scriptFiles] {
            ZipUtil::createObfuscatedZipFileFromPath( dst + L"/scripts.fbpak", scriptFiles );
        } );

        bool packageTextures = getPackageTextures();
        if( packageTextures )
        {
            auto supportedTextureFormats =
                Array<StringW>{ L".dds", L".png", L".jpg", L".jpeg", L".tga", L".tiff" };

            auto textureFiles = appendFilesByExtension( assetsPath, supportedTextureFormats );
            textureFiles = filterRequiredPackageFiles( textureFiles, requiredKeys, packageRoots );

            auto pakCount = 0;
            auto numTextures = 128;
            for( size_t i = 0; i < textureFiles.size(); i += numTextures )
            {
                auto end = std::min( textureFiles.size(), i + static_cast<size_t>( numTextures ) );
                auto texs = Array<StringW>( textureFiles.begin() + i, textureFiles.begin() + end );

                auto pakCountStr = StringUtilW::toString( pakCount );

                jobQueue->startJob( [dst, pakCountStr, texs, projectPathW] {
                    ZipUtil::createObfuscatedZipFileFromPath(
                        dst + L"/textures_" + pakCountStr + L".fbpak", texs, projectPathW, true );
                } );

                pakCount++;
            }
        }

        scenesJob->wait();
        resourcesJob->wait();
        meshJob->wait();
        meshSourceJob->wait();
        scriptsJob->wait();

        while( jobQueue->hasJobs() )
        {
            Thread::sleep( 1.0 );
        }

        WP_LOG( "Finished packaging" );
#endif
    }

    auto JobCreatePackage::getDst() const -> StringW
    {
        return m_dst;
    }

    void JobCreatePackage::setDst( const StringW &dst )
    {
        m_dst = dst;
    }

    auto JobCreatePackage::getPackageTextures() const -> bool
    {
        return m_packageTextures;
    }

    void JobCreatePackage::setPackageTextures( bool packageTextures )
    {
        m_packageTextures = packageTextures;
    }

    Array<bool> JobCreatePackage::getBuildTargetPlatform() const
    {
        return m_buildTargetPlatform.load();
    }

    void JobCreatePackage::setBuildTargetPlatform( const Array<bool> &buildTargetPlatform )
    {
        m_buildTargetPlatform = buildTargetPlatform;
    }
}  // namespace workphone
