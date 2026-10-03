#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/FontManager.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, FontManager, IFontManager );

    FontManager::FontManager() = default;

    FontManager::~FontManager() = default;

    void FontManager::load( SmartPtr<ISharedObject> data )
    {
        m_fonts.reserve( 1024 );
    }

    void FontManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            for( auto &font : m_fonts )
            {
                font->unload( nullptr );
                font = nullptr;
            }

            m_fonts.clear();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IResource> FontManager::create( const String &name )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto font = factoryManager->make_object<IFont>();
        WP_ASSERT( font );

        auto handle = font->getHandle();
        WP_ASSERT( handle );

        font->setName( name );

        auto uuid = StringUtil::getUUID();
        handle->setUUID( uuid );

        m_fonts.push_back( font );

        return font;
    }

    SmartPtr<IResource> FontManager::create( const String &uuid, const String &name )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto font = factoryManager->make_object<IFont>();
        WP_ASSERT( font );

        auto handle = font->getHandle();
        WP_ASSERT( handle );

        font->setName( name );

        handle->setUUID( uuid );

        m_fonts.push_back( font );

        return font;
    }

    Pair<SmartPtr<IResource>, bool> FontManager::createOrRetrieve( const String &uuid,
                                                                   const String &path,
                                                                   const String &type )
    {
        auto fontResource = getById( uuid );
        if( fontResource )
        {
            auto font = workphone::static_pointer_cast<IFont>( fontResource );
            return workphone::make_pair( font, false );
        }

        auto font = create( uuid, path );
        WP_ASSERT( font );

        return workphone::make_pair( font, true );
    }

    Pair<SmartPtr<IResource>, bool> FontManager::createOrRetrieve( const String &path )
    {
        auto fontResource = getByName( path );
        if( fontResource )
        {
            auto font = workphone::static_pointer_cast<IFont>( fontResource );
            return workphone::make_pair( font, false );
        }

        auto font = create( path );
        WP_ASSERT( font );

        return Pair<SmartPtr<IResource>, bool>( font, true );
    }

    void FontManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
            WP_ASSERT( resource );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto font = workphone::static_pointer_cast<IFont>( resource );
            auto data = font->toData();
            if( data->isDerived<Properties>() )
            {
                auto properties = workphone::static_pointer_cast<Properties>( data );

                auto fontStr = DataUtil::toString( properties.get(), true, DataFormat::JSON );
                fileSystem->writeAllText( filePath, fontStr );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IResource> FontManager::loadFromFile( const String &filePath )
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
            if( ApplicationUtil::isSupportedFont( ext ) )
            {
                auto stream = fileSystem->open( filePath );
                if( stream )
                {
                    auto fontStr = stream->getAsString();

                    auto font = create( fontName );

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( filePath, fileInfo ) )
                    {
                        auto fileId = fileInfo.fileId;
                        font->setFileSystemId( fileId );
                    }

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

    SmartPtr<IResource> FontManager::loadResource( const String &name )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            auto fontResource = getByName( name );
            if( fontResource )
            {
                auto font = workphone::static_pointer_cast<IFont>( fontResource );
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

    SmartPtr<IResource> FontManager::getByName( const String &name )
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

    SmartPtr<IResource> FontManager::getById( const String &uuid )
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

    void FontManager::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    void FontManager::destroyAll()
    {
        try
        {
            for( auto &font : m_fonts )
            {
                if( font )
                {
                    font->unload( nullptr );
                }
            }
            m_fonts.clear();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FontManager::destroyResource( SmartPtr<IResource> resource )
    {
        try
        {
            auto it = std::remove( m_fonts.begin(), m_fonts.end(),
                                   workphone::static_pointer_cast<IFont>( resource ) );
            if( it != m_fonts.end() )
            {
                m_fonts.erase( it, m_fonts.end() );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IResource> FontManager::cloneResource( SmartPtr<IResource> resource,
                                                    const String &clonedResourceName )
    {
        try
        {
            auto font = workphone::static_pointer_cast<IFont>( resource );
            if( font )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto clonedFont = factoryManager->make_object<IFont>();
                WP_ASSERT( clonedFont );
                clonedFont->setName( clonedResourceName );

                auto data = font->toData();
                clonedFont->fromData( data );

                m_fonts.push_back( clonedFont );
                return clonedFont;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        return nullptr;
    }

    SmartPtr<IResource> FontManager::cloneResource( const String &name,
                                                    const String &clonedResourceName )
    {
        try
        {
            auto font = getByName( name );
            if( font )
            {
                return cloneResource( font, clonedResourceName );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void FontManager::setStateContext( SmartPtr<IStateContext> stateContext )
    {
    }

    SmartPtr<IStateContext> FontManager::getStateContext() const
    {
        return nullptr;
    }

    IStateContext *FontManager::getStateContextPtr() const
    {
        return nullptr;
    }

    bool FontManager::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

    bool FontManager::handleStateChanged( SmartPtr<IState> &state )
    {
        throw std::logic_error( "The method or operation is not implemented." );
    }

}  // namespace workphone::render
