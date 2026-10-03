#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CFontOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreFontManager.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CFontOgreNext, Font );

    u32 CFontOgreNext::m_nameExt = 0;

    CFontOgreNext::CFontOgreNext( IResourceManager *resourceManager )
    {
        static const auto FontStr = String( "Font" );
        auto name = FontStr + StringUtil::toString( m_nameExt++ );

        setName( name );
        setResourceManager( resourceManager );
    }

    CFontOgreNext::CFontOgreNext() = default;

    CFontOgreNext::~CFontOgreNext()
    {
        const auto &loadingState = getLoadingState();
        if( loadingState != LoadingState::Unloaded )
        {
            unload( nullptr );
        }
    }

    void CFontOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            if( !graphicsSystem->isValid() )
            {
                WP_LOG_ERROR( "Graphics system is not valid." );
                setLoadingState( LoadingState::Error );
                return;
            } 

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            ScopedLock lock( graphicsSystem );

            setLoadingState( LoadingState::Loading );

            auto ogreFontManager = Ogre::FontManager::getSingletonPtr();

            auto name = getName();
            auto grp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            auto resourceName = Ogre::String( name.c_str(), name.length() );
            auto fontResource = ogreFontManager->createOrRetrieve( resourceName, grp );

            auto font = Ogre::static_pointer_cast<Ogre::Font>( fontResource.first );
            if( font )
            {
                font->setType( Ogre::FT_TRUETYPE );
                font->setTrueTypeResolution( m_fontResolution );
                font->setTrueTypeSize( (f32)m_fontSize );

                auto fontSource = Ogre::String( m_fontSource.c_str(), m_fontSource.length() );
                font->setSource( fontSource );

                font->load();

                m_font = font;
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CFontOgreNext::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CFontOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto ogreFontManager = Ogre::FontManager::getSingletonPtr();
                if( ogreFontManager )
                {
                    if( m_font )
                    {
                        auto resourceHandle = m_font->getHandle();
                        if( ogreFontManager->resourceExists( resourceHandle ) )
                        {
                            ogreFontManager->remove( resourceHandle );
                        }

                        m_font.setNull();
                    }
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CFontOgreNext::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = ResourceGraphics<IFont>::getProperties();
        properties->setProperty( "font_type", m_fontType );
        properties->setProperty( "font_source", m_fontSource );
        properties->setProperty( "font_size", m_fontSize );
        properties->setProperty( "font_resolution", m_fontResolution );
        return properties;
    }

    void CFontOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        properties->getPropertyValue( "font_type", m_fontType );
        properties->getPropertyValue( "font_source", m_fontSource );
        properties->getPropertyValue( "font_size", m_fontSize );
        properties->getPropertyValue( "font_resolution", m_fontResolution );
    }

    auto CFontOgreNext::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Font::getChildObjects();

        return objects;
    }

}  // namespace workphone::render
