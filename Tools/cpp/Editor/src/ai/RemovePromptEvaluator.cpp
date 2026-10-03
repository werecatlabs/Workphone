#include <EditorPCH.hpp>
#include <ai/RemovePromptEvaluator.hpp>
#include <Workphone/Workphone.hpp>
#include "commands/AddActorCmd.hpp"
#include "commands/RemoveSelectionCmd.hpp"

namespace workphone::editor
{
    RemovePromptEvaluator::RemovePromptEvaluator( const String &label, const Array<String> &tags ) :
        PromptEvaluator( label, tags )
    {
    }

    void RemovePromptEvaluator::activateGoal()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto selectionManager = applicationManager->getSelectionManager();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();
        auto typeManager = TypeManager::instance();

        auto commandManager = applicationManager->getCommandManager();

        selectionManager->clearSelection();

        auto selection = selectionManager->getSelection();
        if( selection.empty() )
        {
            auto actors = scene->getActors();
            for( auto actor : actors )
            {
                auto actorName = actor->getName();
                for( auto entity : m_namedEntities )
                {
                    if( StringUtil::numCommonSubsequence( actorName, entity ) > entity.size() * 0.8 )
                    {
                        selectionManager->addSelectedObject( actor );
                    }
                    if( StringUtil::numCommonSubsequence( StringUtil::make_lower( actorName ), entity ) >
                        entity.size() * 0.8 )
                    {
                        selectionManager->addSelectedObject( actor );
                    }
                }

                auto components = actor->getComponents();
                for( auto component : components )
                {
                    auto typeInfo = component->getTypeInfo();
                    auto typeName = typeManager->getName( typeInfo );

                    for( auto entity : m_namedEntities )
                    {
                        if( StringUtil::numCommonSubsequence( typeName, entity ) > entity.size() * 0.8 )
                        {
                            selectionManager->addSelectedObject( actor );
                        }
                    }
                }
            }
        }

        auto cmd = workphone::make_ptr<RemoveSelectionCmd>();
        commandManager->addCommand( cmd );
    }

    f32 RemovePromptEvaluator::getRating()
    {
        auto rating = 0.0f;

        for( auto entity : m_namedEntities )
        {
            auto entityLowerCase = StringUtil::make_lower( entity );
            rating += entityLowerCase.find( "remove" ) != String::npos ? 1.0f : 0.0f;
            rating += entityLowerCase.find( "delete" ) != String::npos ? 1.0f : 0.0f;
            rating += entityLowerCase.find( "destroy" ) != String::npos ? 1.0f : 0.0f;

            // Check if the entity matches any keyword for goals
            for( auto tag : m_tags )
            {
                if( StringUtil::numCommonSubsequence( tag, entity ) > entity.size() * 0.5f )
                {
                    rating += 0.5f;
                }
            }
        }

        return rating;
    }

    SmartPtr<ISharedObject> RemovePromptEvaluator::getOwner() const
    {
        return m_owner;
    }

    void RemovePromptEvaluator::setOwner( SmartPtr<ISharedObject> owner )
    {
        m_owner = owner;
    }

    f32 RemovePromptEvaluator::getBias() const
    {
        return m_bias;
    }

    void RemovePromptEvaluator::setBias( f32 bias )
    {
        m_bias = bias;
    }
}  // namespace workphone::editor
