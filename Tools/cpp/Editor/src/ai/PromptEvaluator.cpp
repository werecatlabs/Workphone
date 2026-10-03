#include <EditorPCH.hpp>
#include <ai/PromptEvaluator.hpp>
#include <Workphone/Workphone.hpp>
#include <utility>
#include "commands/AddActorCmd.hpp"
#include "commands/RemoveSelectionCmd.hpp"

namespace workphone::editor
{
    PromptEvaluator::PromptEvaluator() = default;

    PromptEvaluator::PromptEvaluator( const String &label, const Array<String> &tags ) :
        m_tags( tags ),
        m_label( label )
    {
    }

    PromptEvaluator::~PromptEvaluator() = default;

    void PromptEvaluator::activateGoal()
    {
        //auto applicationManager = core::IApplicationManager::instance();
        //auto commandManager = applicationManager->getCommandManager();

        //auto cmd = fb::make_ptr<AddActorCmd>();
        //cmd->setActorType( m_actorType );
        //commandManager->addCommand( cmd );
    }

    f32 PromptEvaluator::getRating()
    {
        auto rating = 0.0f;

        for( auto entity : m_namedEntities )
        {
            rating += entity.find( "add" ) != String::npos ? 1.0f : 0.0f;
            rating += entity.find( "create" ) != String::npos ? 1.0f : 0.0f;
            rating += entity.find( "make" ) != String::npos ? 1.0f : 0.0f;

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

    SmartPtr<ISharedObject> PromptEvaluator::getOwner() const
    {
        return m_owner;
    }

    void PromptEvaluator::setOwner( SmartPtr<ISharedObject> owner )
    {
        m_owner = owner;
    }

    f32 PromptEvaluator::getBias() const
    {
        return m_bias;
    }

    void PromptEvaluator::setBias( f32 bias )
    {
        m_bias = bias;
    }

    void PromptEvaluator::setLabel( const String &label )
    {
        m_label = label;
    }

    String PromptEvaluator::getLabel() const
    {
        return m_label;
    }

    void PromptEvaluator::setTags( const Array<String> &tags )
    {
        m_tags = tags;
    }

    Array<String> PromptEvaluator::getTags() const
    {
        return m_tags;
    }

    void PromptEvaluator::setNamedEntities( const Array<String> &namedEntities )
    {
        m_namedEntities = namedEntities;
    }

    Array<String> PromptEvaluator::getNamedEntities() const
    {
        return m_namedEntities;
    }
}  // namespace workphone::editor
