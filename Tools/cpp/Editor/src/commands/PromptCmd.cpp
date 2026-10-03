#include <EditorPCH.hpp>
#include <commands/PromptCmd.hpp>
#include <Workphone/Workphone.hpp>
#include <ai/PromptEvaluator.hpp>
#include <ai/AddPromptEvaluator.hpp>
#include <ai/RemovePromptEvaluator.hpp>
#include "AddActorCmd.hpp"
#include "RemoveSelectionCmd.hpp"

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, PromptCmd, Command );

    PromptCmd::PromptCmd()
    {
        goalEvaluators.reserve( 32 );

        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::Actor, "actor",
            Array<String>( { "make", "actor", "gameobject", "game", "object" } ) ) );
        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::DirectionalLight, "light", Array<String>( { "make", "light" } ) ) );
        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::PointLight, "light",
            Array<String>( { "create", "make", "point", "light" } ) ) );
        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::Cube, "cube", Array<String>( { "create", "make", "cube" } ) ) );
        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::Cube, "sphere", Array<String>( { "create", "make", "sphere" } ) ) );
        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::Terrain, "terrain",
            Array<String>( { "create", "make", "terrain" } ) ) );

        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::Vehicle, "terrain", Array<String>( { "create", "make", "car" } ) ) );

        goalEvaluators.emplace_back( workphone::make_ptr<AddPromptEvaluator>(
            AddActorCmd::ActorType::Skybox, "skybox",
            Array<String>( { "create", "make", "skybox", "sky", "skydome" } ) ) );

        goalEvaluators.emplace_back( workphone::make_ptr<RemovePromptEvaluator>(
            "remove", Array<String>( { "destroy", "remove", "delete" } ) ) );
    }

    PromptCmd::~PromptCmd() = default;

    void PromptCmd::undo()
    {
    }

    void PromptCmd::redo()
    {
    }

    void PromptCmd::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto aiManager = applicationManager->getAiManager();

        auto prompt = getPrompt();
        if( !StringUtil::isNullOrEmpty( prompt ) )
        {
            auto response = aiManager->query( prompt );
            processAIPrompt( response );
        }
    }

    void PromptCmd::processAIPrompt( const String &userInput )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto taskManager = applicationManager->getTaskManager();
        auto renderLock = taskManager->lockTask( TaskId::Render );
        auto physicsLock = taskManager->lockTask( TaskId::Physics );

        String response_text;
        response_text.reserve( 1024 );

        auto jsonList = StringUtil::split( userInput, "{", "}" );
        for( auto &json : jsonList )
        {
            auto jsonDataStr = StringUtil::replace( json, '\\', ' ' );
            jsonDataStr = "{\n" + jsonDataStr + "\n}";

            if( DataUtil::isValidData( jsonDataStr ) )
            {
                auto properties = workphone::make_ptr<Properties>();
                DataUtil::parse( jsonDataStr, properties.get() );

                auto propertyList = properties->getPropertiesAsArray();
                for( auto &property : propertyList )
                {
                    auto name = property.getName();
                    auto value = property.getValue();

                    if( StringUtil::contains( name, "response" ) )
                    {
                        response_text += value;
                    }
                }
            }
        }

        // User input
        // Tokenization
        auto tokens = StringUtil::tokenize( response_text );

        // Dependency parsing (not implemented in this example)

        // Named Entity Recognition (NER)
        Array<String> namedEntities;  // = StringUtil::extractNamedEntities( tokens );
        namedEntities = tokens;

        SmartPtr<PromptEvaluator> bestMatch;

        // Extract goal
        String goal;
        for( const auto &entity : namedEntities )
        {
            // Check if the entity matches any keyword for goals
            if( entity == "home" )
            {
                goal = "drive to " + entity;
                break;
            }

            auto highestRating = 0.0f;

            for( auto goalEvaluator : goalEvaluators )
            {
                auto promptGoalEvaluator = dynamic_cast<PromptEvaluator *>( goalEvaluator.get() );
                promptGoalEvaluator->setNamedEntities( namedEntities );

                auto rating = goalEvaluator->getRating();
                if( rating > highestRating )
                {
                    highestRating = rating;
                    bestMatch = goalEvaluator;
                }
            }
        }

        if( bestMatch )
        {
            std::cout << "Extracted Goal: " << bestMatch->getLabel() << std::endl;
            bestMatch->activateGoal();
        }
        else
        {
            std::cout << "No goal extracted." << std::endl;
        }
    }

    String PromptCmd::getPrompt() const
    {
        return m_prompt;
    }

    void PromptCmd::setPrompt( const String &prompt )
    {
        m_prompt = prompt;
    }
}  // namespace workphone::editor
