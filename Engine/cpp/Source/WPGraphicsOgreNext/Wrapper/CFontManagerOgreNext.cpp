#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CFontManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CFontOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreFontManager.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CFontManagerOgreNext, FontManager );

    CFontManagerOgreNext::CFontManagerOgreNext() = default;

    CFontManagerOgreNext::~CFontManagerOgreNext() = default;

    void CFontManagerOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            FontManager::load( data );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto resourceDatabase = applicationManager->getResourceDatabase();

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

    void CFontManagerOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto ogreFontManager = Ogre::FontManager::getSingletonPtr();
            WP_ASSERT( ogreFontManager );

            for( auto &font : m_fonts )
            {
                font->unload( nullptr );
            }

            m_fonts.clear();

            ogreFontManager->removeAll();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CFontManagerOgreNext::create( const String &name ) -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto font = factoryManager->make_ptr<CFontOgreNext>( this );
        WP_ASSERT( font );

        auto handle = font->getHandle();
        WP_ASSERT( handle );

        font->setName( name );

        auto uuid = StringUtil::getUUID();
        handle->setUUID( uuid );

        m_fonts.emplace_back( font );

        return font;
    }

    auto CFontManagerOgreNext::create( const String &uuid, const String &name ) -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto font = factoryManager->make_ptr<CFontOgreNext>( this );
        WP_ASSERT( font );

        auto handle = font->getHandle();
        WP_ASSERT( handle );

        font->setName( name );

        handle->setUUID( uuid );

        m_fonts.emplace_back( font );

        return font;
    }

    auto CFontManagerOgreNext::createOrRetrieve( const String &uuid, const String &path,
                                                 const String &type ) -> Pair<SmartPtr<IResource>, bool>
    {
        auto fontResource = getById( uuid );
        if( fontResource )
        {
            auto font = workphone::static_pointer_cast<CFontOgreNext>( fontResource );
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

    auto CFontManagerOgreNext::createOrRetrieve( const String &path ) -> Pair<SmartPtr<IResource>, bool>
    {
        auto fontResource = getByName( path );
        if( fontResource )
        {
            auto font = workphone::static_pointer_cast<CFontOgreNext>( fontResource );
            return Pair<SmartPtr<IResource>, bool>( font, false );
        }

        auto font = create( path );
        WP_ASSERT( font );

        return Pair<SmartPtr<IResource>, bool>( font, true );
    }

    void CFontManagerOgreNext::saveToFile( const String &filePath, SmartPtr<IResource> resource )
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

    auto CFontManagerOgreNext::loadFromFile( const String &filePath ) -> SmartPtr<IResource>
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

                    font->load( properties );

                    m_fonts.push_back( font );

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

    auto CFontManagerOgreNext::loadResource( const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );
            if( auto fontResource = getByName( name ) )
            {
                auto font = workphone::static_pointer_cast<CFontOgreNext>( fontResource );
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

    auto CFontManagerOgreNext::getByName( const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            for( auto &font : m_fonts )
            {
                if( font->getNamePtr() == name )
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

    auto CFontManagerOgreNext::getById( const String &uuid ) -> SmartPtr<IResource>
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

    SmartPtr<IResource> CFontManagerOgreNext::cloneResource( const String &name,
                                                             const String &clonedResourceName )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        if( auto clonedFont = factoryManager->make_ptr<CFontOgreNext>() )
        {
            return clonedFont;
        }

        return nullptr;
    }

    void CFontManagerOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    bool CFontManagerOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    bool CFontManagerOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

}  // namespace workphone::render
