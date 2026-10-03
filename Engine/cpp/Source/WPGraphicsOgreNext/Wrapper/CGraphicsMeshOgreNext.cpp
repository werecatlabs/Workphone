#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsMeshOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSceneNodeOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CAnimationControllerOgre.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCubemapOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include "WPGraphicsOgreNext/Wrapper/GraphicsObjectListenerOgreNext.hpp"
#include "WPGraphicsOgreNext/OgreUtil.hpp"
#include <Workphone/Workphone.hpp>
#include <WPOgreDataStream.hpp>
#include <OgreHlms.h>
#include <OgreItem.h>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlmsSamplerblock.h>
#include <OgreMeshManager.h>
#include <OgreMeshManager2.h>
#include <OgreMesh.h>
#include <OgreMesh2.h>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <MeshLoader.hpp>
#include <mutex>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsMeshOgreNext,
                               CGraphicsObjectOgreNext<GraphicsMesh> );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsMeshOgreNext::MeshMaterialEventListener,
                               IEventListener );

    namespace
    {

        String getItemMeshName( const Ogre::Item *item )
        {
            if( !item )
            {
                return {};
            }

            auto mesh = item->getMesh();
            if( !mesh )
            {
                return {};
            }

            return String( mesh->getName().c_str() );
        }

        void destroyOgreItem( Ogre::SceneManager *smgr, Ogre::Item *item )
        {
            if( !item )
            {
                return;
            }

            if( !smgr )
            {
                smgr = item->_getManager();
            }

            item->detachFromParent();
            if( smgr )
            {
                smgr->destroyItem( item );
            }
        }

        Ogre::SubItem *getSubItem( Ogre::Item *item, s32 index )
        {
            if( !item || index < 0 )
            {
                return nullptr;
            }

            auto materialIndex = static_cast<size_t>( index );
            if( materialIndex >= item->getNumSubItems() )
            {
                return nullptr;
            }

            return item->getSubItem( materialIndex );
        }

        Ogre::HlmsPbsDatablock *getPbsDatablock( const SmartPtr<IMaterial> &material )
        {
            auto ogreNextMaterial = workphone::dynamic_pointer_cast<CMaterialOgreNext>( material );
            if( !ogreNextMaterial )
            {
                return nullptr;
            }

            return dynamic_cast<Ogre::HlmsPbsDatablock *>( ogreNextMaterial->getHlmsDatablock() );
        }

        Ogre::TextureGpu *getTextureGpu( const SmartPtr<ITexture> &texture )
        {
            auto ogreNextTexture = workphone::dynamic_pointer_cast<CTextureOgreNext>( texture );
            return ogreNextTexture ? ogreNextTexture->getTexture() : nullptr;
        }

        Ogre::HlmsSamplerblock createReflectionSamplerblock()
        {
            Ogre::HlmsSamplerblock samplerblock;
            samplerblock.mU = Ogre::TAM_CLAMP;
            samplerblock.mV = Ogre::TAM_CLAMP;
            samplerblock.mW = Ogre::TAM_CLAMP;
            return samplerblock;
        }

        bool setReflectionTexture( Ogre::HlmsPbsDatablock *pbsDatablock,
                                   const SmartPtr<ITexture> &texture, bool clearIfTextureUnavailable )
        {
            if( !pbsDatablock )
            {
                return false;
            }

            auto textureGpu = getTextureGpu( texture );
            if( texture && !textureGpu && !clearIfTextureUnavailable )
            {
                return false;
            }

            if( textureGpu )
            {
                auto samplerblock = createReflectionSamplerblock();
                pbsDatablock->setTexture( Ogre::PBSM_REFLECTION, textureGpu, &samplerblock );
            }
            else
            {
                pbsDatablock->setTexture( Ogre::PBSM_REFLECTION,
                                          static_cast<Ogre::TextureGpu *>( nullptr ), nullptr );
            }

            return true;
        }

        Ogre::HlmsPbsDatablock *clonePbsDatablock( Ogre::HlmsPbsDatablock *sourceDatablock )
        {
            if( !sourceDatablock )
            {
                return nullptr;
            }

            auto cloneName = String( "AutomaticCubemap_" ) + StringUtil::getUUID();
            auto clonedDatablock = sourceDatablock->clone( cloneName.c_str() );
            auto pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock *>( clonedDatablock );
            if( !pbsDatablock && clonedDatablock )
            {
                if( auto creator = clonedDatablock->getCreator() )
                {
                    creator->destroyDatablock( clonedDatablock->getName() );
                }
            }

            return pbsDatablock;
        }

        void destroyAutomaticDatablock( Ogre::HlmsDatablock *datablock )
        {
            if( !datablock )
            {
                return;
            }

            if( auto creator = datablock->getCreator() )
            {
                creator->destroyDatablock( datablock->getName() );
            }
        }
    }  // namespace

    CGraphicsMeshOgreNext::CGraphicsMeshOgreNext()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        setupStateObject();

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto factoryManager = graphicsSystem->getFactoryManagerPtr();

        m_materialSharedObjectListener = factoryManager->make_ptr<MeshMaterialEventListener>();
        m_materialSharedObjectListener->setOwner( this );

        m_subMaterials.resize( 4 );

