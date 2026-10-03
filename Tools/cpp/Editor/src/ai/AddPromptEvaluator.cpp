#include <EditorPCH.hpp>
#include <ai/AddPromptEvaluator.hpp>
#include <Workphone/Workphone.hpp>
#include "commands/AddActorCmd.hpp"
#include "commands/RemoveSelectionCmd.hpp"

namespace workphone::editor
{
    AddPromptEvaluator::AddPromptEvaluator() = default;

    AddPromptEvaluator::AddPromptEvaluator( AddActorCmd::ActorType actorType, const String &label,
                                            const Array<String> &tags ) :
        PromptEvaluator( label, tags ),
        m_actorType( actorType )
    {
    }

    AddPromptEvaluator::~AddPromptEvaluator() = default;

    void AddPromptEvaluator::activateGoal()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto commandManager = applicationManager->getCommandManager();

        auto cmd = workphone::make_ptr<AddActorCmd>();
        cmd->setActorType( m_actorType );
        commandManager->addCommand( cmd );
    }

    f32 AddPromptEvaluator::getRating()
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

    SmartPtr<ISharedObject> AddPromptEvaluator::getOwner() const
    {
        return m_owner;
    }

    void AddPromptEvaluator::setOwner( SmartPtr<ISharedObject> owner )
    {
        m_owner = owner;
    }

    f32 AddPromptEvaluator::getBias() const
    {
        return m_bias;
    }

    void AddPromptEvaluator::setBias( f32 bias )
    {
        m_bias = bias;
    }

    void AddPromptEvaluator::setActorType( AddActorCmd::ActorType actorType )
    {
        m_actorType = actorType;
    }

    AddActorCmd::ActorType AddPromptEvaluator::getActorType() const
    {
        return m_actorType;
    }
}  // namespace workphone::editor
