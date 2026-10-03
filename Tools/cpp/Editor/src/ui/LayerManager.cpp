#include <EditorPCH.hpp>
#include <ui/LayerManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, LayerManager, ISharedObject );

    namespace
    {
        const String DefaultLayer = "Default";

        bool containsLayer( const Array<String> &layers, const String &layer )
        {
            return std::find( layers.begin(), layers.end(), layer ) != layers.end();
        }
    }  // namespace

    LayerManager::LayerManager()
    {
        m_layers.push_back( DefaultLayer );
    }

    LayerManager::~LayerManager() = default;

    Array<String> LayerManager::getLayers() const
    {
        return m_layers;
    }

    void LayerManager::setLayers( const Array<String> &layers )
    {
        m_layers.clear();
        m_layers.push_back( DefaultLayer );

        for( auto layer : layers )
        {
            addLayer( layer );
        }
    }

    String LayerManager::getDefaultLayer() const
    {
        return DefaultLayer;
    }

    String LayerManager::normaliseLayerName( const String &layer ) const
    {
        auto result = StringUtil::trim( layer );
        if( StringUtil::isNullOrEmpty( result ) )
        {
            result = DefaultLayer;
        }

        return result;
    }

    bool LayerManager::hasLayer( const String &layer ) const
    {
        return containsLayer( m_layers, normaliseLayerName( layer ) );
    }

    bool LayerManager::addLayer( const String &layer )
    {
        auto layerName = normaliseLayerName( layer );
        if( containsLayer( m_layers, layerName ) )
        {
            return false;
        }

        m_layers.push_back( layerName );
        std::sort( m_layers.begin(), m_layers.end() );

        auto defaultIt = std::find( m_layers.begin(), m_layers.end(), DefaultLayer );
        if( defaultIt != m_layers.end() && defaultIt != m_layers.begin() )
        {
            std::iter_swap( m_layers.begin(), defaultIt );
        }

        return true;
    }

    bool LayerManager::removeLayer( const String &layer )
    {
        auto layerName = normaliseLayerName( layer );
        if( layerName == DefaultLayer )
        {
            return false;
        }

        auto it = std::find( m_layers.begin(), m_layers.end(), layerName );
        if( it == m_layers.end() )
        {
            return false;
        }

        m_layers.erase( it );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( applicationManager )
        {
            auto sceneManager = applicationManager->getGameManagerPtr();
            auto currentScene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;
            if( currentScene )
            {
                auto actors = currentScene->getActors();
                for( auto actor : actors )
                {
                    replaceLayerRecursive( actor, layerName, DefaultLayer );
                }
            }
        }

        return true;
    }

    u32 LayerManager::getLayerIndex( const String &layer ) const
    {
        auto layerName = normaliseLayerName( layer );
        auto it = std::find( m_layers.begin(), m_layers.end(), layerName );
        if( it == m_layers.end() )
        {
            return 0;
        }

        return static_cast<u32>( std::distance( m_layers.begin(), it ) );
    }

    String LayerManager::getLayerByIndex( u32 index ) const
    {
        if( index >= m_layers.size() )
        {
            return DefaultLayer;
        }

        return m_layers[index];
    }

    void LayerManager::refreshFromScene()
    {
        addLayer( DefaultLayer );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto sceneManager = applicationManager->getGameManagerPtr();
        auto currentScene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;
        if( !currentScene )
        {
            return;
        }

        auto actors = currentScene->getActors();
        for( auto actor : actors )
        {
            addLayerRecursive( actor );
        }
    }

    void LayerManager::addLayerRecursive( SmartPtr<scene::IGameActor> actor )
    {
        if( !actor )
        {
            return;
        }

        addLayer( actor->getLayer() );

        auto children = actor->getChildren();
        for( auto child : children )
        {
            addLayerRecursive( child );
        }
    }

    void LayerManager::replaceLayerRecursive( SmartPtr<scene::IGameActor> actor, const String &oldLayer,
                                              const String &newLayer )
    {
        if( !actor )
        {
            return;
        }

        if( normaliseLayerName( actor->getLayer() ) == oldLayer )
        {
            actor->setLayer( newLayer );
        }

        auto children = actor->getChildren();
        for( auto child : children )
        {
            replaceLayerRecursive( child, oldLayer, newLayer );
        }
    }
}  // namespace workphone::editor
