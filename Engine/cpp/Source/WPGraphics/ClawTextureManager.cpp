#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawTextureManager.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawCubemapTexture.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawTextureManager, TextureManager );

        ClawTextureManager::ClawTextureManager() = default;
        ClawTextureManager::~ClawTextureManager() = default;

        SmartPtr<ITexture> ClawTextureManager::createCubeMap(
            const Array<SmartPtr<ITexture>> &textures )
        {
            auto texture = workphone::make_ptr<ClawCubemapTexture>();
            texture->setName( "GeneratedCubemap/" + StringUtil::getUUID() );
            texture->setFaces( textures );
            texture->load( nullptr );
            m_textures.push_back( texture );
            return texture;
        }

        void ClawTextureManager::load( SmartPtr<ISharedObject> data )
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );
            TextureManager::load( data );
            setLoadingState( LoadingState::Loaded );
        }

        void ClawTextureManager::unload( SmartPtr<ISharedObject> data )
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
            auto textures = m_textures.snapshot();
            for( auto &texture : textures )
            {
                if( texture )
                {
                    texture->unload( nullptr );
                }
            }
            m_renderTargets.clear();
            m_textures.clear();
            TextureManager::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }

        SmartPtr<ITexture> ClawTextureManager::createTexture( const String &uuid, const String &name )
        {
            auto texture = workphone::make_ptr<ClawTexture>();
            if( !texture )
            {
                return nullptr;
            }

            texture->setName( name );
            texture->setFilePath( name );
            if( auto handle = texture->getHandle() )
            {
                handle->setUUID( StringUtil::isNullOrEmpty( uuid ) ? StringUtil::getUUID() : uuid );
            }

            m_textures.push_back( texture );
            return texture;
        }

        SmartPtr<IResource> ClawTextureManager::create( const String &name )
        {
            if( StringUtil::isNullOrEmpty( name ) )
            {
                return nullptr;
            }
            if( auto existing = getByName( name ) )
            {
                return existing;
            }
            return createTexture( String(), name );
        }

        SmartPtr<IResource> ClawTextureManager::create( const String &uuid, const String &name )
        {
            if( StringUtil::isNullOrEmpty( name ) )
            {
                return nullptr;
            }
            if( !StringUtil::isNullOrEmpty( uuid ) )
            {
                if( auto existing = getById( uuid ) )
                {
                    return existing;
                }
            }
            if( auto existing = getByName( name ) )
            {
                return existing;
            }
            return createTexture( uuid, name );
        }

        SmartPtr<IResource> ClawTextureManager::loadResource( const String &name )
        {
            auto resource = getByName( name );
            if( !resource )
            {
                resource = create( name );
            }
            if( resource && !resource->isLoaded() )
            {
                resource->load( nullptr );
            }
            return resource;
        }

        SmartPtr<IResource> ClawTextureManager::loadFromFile( const String &filePath )
        {
            auto resource = loadResource( filePath );
            if( resource )
            {
                resource->setFilePath( filePath );
                resource->loadFromFile( filePath );
            }
            return resource;
        }

        SmartPtr<IResource> ClawTextureManager::cloneResource( SmartPtr<IResource> resource,
                                                               const String &clonedResourceName )
        {
            if( !resource || !resource->isDerived<ITexture>() )
            {
                return nullptr;
            }
            return cloneTexture( workphone::static_pointer_cast<ITexture>( resource ),
                                 clonedResourceName );
        }

        SmartPtr<IResource> ClawTextureManager::cloneResource( const String &name,
                                                               const String &clonedResourceName )
        {
            return cloneResource( getByName( name ), clonedResourceName );
        }

        Pair<SmartPtr<IResource>, bool> ClawTextureManager::createOrRetrieve( const String &uuid,
                                                                              const String &path,
                                                                              const String & /*type*/ )
        {
            if( !StringUtil::isNullOrEmpty( uuid ) )
            {
                if( auto resource = getById( uuid ) )
                {
                    return { resource, false };
                }
            }
            if( auto resource = getByName( path ) )
            {
                return { resource, false };
            }
            return { create( uuid, path ), true };
        }

        Pair<SmartPtr<IResource>, bool> ClawTextureManager::createOrRetrieve( const String &path )
        {
            if( auto resource = getByName( path ) )
            {
                return { resource, false };
            }
            return { create( path ), true };
        }

        SmartPtr<ITexture> ClawTextureManager::createManual( const String &name,
                                                             const String & /*group*/, u8 texType,
                                                             u32 width, u32 height, u32 /*depth*/,
                                                             s32 /*numMips*/, u8 /*format*/, s32 usage )
        {
            auto texture = workphone::dynamic_pointer_cast<ITexture>( create( name ) );
            if( texture )
            {
                texture->setSize( Vector2I( static_cast<s32>( width ), static_cast<s32>( height ) ) );
                texture->setUsageFlags( static_cast<u32>( usage ) );
                if( auto clawTexture = dynamic_pointer_cast<ClawTexture>( texture ) )
                {
                    clawTexture->setTextureType( static_cast<TextureType>( texType ) );
                }
                // Manual textures have no file to decode. Initialise the pixel storage before
                // loading so copyData-backed procedural resources stay entirely in memory.
                if( width && height && !(static_cast<u32>(usage) & static_cast<u32>(TextureUsage::TU_RENDERTARGET)) )
                {
                    Array<u8> pixels(static_cast<size_t>(width)*height*4u,0);
                    texture->copyData(pixels.data(),Vector2I(width,height));
                }
                texture->load( nullptr );
            }
            return texture;
        }

        SmartPtr<ITexture> ClawTextureManager::createRenderTexture()
        {
            auto texture = workphone::make_ptr<ClawTexture>();
            texture->setName( "ClawRenderTexture_" + StringUtil::toString( m_renderTargets.size() ) );
            texture->setUsageFlags( static_cast<u32>( TextureUsage::TU_RENDERTARGET ) );
            texture->setRenderTarget( workphone::make_ptr<ClawRenderTarget>() );
            texture->load( nullptr );

            m_textures.push_back( texture );
            m_renderTargets.push_back( texture );
            return texture;
        }

        void ClawTextureManager::destroyRenderTexture( SmartPtr<ITexture> texture )
        {
            if( !texture )
            {
                return;
            }

            texture->unload( nullptr );
            m_renderTargets.erase(
                std::remove( m_renderTargets.begin(), m_renderTargets.end(), texture ),
                m_renderTargets.end() );
            m_textures.erase( std::remove( m_textures.begin(), m_textures.end(), texture ),
                              m_textures.end() );
        }

        void ClawTextureManager::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = const_cast<ClawTextureManager *>( this );
            }
        }
    }  // namespace render
}  // namespace workphone
