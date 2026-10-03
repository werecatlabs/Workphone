#include <EditorPCH.hpp>
#include <ui/TagManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, TagManager, ISharedObject );

    TagManager::TagManager() = default;

    TagManager::~TagManager() = default;

    Array<String> TagManager::getTags() const
    {
        return m_tags;
    }

    void TagManager::setTags( const Array<String> &tags )
    {
        m_tags.clear();

        for( const auto &tag : tags )
        {
            addTag( tag );
        }
    }

    String TagManager::normaliseTag( const String &tag ) const
    {
        return StringUtil::trim( tag );
    }

    bool TagManager::hasTag( const String &tag ) const
    {
        auto tagName = normaliseTag( tag );
        return std::find( m_tags.begin(), m_tags.end(), tagName ) != m_tags.end();
    }

    bool TagManager::addTag( const String &tag )
    {
        auto tagName = normaliseTag( tag );
        if( StringUtil::isNullOrEmpty( tagName ) )
        {
            return false;
        }

        if( hasTag( tagName ) )
        {
            return false;
        }

        m_tags.push_back( tagName );
        std::sort( m_tags.begin(), m_tags.end() );
        return true;
    }

    bool TagManager::removeTag( const String &tag )
    {
        auto tagName = normaliseTag( tag );
        auto it = std::find( m_tags.begin(), m_tags.end(), tagName );
        if( it == m_tags.end() )
        {
            return false;
        }

        m_tags.erase( it );

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
                    removeActorTagRecursive( actor, tagName );
                }
            }
        }

        return true;
    }

    void TagManager::refreshFromScene()
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
            addActorTagsRecursive( actor );
        }
    }

    void TagManager::addActorTagsRecursive( SmartPtr<scene::IGameActor> actor )
    {
        if( !actor )
        {
            return;
        }

        auto tags = actor->getTags();
        for( const auto &tag : tags )
        {
            addTag( tag );
        }

        auto children = actor->getChildren();
        for( auto child : children )
        {
            addActorTagsRecursive( child );
        }
    }

    void TagManager::removeActorTagRecursive( SmartPtr<scene::IGameActor> actor, const String &tag )
    {
        if( !actor )
        {
            return;
        }

        actor->removeTag( tag );

        auto children = actor->getChildren();
        for( auto child : children )
        {
            removeActorTagRecursive( child, tag );
        }
    }
}  // namespace workphone::editor
