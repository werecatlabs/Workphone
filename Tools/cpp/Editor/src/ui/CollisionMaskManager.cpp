#include <EditorPCH.hpp>
#include <ui/CollisionMaskManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, CollisionMaskManager, ISharedObject );

    namespace
    {
        const String DefaultMaskName = "Default";
        const String AllMaskName = "All";

        String makeMaskName( u32 mask )
        {
            return String( "Mask " ) + StringUtil::toString( mask );
        }
    }  // namespace

    CollisionMaskManager::CollisionMaskManager()
    {
        m_options.push_back( Option{ DefaultMaskName, 0 } );
        m_options.push_back( Option{ AllMaskName, std::numeric_limits<u32>::max() } );
    }

    CollisionMaskManager::~CollisionMaskManager() = default;

    Array<CollisionMaskManager::Option> CollisionMaskManager::getOptions() const
    {
        return m_options;
    }

    Array<String> CollisionMaskManager::getOptionLabels() const
    {
        auto labels = Array<String>();
        labels.reserve( m_options.size() );

        for( const auto &option : m_options )
        {
            labels.push_back( option.name + " (" + StringUtil::toString( option.mask ) + ")" );
        }

        return labels;
    }

    String CollisionMaskManager::getDefaultOptionName() const
    {
        return DefaultMaskName;
    }

    u32 CollisionMaskManager::getDefaultMask() const
    {
        return 0;
    }

    String CollisionMaskManager::normaliseOptionName( const String &name, u32 mask ) const
    {
        auto result = StringUtil::trim( name );
        if( StringUtil::isNullOrEmpty( result ) )
        {
            result = makeMaskName( mask );
        }

        return result;
    }

    bool CollisionMaskManager::addOption( const String &name, u32 mask )
    {
        auto optionName = normaliseOptionName( name, mask );

        for( auto &option : m_options )
        {
            if( option.name == optionName )
            {
                option.mask = mask;
                return false;
            }

            if( option.mask == mask )
            {
                return false;
            }
        }

        m_options.push_back( Option{ optionName, mask } );
        std::sort( m_options.begin(), m_options.end(), []( const Option &a, const Option &b ) {
            if( a.mask == b.mask )
            {
                return a.name < b.name;
            }

            return a.mask < b.mask;
        } );

        auto defaultIt = std::find_if( m_options.begin(), m_options.end(), []( const Option &option ) {
            return option.name == DefaultMaskName;
        } );
        if( defaultIt != m_options.end() && defaultIt != m_options.begin() )
        {
            std::iter_swap( m_options.begin(), defaultIt );
        }

        return true;
    }

    bool CollisionMaskManager::removeOption( const String &name )
    {
        auto optionName = StringUtil::trim( name );
        if( optionName == DefaultMaskName )
        {
            return false;
        }

        auto it = std::find_if( m_options.begin(), m_options.end(), [&]( const Option &option ) {
            return option.name == optionName;
        } );
        if( it == m_options.end() )
        {
            return false;
        }

        m_options.erase( it );
        return true;
    }

    u32 CollisionMaskManager::getOptionIndexByMask( u32 mask ) const
    {
        auto it = std::find_if( m_options.begin(), m_options.end(), [&]( const Option &option ) {
            return option.mask == mask;
        } );
        if( it == m_options.end() )
        {
            return 0;
        }

        return static_cast<u32>( std::distance( m_options.begin(), it ) );
    }

    u32 CollisionMaskManager::getOptionIndexByName( const String &name ) const
    {
        auto optionName = StringUtil::trim( name );
        auto it = std::find_if( m_options.begin(), m_options.end(), [&]( const Option &option ) {
            return option.name == optionName;
        } );
        if( it == m_options.end() )
        {
            return 0;
        }

        return static_cast<u32>( std::distance( m_options.begin(), it ) );
    }

    u32 CollisionMaskManager::getMaskByIndex( u32 index ) const
    {
        if( index >= m_options.size() )
        {
            return getDefaultMask();
        }

        return m_options[index].mask;
    }

    String CollisionMaskManager::getOptionNameByIndex( u32 index ) const
    {
        if( index >= m_options.size() )
        {
            return DefaultMaskName;
        }

        return m_options[index].name;
    }

    void CollisionMaskManager::refreshFromScene()
    {
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
            addActorMaskRecursive( actor );
        }
    }

    void CollisionMaskManager::addActorMaskRecursive( SmartPtr<scene::IGameActor> actor )
    {
        if( !actor )
        {
            return;
        }

        const auto mask = actor->getCollisionMask();
        addOption( makeMaskName( mask ), mask );

        auto children = actor->getChildren();
        for( auto child : children )
        {
            addActorMaskRecursive( child );
        }
    }
}  // namespace workphone::editor