#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif
    }

    CGraphicsMeshOgreNext::CGraphicsMeshOgreNext( SmartPtr<IGraphicsScene> creator )
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        setCreator( creator );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto factoryManager = graphicsSystem->getFactoryManagerPtr();

        m_materialSharedObjectListener = factoryManager->make_ptr<MeshMaterialEventListener>();
        m_materialSharedObjectListener->setOwner( this );

#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif
    }

    CGraphicsMeshOgreNext::~CGraphicsMeshOgreNext()
    {
        const auto &loadingState = getLoadingState();
        if( loadingState != LoadingState::Unloaded )
        {
            unload( nullptr );
        }

        destroyStateContext();
    }

    void CGraphicsMeshOgreNext::setCreator( SmartPtr<IGraphicsScene> creator )
    {
        GraphicsMesh::setCreator( creator );

        if( creator )
        {
            setupStateObject();
        }
    }

    void CGraphicsMeshOgreNext::addMaterialListeners( SmartPtr<IMaterial> material )
    {
        if( !material )
        {
            return;
        }

        if( m_materialStateListener )
        {
            if( auto materialStateObject = material->getStateContext() )
            {
                auto listeners = materialStateObject->getStateListeners();
                if( std::find( listeners.begin(), listeners.end(), m_materialStateListener ) ==
                    listeners.end() )
                {
                    materialStateObject->addStateListener( m_materialStateListener );
                }
            }
        }

        if( m_materialSharedObjectListener )
        {
            material->addObjectListener( m_materialSharedObjectListener );
        }
    }

    void CGraphicsMeshOgreNext::removeMaterialListeners( SmartPtr<IMaterial> material )
    {
        if( !material )
        {
            return;
        }

        if( m_materialStateListener )
        {
            if( auto materialStateObject = material->getStateContext() )
            {
                auto listeners = materialStateObject->getStateListeners();
                if( std::find( listeners.begin(), listeners.end(), m_materialStateListener ) !=
                    listeners.end() )
                {
                    materialStateObject->removeStateListener( m_materialStateListener );
                }
            }
        }

        if( m_materialSharedObjectListener )
        {
            if( material->hasObjectListener( m_materialSharedObjectListener ) )
            {
                material->removeObjectListener( m_materialSharedObjectListener );
            }
        }
    }

    void CGraphicsMeshOgreNext::cacheMaterial( SmartPtr<IMaterial> material, s32 index )
    {
        if( index == -1 )
        {
            m_material = material;
            return;
        }

        if( index < 0 )
        {
            WP_LOG_WARNING( "CGraphicsMeshOgreNext::cacheMaterial - invalid material index." );
            return;
        }

        auto materialIndex = static_cast<size_t>( index );
        if( m_subMaterials.size() <= materialIndex )
        {
            m_subMaterials.resize( materialIndex + 1 );
        }

        m_subMaterials[materialIndex] = material;
    }

    bool CGraphicsMeshOgreNext::isMaterialUsed( SmartPtr<IMaterial> material, s32 ignoredIndex ) const
    {
        if( !material )
        {
            return false;
        }

        if( ignoredIndex != -1 && m_material == material )
        {
            return true;
        }

        for( size_t i = 0; i < m_subMaterials.size(); ++i )
        {
            if( ignoredIndex >= 0 && i == static_cast<size_t>( ignoredIndex ) )
            {
                continue;
            }

            if( m_subMaterials[i] == material )
            {
                return true;
            }
        }

        return false;
    }

    void CGraphicsMeshOgreNext::applyMaterial( SmartPtr<IMaterial> material, s32 index )
    {
        if( material )
        {
            auto pMaterial = workphone::dynamic_pointer_cast<CMaterialOgreNext>( material );
            WP_ASSERT( pMaterial );
            if( !pMaterial )
            {
                return;
            }
        }

        auto previousMaterial = getMaterial( index );
        if( previousMaterial != material )
        {
            if( !isMaterialUsed( previousMaterial, index ) )
            {
                removeMaterialListeners( previousMaterial );
            }
        }

        if( index == -1 )
        {
            if( auto item = getItem() )
            {
                if( material )
                {
                    auto pMaterial = workphone::dynamic_pointer_cast<CMaterialOgreNext>( material );
                    if( pMaterial )
                    {
                        auto datablock = pMaterial->getHlmsDatablock();
                        if( datablock )
                        {
                            item->setDatablock( datablock );
                        }
                        else
                        {
                            // Backend-specific material types may intentionally have no HLMS
                            // datablock. Clear the previous binding instead of rendering with a
                            // stale shader from the formerly selected material type.
                            item->setDatablock( Ogre::IdString() );
                        }
                    }
                }
                else
                {
                    item->setDatablock( Ogre::IdString() );
                }
            }
        }
        else
        {
            if( index < 0 )
            {
                WP_LOG_WARNING( "CGraphicsMeshOgreNext::applyMaterial - invalid material index." );
                return;
            }

            auto materialIndex = static_cast<size_t>( index );
            if( auto item = getItem() )
            {
                const auto numSubItems = item->getNumSubItems();
                if( m_subMaterials.size() < numSubItems )
                {
                    m_subMaterials.resize( numSubItems );
                }

                if( materialIndex < numSubItems )
                {
                    auto subItem = item->getSubItem( materialIndex );
                    if( subItem )
                    {
                        if( material )
                        {
                            auto pMaterial =
                                workphone::dynamic_pointer_cast<CMaterialOgreNext>( material );
                            if( pMaterial )
                            {
                                auto datablock = pMaterial->getHlmsDatablock();
                                if( datablock )
                                {
                                    subItem->setDatablock( datablock );
                                }
                                else
                                {
                                    subItem->setDatablock( Ogre::IdString() );
                                }
                            }
                        }
                        else
                        {
                            subItem->setDatablock( Ogre::IdString() );
                        }
                    }
                }
            }
        }

        cacheMaterial( material, index );
        addMaterialListeners( material );
        applyAutomaticCubemap( material, index );
    }

    CGraphicsMeshOgreNext::AutomaticCubemapBinding &CGraphicsMeshOgreNext::getAutomaticCubemapBinding(
        s32 index )
    {
        if( index == -1 )
        {
            return m_autoCubemapBinding;
        }

        if( index < 0 )
        {
            return m_autoCubemapBinding;
        }

        auto materialIndex = static_cast<size_t>( index );
        if( m_autoSubCubemapBindings.size() <= materialIndex )
        {
            m_autoSubCubemapBindings.resize( materialIndex + 1 );
        }

        return m_autoSubCubemapBindings[materialIndex];
    }

    Vector3F CGraphicsMeshOgreNext::getAutomaticCubemapSamplePosition() const
    {
        if( auto owner = getOwner() )
        {
            return owner->getWorldPosition();
        }

        if( auto item = getItem() )
        {
            if( auto node = item->getParentSceneNode() )
            {
                const auto position = node->_getDerivedPosition();
                return Vector3F( position.x, position.y, position.z );
            }
        }

        return Vector3F();
    }

    void CGraphicsMeshOgreNext::clearAutomaticCubemapBinding( AutomaticCubemapBinding &binding )
    {
        if( binding.renderable && binding.automaticDatablock &&
            binding.renderable->getDatablock() == binding.automaticDatablock )
        {
            binding.renderable->setDatablock( binding.sourceDatablock );
        }

        destroyAutomaticDatablock( binding.automaticDatablock );

        binding.material = nullptr;
        binding.texture = nullptr;
        binding.renderable = nullptr;
        binding.sourceDatablock = nullptr;
        binding.automaticDatablock = nullptr;
        binding.textureGpu = nullptr;
    }

    void CGraphicsMeshOgreNext::applyAutomaticCubemap( SmartPtr<IMaterial> material, s32 index )
    {
        if( index == -1 )
        {
            clearAutomaticCubemapBinding( m_autoCubemapBinding );

            if( auto item = getItem() )
            {
                const auto numSubItems = item->getNumSubItems();
                if( m_autoSubCubemapBindings.size() < numSubItems )
                {
                    m_autoSubCubemapBindings.resize( numSubItems );
                }

                for( size_t subIndex = 0; subIndex < numSubItems; ++subIndex )
                {
                    applyAutomaticCubemap( material, static_cast<s32>( subIndex ) );
                }
            }

            return;
        }

        auto &binding = getAutomaticCubemapBinding( index );
        auto item = getItem();
        auto subItem = getSubItem( item, index );

        if( !isLoaded() || !item || !subItem || !material )
        {
            clearAutomaticCubemapBinding( binding );
            return;
        }

        auto scene = workphone::dynamic_pointer_cast<CGraphicsSceneOgreNext>( getCreator() );
        if( !scene )
        {
            clearAutomaticCubemapBinding( binding );
            return;
        }

        auto cubemap =
            scene->findBestCubemap( getAutomaticCubemapSamplePosition(), getVisibilityFlags() );
        auto selectedTexture = cubemap ? cubemap->getTexture() : SmartPtr<ITexture>();
        auto selectedTextureGpu = getTextureGpu( selectedTexture );
        auto sourceDatablock = getPbsDatablock( material );

        if( binding.material &&
            ( binding.material != material || binding.texture != selectedTexture ||
              binding.textureGpu != selectedTextureGpu || binding.renderable != subItem ||
              binding.sourceDatablock != sourceDatablock ) )
        {
            clearAutomaticCubemapBinding( binding );
        }

        if( !selectedTexture || !selectedTextureGpu || !sourceDatablock )
        {
            clearAutomaticCubemapBinding( binding );
            return;
        }

        if( binding.automaticDatablock && subItem->getDatablock() != binding.automaticDatablock )
        {
            clearAutomaticCubemapBinding( binding );
        }

        if( !binding.automaticDatablock )
        {
            auto automaticDatablock = clonePbsDatablock( sourceDatablock );
            if( !automaticDatablock )
            {
                clearAutomaticCubemapBinding( binding );
                return;
            }

            binding.material = material;
            binding.texture = selectedTexture;
            binding.renderable = subItem;
            binding.sourceDatablock = subItem->getDatablock();
            binding.automaticDatablock = automaticDatablock;

            subItem->setDatablock( automaticDatablock );
        }

        if( !setReflectionTexture( binding.automaticDatablock, selectedTexture, false ) )
        {
            clearAutomaticCubemapBinding( binding );
            return;
        }

        if( binding.automaticDatablock )
        {
            binding.material = material;
            binding.texture = selectedTexture;
            binding.renderable = subItem;
            binding.sourceDatablock = sourceDatablock;
            binding.textureGpu = selectedTextureGpu;
        }
    }

    void CGraphicsMeshOgreNext::updateAutomaticCubemap()
    {
        if( !isLoaded() || !getItem() )
        {
            clearAutomaticCubemapBinding( m_autoCubemapBinding );
            for( auto &binding : m_autoSubCubemapBindings )
            {
                clearAutomaticCubemapBinding( binding );
            }
            return;
        }

        clearAutomaticCubemapBinding( m_autoCubemapBinding );

        const auto numSubItems = getItem()->getNumSubItems();
        if( m_autoSubCubemapBindings.size() < numSubItems )
        {
            m_autoSubCubemapBindings.resize( numSubItems );
        }
        else
        {
            for( size_t index = numSubItems; index < m_autoSubCubemapBindings.size(); ++index )
            {
                clearAutomaticCubemapBinding( m_autoSubCubemapBindings[index] );
            }
        }

        auto globalMaterial = getMaterial( -1 );
        for( size_t index = 0; index < numSubItems; ++index )
        {
            auto material = getMaterial( static_cast<s32>( index ) );
            if( !material )
            {
                material = globalMaterial;
            }

            applyAutomaticCubemap( material, static_cast<s32>( index ) );
        }
    }

    void CGraphicsMeshOgreNext::setMaterialName( const String &materialName, s32 index )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto materialManager = graphicsSystem->getMaterialManager();
            WP_ASSERT( materialManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( index == -1 )
            {
                m_materialName = materialName;
            }
            else if( index >= 0 )
            {
                auto materialIndex = static_cast<size_t>( index );
                if( m_subMaterialNames.size() <= materialIndex )
                {
                    m_subMaterialNames.resize( materialIndex + 1 );
                }

                m_subMaterialNames[materialIndex] = materialName;
            }
            else
            {
                WP_LOG_WARNING( "CGraphicsMeshOgreNext::setMaterialName - invalid material index." );
                return;
            }

            if( loadingState == LoadingState::Loaded && task != renderTask )
            {
                auto message = factoryManager->make_ptr<StateMessageMaterialName>();
                message->setSender( this );
                message->setMaterialName( materialName );
                message->setIndex( static_cast<u32>( index ) );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, message );
                }

                return;
            }

            auto material = materialManager->getByName( materialName );
            if( material )
            {
                setMaterial( material, index );
                return;
            }

            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                ScopedLock graphicsLock( graphicsSystem );

                if( index == -1 )
                {
                    if( m_entity )
                    {
                        m_entity->setMaterialName( materialName.c_str() );
                    }

                    if( auto item = getItem() )
                    {
                        item->setDatablockOrMaterialName(
                            materialName.c_str(),
                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                    }
                }
                else
                {
                    if( m_entity )
                    {
                        auto subEntity = m_entity->getSubEntity( index );
                        if( subEntity )
                        {
                            subEntity->setMaterialName(
                                materialName.c_str(),
                                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                        }
                    }

                    if( auto item = getItem() )
                    {
                        auto materialIndex = static_cast<size_t>( index );
                        if( materialIndex < item->getNumSubItems() )
                        {
                            auto subItem = item->getSubItem( materialIndex );
                            if( subItem )
                            {
                                subItem->setDatablockOrMaterialName(
                                    materialName.c_str(),
                                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                            }
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String CGraphicsMeshOgreNext::getMaterialName( s32 index ) const
    {
        if( auto material = getMaterial( index ) )
        {
            return material->getName();
        }

        if( index == -1 )
        {
            return m_materialName;
        }

        if( index >= 0 && static_cast<size_t>( index ) < m_subMaterialNames.size() )
        {
            return m_subMaterialNames[static_cast<size_t>( index )];
        }

        return {};
    }

    void CGraphicsMeshOgreNext::setMaterial( SmartPtr<IMaterial> material, s32 index )
    {
        try
        {
            if( material )
            {
                auto pMaterial = workphone::dynamic_pointer_cast<CMaterialOgreNext>( material );
                WP_ASSERT( pMaterial );
                if( !pMaterial )
                {
                    return;
                }
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                if( task == renderTask )
                {
                    if( material && !material->isLoaded() )
                    {
                        material->load( nullptr );
                    }

                    ScopedLock graphicsLock( graphicsSystem );
                    applyMaterial( material, index );

                    return;
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageMaterial>();
                    message->setSender( this );
                    message->setMaterial( material );
                    message->setIndex( index );

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->addMessage( renderTask, message );
                    }

                    return;
                }
            }

            auto previousMaterial = getMaterial( index );
            if( previousMaterial != material )
            {
                if( !isMaterialUsed( previousMaterial, index ) )
                {
                    removeMaterialListeners( previousMaterial );
                }
            }

            cacheMaterial( material, index );
            addMaterialListeners( material );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IMaterial> CGraphicsMeshOgreNext::getMaterial( s32 index ) const
    {
        if( index == -1 )
        {
            return m_material;
        }

        if( index >= 0 && static_cast<size_t>( index ) < m_subMaterials.size() )
        {
            return m_subMaterials[static_cast<size_t>( index )];
        }

        return nullptr;
    }

    SmartPtr<IGraphicsObject> CGraphicsMeshOgreNext::clone( const String &name ) const
    {
        auto mesh = workphone::make_ptr<CGraphicsMeshOgreNext>();
        auto data = toData();
        mesh->fromData( data );
        return mesh;
    }

    void CGraphicsMeshOgreNext::_getObject( void **ppObject ) const
    {
        if( m_entity )
        {
            *ppObject = m_entity;
        }
        else if( m_item )
        {
            *ppObject = m_item;
        }
        else
        {
            *ppObject = nullptr;
        }
    }

    void CGraphicsMeshOgreNext::checkVertexProcessing()
    {
        m_checkVertProcessing = true;
    }

    SmartPtr<IAnimationController> CGraphicsMeshOgreNext::getAnimationController()
    {
        if( !m_animationController )
        {
            auto animationController = workphone::make_ptr<CAnimationController>();
            m_animationController = animationController;
        }

        return m_animationController;
    }

    Ogre::v1::Entity *CGraphicsMeshOgreNext::getEntity() const
    {
        return m_entity;
    }

    Ogre::MeshPtr CGraphicsMeshOgreNext::createMesh( const String &meshName )
    {
        try
        {
            ScopedLock lock( this );

            Ogre::SceneManager *smgr = nullptr;

            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();

            auto meshManagerV2 = Ogre::MeshManager::getSingletonPtr();

            auto meshExtention = Path::getFileExtension( meshName );
            static const auto engineMeshExtention = String( ".fbmeshbin" );
            static const auto ogreMeshExtention = String( ".mesh" );

            auto meshNameWithoutExtention = Path::getFileNameWithoutExtension( meshName );
            auto ogreMeshNameWithoutExtention = OgreUtil::toString( meshNameWithoutExtention );

            if( meshExtention == engineMeshExtention )
            {
                m_graphicsMeshManualResourceLoader.setFilePath( meshName );
                const auto progressiveMeshOptions = getProgressiveMeshOptions();
                m_graphicsMeshManualResourceLoader.setProgressiveMeshOptions(
                    progressiveMeshOptions );

                auto resourceGroupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
                auto variantName = meshNameWithoutExtention;
                if( progressiveMeshOptions.isEnabled() )
                {
                    variantName += "_lod_" +
                                   StringUtil::toString( progressiveMeshOptions.getHash() );
                }
                auto ogreMeshName = OgreUtil::toString( variantName );

                auto meshResult = meshManagerV2->getByName( ogreMeshName, resourceGroupName );
                if( !meshResult )
                {
                    auto mesh = meshManagerV2->createManual( ogreMeshName, resourceGroupName,
                                                             &m_graphicsMeshManualResourceLoader );
                    if( mesh )
                    {
                        return mesh;
                    }
                }
                else
                {
                    auto mesh = meshManagerV2->load( ogreMeshName, resourceGroupName );
                    if( !mesh )
                    {
                        WP_LOG_ERROR( "CGraphicsMeshOgreNext::createMesh - unable to load mesh: " +
                                      ogreMeshName );
                    }

                    return mesh;
                }
            }
            else if( meshExtention == ogreMeshExtention )
            {
                auto meshManagerV1 = Ogre::v1::MeshManager::getSingletonPtr();
                auto resourceGroupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

                auto meshResult = meshManagerV1->createOrRetrieve( meshName.c_str(), resourceGroupName );
                if( meshResult.second )
                {
                    auto meshV1 = meshResult.first.dynamicCast<Ogre::v1::Mesh>();

                    auto meshV2 = meshManagerV2->createManual( meshName.c_str(), resourceGroupName );
                    meshV2->importV1( meshV1.get(), false, false, false );
                    return meshV2;
                }

                auto mesh = meshManagerV2->load( meshName.c_str(), resourceGroupName );
                return mesh;
            }
        }
        catch( Ogre::Exception &e )
        {
            auto error = e.getFullDescription();
            WP_LOG_ERROR( error.c_str() );
        }

        return {};
    }

    void CGraphicsMeshOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this );

            if( !getStateContext() || !m_materialStateListener )
            {
                setupStateObject();
            }

            setLoadingState( LoadingState::Loading );

            // Resolve the Ogre scene manager from our creator.
            Ogre::SceneManager *smgr = nullptr;
            if( auto creator = getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            if( !smgr )
            {
                WP_LOG_ERROR(
                    "CGraphicsMeshOgreNext::load - no scene manager available; cannot create item." );
                setLoadingState( LoadingState::Error );
                return;
            }

            // Create and register the Ogre item if a mesh name has been set.
            const auto meshName = getMeshName();
            if( !StringUtil::isNullOrEmpty( meshName ) )
            {
                auto *item = getItem();
                WP_ASSERT( item == nullptr );

                if( !item )
                {
                    auto mesh = createMesh( meshName );
                    if( !mesh )
                    {
                        WP_LOG_ERROR( "CGraphicsMeshOgreNext::load - unable to create mesh: " +
                                      meshName );
                        setLoadingState( LoadingState::Error );
                        return;
                    }

                    item = smgr->createItem( mesh );
                    if( !item )
                    {
                        WP_LOG_ERROR(
                            "CGraphicsMeshOgreNext::load - scene manager returned null item for mesh: " +
                            meshName );
                        setLoadingState( LoadingState::Error );
                        return;
                    }

                    setItem( item );
                    setGraphicsObject( item );
                }

                item->setName( meshName.c_str() );
                item->setCastShadows( getCastShadows() );
            }

            // Apply materials to sub-items.
            if( auto item = getItem() )
            {
                const auto numSubItems = item->getNumSubItems();
                if( m_subMaterials.size() < numSubItems )
                {
                    m_subMaterials.resize( numSubItems );
                }

                if( auto material = m_material.load() )
                {
                    if( !material->isLoaded() )
                    {
                        material->load( nullptr );
                    }

                    applyMaterial( material, -1 );
                }
                else if( !StringUtil::isNullOrEmpty( m_materialName ) )
                {
                    item->setDatablockOrMaterialName(
                        m_materialName.c_str(),
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                }

                for( size_t index = 0; index < numSubItems; ++index )
                {
                    auto *subItem = item->getSubItem( index );
                    if( subItem )
                    {
                        auto material = m_subMaterials[index];
                        if( material )
                        {
                            if( !material->isLoaded() )
                            {
                                material->load( nullptr );
                            }

                            applyMaterial( material, static_cast<s32>( index ) );
                        }
                        else if( index < m_subMaterialNames.size() )
                        {
                            auto materialName = m_subMaterialNames[index];
                            if( !StringUtil::isNullOrEmpty( materialName ) )
                            {
                                subItem->setDatablockOrMaterialName(
                                    materialName.c_str(),
                                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                            }
                        }
                    }
                }
            }
            else if( !StringUtil::isNullOrEmpty( getMeshName() ) )
            {
                // A mesh name was set but no item was produced — flag as an error.
                WP_LOG_ERROR( "CGraphicsMeshOgreNext::load - no item after loading mesh: " +
                              getMeshName() );
                setLoadingState( LoadingState::Error );
                return;
            }

            setLoadingState( LoadingState::Loaded );
            updateAutomaticCubemap();
        }
        catch( Ogre::Exception &e )
        {
            auto message = "CGraphicsMeshOgreNext::load - Ogre exception: " + e.getFullDescription();
            WP_LOG_ERROR( message );
            setLoadingState( LoadingState::Error );
        }
        catch( std::exception &e )
        {
            auto message = String( "CGraphicsMeshOgreNext::load - exception: " ) + e.what();
            WP_LOG_ERROR( message );
            setLoadingState( LoadingState::Error );
        }
    }

    void CGraphicsMeshOgreNext::reload( SmartPtr<ISharedObject> data )
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

    void CGraphicsMeshOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto hasOgreObject = m_entity || getItem();
            if( isLoaded() || hasOgreObject || m_materialStateListener )
            {
                setLoadingState( LoadingState::Unloading );

                ScopedLock lock( this );

                clearAutomaticCubemapBinding( m_autoCubemapBinding );
                for( auto &binding : m_autoSubCubemapBindings )
                {
                    clearAutomaticCubemapBinding( binding );
                }
                m_autoSubCubemapBindings.clear();

                Ogre::SceneManager *ogreSmgr = nullptr;

                if( auto creator = getCreator() )
                {
                    auto smgr = workphone::static_pointer_cast<CGraphicsSceneOgreNext>( creator );
                    ogreSmgr = smgr->getSceneManager();

                    if( m_entity )
                    {
                        m_entity->detachFromParent();

                        if( ogreSmgr )
                        {
                            ogreSmgr->destroyEntity( m_entity );
                        }

                        m_entity = nullptr;
                    }
                }

                if( auto item = getItem() )
                {
                    destroyOgreItem( ogreSmgr, item );
                    setItem( nullptr );
                }

                //registerForUpdates( false );

                removeMaterialListeners( m_material.load() );
                for( auto material : m_subMaterials )
                {
                    removeMaterialListeners( material );
                }

                if( m_materialStateListener )
                {
                    m_materialStateListener->unload( data );
                    m_materialStateListener = nullptr;
                }

                setItem( nullptr );
                setGraphicsObject( nullptr );

                CGraphicsObjectOgreNext<GraphicsMesh>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Ogre::Item *CGraphicsMeshOgreNext::getItem() const
    {
        return m_item;
    }

    void CGraphicsMeshOgreNext::setItem( Ogre::Item *item )
    {
        m_item = item;
    }

    SmartPtr<Properties> CGraphicsMeshOgreNext::getProperties() const
    {
        auto properties = CGraphicsObjectOgreNext<GraphicsMesh>::getProperties();

        auto meshName = getMeshName();
        properties->setProperty( IGraphicsMesh::meshNamePropertyStr, meshName );

        auto castShadows = getCastShadows();
        properties->setProperty( IGraphicsMesh::castShadowsPropertyStr, castShadows );

        return properties;
    }

    void CGraphicsMeshOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        CGraphicsObjectOgreNext<GraphicsMesh>::setProperties( properties );

        auto meshName = getMeshName();
        properties->getPropertyValue( IGraphicsMesh::meshNamePropertyStr, meshName );
        setMeshName( meshName );

        auto castShadows = getCastShadows();
        if( properties->getPropertyValue( IGraphicsMesh::castShadowsPropertyStr, castShadows ) )
        {
            setCastShadows( castShadows );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                if( graphicsSystem )
                {
                    auto renderTask = graphicsSystem->getRenderTask();
                    auto task = Thread::getCurrentTask();

                    if( task == renderTask )
                    {
                        if( auto item = getItem() )
                        {
                            item->setCastShadows( castShadows );
                        }
                    }
                }
            }
        }
    }

    Array<SmartPtr<ISharedObject>> CGraphicsMeshOgreNext::getChildObjects() const
    {
        Array<SmartPtr<ISharedObject>> childObjects;
        childObjects.reserve( 4 );

        if( auto material = getMaterial() )
        {
            childObjects.emplace_back( material );
        }

        if( !m_subMaterials.empty() )
        {
            for( auto material : m_subMaterials )
            {
                childObjects.emplace_back( material );
            }
        }

        childObjects.emplace_back( getStateContext() );
        childObjects.emplace_back( getStateListener() );
        childObjects.emplace_back( getOwner() );

        return childObjects;
    }

    void CGraphicsMeshOgreNext::materialLoaded( SmartPtr<IMaterial> material )
    {
        try
        {
            if( !material || !isLoaded() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto renderTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            if( task != renderTask )
            {
                auto factoryManager = applicationManager->getFactoryManagerPtr();
                WP_ASSERT( factoryManager );

                auto message = factoryManager->make_ptr<StateMessageLoad>();
                message->setType( StateMessageLoad::LOADED_HASH );
                message->setSender( material );
                message->setObject( material );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->addMessage( renderTask, message );
                }

                return;
            }

            ScopedLock lock( this, true );

            if( material == m_material )
            {
                applyMaterial( material, -1 );
            }

            for( size_t i = 0; i < m_subMaterials.size(); ++i )
            {
                auto currentMaterial = m_subMaterials[i];

                if( material == currentMaterial )
                {
                    applyMaterial( material, static_cast<s32>( i ) );
                }
            }

            updateAutomaticCubemap();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CGraphicsMeshOgreNext::setupStateObject()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManagerPtr();
        WP_ASSERT( stateManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = graphicsSystem->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto sender = getCreator();
        if( !sender )
        {
            return;
        }

        auto stateContext = sender->getGraphicsObjectContextPtr( getTypeInfo() );
        WP_ASSERT( stateContext );
        if( !stateContext )
        {
            return;
        }

        if( auto currentStateContext = getStateContext() )
        {
            if( currentStateContext.get() != stateContext )
            {
                currentStateContext->removeStatesById( getId() );
            }
        }

        setStateContext( stateContext );

        if( !stateContext->getStateById( getId() ) )
        {
            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<GraphicsMeshState>();
            state->setData( stateData );
        }

        if( !m_materialStateListener )
        {
            auto materialStateListener = factoryManager->make_ptr<GraphicsObjectListenerOgreNext>();
            materialStateListener->setOwner( this );
            m_materialStateListener = materialStateListener;
        }
    }

    void CGraphicsMeshOgreNext::setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton )
    {
        m_skeleton = skeleton;
    }

    SmartPtr<IGraphicsSkeleton> CGraphicsMeshOgreNext::getSkeleton() const
    {
        return m_skeleton;
    }

    bool CGraphicsMeshOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( !message )
        {
            return false;
        }

        if( CGraphicsObjectOgreNext<GraphicsMesh>::handleStateMessage( message ) )
        {
            return true;
        }

        if( message->isExactly<StateMessageLoad>() )
        {
            auto stateMessageLoad = workphone::static_pointer_cast<StateMessageLoad>( message );
            auto eventType = stateMessageLoad->getType();

            if( eventType == StateMessageLoad::LOADED_HASH )
            {
                if( isLoaded() )
                {
                    auto materialObject =
                        workphone::dynamic_pointer_cast<IMaterial>( stateMessageLoad->getObject() );
                    if( materialObject )
                    {
                        materialLoaded( materialObject );
                    }
                }
            }
        }

        if( message->getSender() == this )
        {
            if( message->isExactly<StateMessageMaterialName>() )
            {
                auto materialMessage =
                    workphone::static_pointer_cast<StateMessageMaterialName>( message );
                auto materialName = materialMessage->getMaterialName();
                auto materialIndex = materialMessage->getIndex();
                auto index =
                    materialIndex == static_cast<u32>( -1 ) ? -1 : static_cast<s32>( materialIndex );
                this->setMaterialName( materialName, index );

                return true;
            }
            else if( message->isExactly<StateMessageMaterial>() )
            {
                auto materialMessage = workphone::static_pointer_cast<StateMessageMaterial>( message );
                auto material = materialMessage->getMaterial();
                auto materialIndex = materialMessage->getIndex();
                this->setMaterial( material, materialIndex );

                return true;
            }
            else if( message->isExactly<StateMessageVisible>() )
            {
                auto visibleMessage = workphone::static_pointer_cast<StateMessageVisible>( message );
                auto bIsVisible = visibleMessage->isVisible();
                this->setVisible( bIsVisible );

                return true;
            }
            else if( message->isExactly<StateMessageUIntValue>() )
            {
                auto intValueMessage = workphone::static_pointer_cast<StateMessageUIntValue>( message );
                auto type = intValueMessage->getType();
                auto value = intValueMessage->getValue();

                if( type == RENDER_QUEUE_HASH )
                {
                    this->setRenderQueueGroup( static_cast<u8>( value ) );
                }
                else if( type == VISIBILITY_FLAGS_HASH )
                {
                    this->setVisibilityFlags( value );
                }

                return true;
            }
        }

        return false;
    }

    bool CGraphicsMeshOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        return CGraphicsObjectOgreNext<GraphicsMesh>::handleStateChanged( state );
    }

    CGraphicsMeshOgreNext::MeshMaterialEventListener::~MeshMaterialEventListener() = default;

    CGraphicsMeshOgreNext::MeshMaterialEventListener::MeshMaterialEventListener() = default;

    void CGraphicsMeshOgreNext::MeshMaterialEventListener::setOwner(
        SmartPtr<CGraphicsMeshOgreNext> owner )
    {
        m_owner = owner;
    }

    SmartPtr<CGraphicsMeshOgreNext> CGraphicsMeshOgreNext::MeshMaterialEventListener::getOwner() const
    {
        return m_owner.lock();
    }

    Parameter CGraphicsMeshOgreNext::MeshMaterialEventListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::loadingStateChanged && arguments.size() > 1 )
        {
            auto newState = static_cast<LoadingState>( arguments[1].getS32() );
            if( newState == LoadingState::Loaded )
            {
                if( auto owner = getOwner() )
                {
                    auto materialObject = workphone::dynamic_pointer_cast<IMaterial>( sender );
                    if( materialObject )
                    {
                        owner->materialLoaded( materialObject );
                    }
                }
            }
        }

        return {};
    }

    void CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::prepareResource(
        Ogre::Resource *resource )
    {
        if( !resource )
        {
            return;
        }

        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto meshManager = applicationManager->getMeshManager();
            if( !meshManager )
            {
                return;
            }

            const auto meshName = getFilePath();

            if( auto meshResourceObject = meshManager->loadFromFile( meshName ) )
            {
                auto meshResource = workphone::static_pointer_cast<IMeshResource>( meshResourceObject );
                if( meshResource && !meshResource->isLoaded() )
                {
                    meshResource->load( nullptr );
                }
            }
        }
        catch( Ogre::Exception &e )
        {
            auto error =
                String(
                    "CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::prepareResource - "
                    "Ogre exception: " ) +
                e.getFullDescription();
            WP_LOG_ERROR( error );
            throw;
        }
        catch( std::exception &e )
        {
            auto error =
                String(
                    "CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::prepareResource - "
                    "exception: " ) +
                e.what();
            WP_LOG_ERROR( error );
            throw;
        }
    }

    void CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::loadResource(
        Ogre::Resource *resource )
    {
        if( !resource )
        {
            WP_LOG_ERROR(
                "CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::loadResource - null "
                "resource." );
            return;
        }

        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( !applicationManager )
            {
                OGRE_EXCEPT( Ogre::Exception::ERR_INVALID_STATE, "Application manager is unavailable.",
                             "GraphicsMeshManualResourceLoader::loadResource" );
            }

            auto resourceDatabase = applicationManager->getResourceDatabase();

            const auto meshName = getFilePath();

            WP_ASSERT( dynamic_cast<Ogre::Mesh *>( resource ) );
            auto mesh = static_cast<Ogre::Mesh *>( resource );
            if( mesh->getNumSubMeshes() == 0 )
            {
                MeshLoader::loadFBMesh( mesh, meshName, getProgressiveMeshOptions() );
            }
        }
        catch( Ogre::Exception &e )
        {
            auto error = String(
                             "CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::loadResource - "
                             "Ogre exception: " ) +
                         e.getFullDescription();
            WP_LOG_ERROR( error );
            throw;
        }
        catch( std::exception &e )
        {
            auto error = String(
                             "CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::loadResource - "
                             "exception: " ) +
                         e.what();
            WP_LOG_ERROR( error );
            throw;
        }
    }

    CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::~GraphicsMeshManualResourceLoader() =
        default;

    CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::GraphicsMeshManualResourceLoader() =
        default;

    String CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::getFilePath() const
    {
        return m_meshFilePath.load();
    }

    void CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::setFilePath( const String &filePath )
    {
        WP_ASSERT( filePath.size() <= m_meshFilePath.load().capacity() );
        m_meshFilePath.store( filePath );
    }

    ProgressiveMeshOptions
    CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::getProgressiveMeshOptions() const
    {
        return m_progressiveMeshOptions.load();
    }

    void CGraphicsMeshOgreNext::GraphicsMeshManualResourceLoader::setProgressiveMeshOptions(
        const ProgressiveMeshOptions &options )
    {
        m_progressiveMeshOptions.store( options );
    }

}  // namespace workphone::render
