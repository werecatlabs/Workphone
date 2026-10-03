#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawFontManager.hpp>
#include <WPGraphics/ClawFont.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawFontManager, FontManager );

    ClawFontManager::ClawFontManager() = default;

    ClawFontManager::~ClawFontManager() = default;

    void ClawFontManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            FontManager::load( data );

            ApplicationUtil::createDefaultFont();

            static const auto fontName = String( "default" );
            auto defaultFont = getByName( fontName );
            if( defaultFont )
            {
                if( !defaultFont->isLoaded() )
                {
                    defaultFont->load( nullptr );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ClawFontManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            for( auto &font : m_fonts )
            {
                font->unload( nullptr );
            }

            m_fonts.clear();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IResource> ClawFontManager::create( const String &name )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto font = factoryManager->make_ptr<ClawFont>( this );
        WP_ASSERT( font );

        auto handle = font->getHandle();
        WP_ASSERT( handle );

        font->setName( name );

        auto uuid = StringUtil::getUUID();
        handle->setUUID( uuid );

        m_fonts.emplace_back( font );

        return font;
    }

    SmartPtr<IResource> ClawFontManager::create( const String &uuid, const String &name )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto font = factoryManager->make_ptr<ClawFont>( this );
        WP_ASSERT( font );

        auto handle = font->getHandle();
        WP_ASSERT( handle );

        font->setName( name );

        handle->setUUID( uuid );

        m_fonts.emplace_back( font );

        return font;
    }

    Pair<SmartPtr<IResource>, bool> ClawFontManager::createOrRetrieve( const String &uuid,
                                                                       const String &path,
                                                                       const String &type )
    {
        auto fontResource = getById( uuid );
        if( fontResource )
        {
            auto font = workphone::static_pointer_cast<ClawFont>( fontResource );
            return Pair<SmartPtr<IResource>, bool>( font, false );
        }

        if( !StringUtil::isNullOrEmpty( uuid ) )
        {
            auto font = create( uuid, path );
            WP_ASSERT( font );

            return Pair<SmartPtr<IResource>, bool>( font, true );
        }

        auto font = create( path );
        WP_ASSERT( font );

        return Pair<SmartPtr<IResource>, bool>( font, true );
    }

    Pair<SmartPtr<IResource>, bool> ClawFontManager::createOrRetrieve( const String &path )
    {
        auto fontResource = getByName( path );
        if( fontResource )
        {
            auto font = workphone::static_pointer_cast<ClawFont>( fontResource );
            return Pair<SmartPtr<IResource>, bool>( font, false );
        }

        auto font = create( path );
        WP_ASSERT( font );

        return Pair<SmartPtr<IResource>, bool>( font, true );
    }

    void ClawFontManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
            WP_ASSERT( resource );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto font = workphone::static_pointer_cast<IFont>( resource );
            auto pData = font->toData();

            auto fontStr = DataUtil::toString( pData.get(), true );
            fileSystem->writeAllText( filePath, fontStr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IResource> ClawFontManager::loadFromFile( const String &filePath )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto fontName = Path::getFileNameWithoutExtension( filePath );

            for( auto font : m_fonts )
            {
                auto currentMaterialName = font->getName();

                if( fontName == currentMaterialName )
                {
                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( filePath, fileInfo ) )
                    {
                        auto currentMaterialPath = String( fileInfo.filePath.c_str() );
                        auto currentFileId = font->getFileSystemId();

                        if( filePath == currentMaterialPath && fileInfo.fileId == currentFileId )
                        {
                            return font;
                        }
                    }
                }
            }

            auto ext = Path::getFileExtension( filePath );
            if( ext == ".font" )
            {
                if( auto stream = fileSystem->open( filePath ) )
                {
                    auto fontStr = stream->getAsString();

                    auto properties = workphone::make_ptr<Properties>();
                    DataUtil::parse( fontStr, properties.get() );

                    auto font = create( fontName );

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( filePath, fileInfo ) )
                    {
                        auto fileId = fileInfo.fileId;
                        font->setFileSystemId( fileId );
                    }

                    font->setProperties( properties );
                    font->load( properties );
                    return font;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IResource> ClawFontManager::loadResource( const String &name )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );
            if( auto fontResource = getByName( name ) )
            {
                auto font = workphone::static_pointer_cast<ClawFont>( fontResource );
                font->load( nullptr );
                return font;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IResource> ClawFontManager::getByName( const String &name )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            for( auto &font : m_fonts )
            {
                if( font->getName() == name )
                {
                    return font;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IResource> ClawFontManager::getById( const String &uuid )
    {
        try
        {
            for( auto &font : m_fonts )
            {
                auto handle = font->getHandle();
                if( handle->getUUIDAsString() == uuid )
                {
                    return font;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IResource> ClawFontManager::cloneResource( const String &name,
                                                        const String &clonedResourceName )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        if( auto clonedFont = factoryManager->make_ptr<ClawFont>() )
        {
            return clonedFont;
        }

        return nullptr;
    }

    void ClawFontManager::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }
}  // namespace workphone::render
