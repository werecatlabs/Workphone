#include <EditorPCH.hpp>
#include <editor/McpAuthoring.hpp>

#include <commands/AddActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Script/ProjectScriptPolicy.hpp>
#include <boost/json.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>

namespace workphone::editor
{
    namespace
    {
        namespace fs = std::filesystem;
        namespace json = boost::json;
        using Actor = SmartPtr<scene::IGameActor>;
        using Scene = SmartPtr<scene::IGameScene>;
        using Vector = Vector3<real_Num>;

        std::string stringArg( const json::object &args, const char *name )
        {
            const auto value = args.if_contains( name );
            if( !value || !value->is_string() || value->as_string().empty() ||
                value->as_string().find( '\0' ) != json::string::npos )
            {
                throw std::runtime_error( std::string( name ) + " must be a nonempty string" );
            }
            return std::string( value->as_string() );
        }

        fs::path contained( const fs::path &root, const fs::path &candidate )
        {
            const auto base = fs::canonical( root );
            const auto path = fs::weakly_canonical( candidate );
            const auto relative = path.lexically_relative( base );
            if( relative.empty() || relative.is_absolute() || *relative.begin() == ".." )
            {
                throw std::runtime_error( "Path is outside the selected workspace or project" );
            }
            return path;
        }

        fs::path relativeInput( const fs::path &base, const std::string &text )
        {
            const auto input = fs::u8path( text );
            if( input.has_root_path() || text.find( ':' ) != std::string::npos )
            {
                throw std::runtime_error( "Expected a relative path" );
            }
            return contained( base, base / input );
        }

        bool isScenePath( const fs::path &p )
        {
            return p.extension() == ".fbscene" || p.extension() == ".fbscenebin" ||
                   p.extension() == ".fbscenexml";
        }

        std::string uuid( const SmartPtr<ISharedObject> &object )
        {
            return object->getHandle()->getUUIDAsString().c_str();
        }

        json::array vectorJson( const Vector &v )
        {
            return { v[0], v[1], v[2] };
        }

        std::optional<Vector> vectorArg( const json::object &args, const char *key )
        {
            const auto value = args.if_contains( key );
            if( !value )
            {
                return std::nullopt;
            }
            if( !value->is_array() || value->as_array().size() != 3 )
            {
                throw std::runtime_error( std::string( key ) + " must contain three numbers" );
            }
            Vector result;
            for( int i = 0; i < 3; ++i )
            {
                const auto &element = value->as_array()[i];
                if( !element.is_number() || !std::isfinite( element.to_number<double>() ) ||
                    std::abs( element.to_number<double>() ) > std::numeric_limits<real_Num>::max() )
                {
                    throw std::runtime_error( std::string( key ) + " contains an invalid number" );
                }
                result[i] = static_cast<real_Num>( element.to_number<double>() );
            }
            return result;
        }

        struct ActorEdit
        {
            explicit ActorEdit( const json::object &args, bool creating ) :
                position( vectorArg( args, "position" ) ),
                rotation( vectorArg( args, "rotation" ) ),
                scale( vectorArg( args, "scale" ) )
            {
                if( creating || args.contains( "name" ) )
                {
                    name = stringArg( args, "name" );
                }
                if( const auto value = args.if_contains( "enabled" ) )
                {
                    enabled = value->as_bool();
                }
                if( !name && !position && !rotation && !scale && !enabled )
                {
                    throw std::runtime_error( "Specify an actor change" );
                }
            }

            void apply( Actor actor ) const
            {
                if( name )
                {
                    actor->setName( name->c_str() );
                }
                if( position )
                {
                    actor->setLocalPosition( *position );
                }
                if( rotation )
                {
                    actor->setLocalRotation( *rotation );
                }
                if( scale )
                {
                    actor->setLocalScale( *scale );
                }
                if( enabled )
                {
                    actor->setEnabled( *enabled );
                }
                actor->updateTransform();
            }

            std::optional<std::string> name;
            std::optional<Vector> position, rotation, scale;
            std::optional<bool> enabled;
        };

        template <typename Visitor>
        void visitActors( const Scene &scene, Visitor visit )
        {
            auto pending = scene->getActors();
            std::set<scene::IGameActor *> visited;
            while( !pending.empty() )
            {
                auto actor = pending.back();
                pending.pop_back();
                if( !actor || !visited.insert( actor.get() ).second )
                {
                    continue;
                }
                if( !visit( actor ) )
                {
                    return;
                }
                const auto children = actor->getChildren();
                pending.insert( pending.end(), children.begin(), children.end() );
            }
        }

        Actor findActor( const Scene &scene, const std::string &id )
        {
            Actor result;
            visitActors( scene, [&]( const Actor &actor ) {
                if( uuid( actor ) == id )
                {
                    result = actor;
                }
                return !result;
            } );
            if( !result )
            {
                throw std::runtime_error( "No actor with that UUID in the current scene" );
            }
            return result;
        }

        json::object describeComponent( const SmartPtr<scene::IComponent> &component, bool inspect )
        {
            json::object result{ { "id", uuid( component ) },
                                 { "type",
                                   TypeManager::instance()->getName( component->getTypeInfo() ) } };
            if( auto script = workphone::dynamic_pointer_cast<scene::Script>( component ) )
            {
                result["className"] = script->getClassName().c_str();
            }
            if( inspect )
            {
                json::array fields;
                if( auto properties = component->getProperties() )
                {
                    for( const auto &p : properties->getPropertiesAsArray() )
                    {
                        fields.emplace_back( json::object{ { "name", p.getName().c_str() },
                                                           { "type", p.getTypeName().c_str() },
                                                           { "value", p.getValue().c_str() },
                                                           { "readOnly", p.isReadOnly() } } );
                    }
                }
                result["properties"] = std::move( fields );
            }
            return result;
        }

        json::object describeActor( const Actor &actor )
        {
            json::array components;
            for( const auto &component : actor->getComponents() )
            {
                if( component )
                {
                    components.emplace_back( describeComponent( component, false ) );
                }
            }
            const auto parent = actor->getParent();
            return { { "id", uuid( actor ) },
                     { "name", actor->getName().c_str() },
                     { "parent", parent ? json::value( uuid( parent ) ) : json::value( nullptr ) },
                     { "position", vectorJson( actor->getLocalPosition() ) },
                     { "rotation", vectorJson( actor->getLocalRotation() ) },
                     { "scale", vectorJson( actor->getLocalScale() ) },
                     { "enabled", actor->isEnabled() },
                     { "components", std::move( components ) } };
        }

        void loadScene( const Scene &scene, const fs::path &path )
        {
            if( !isScenePath( path ) || !fs::is_regular_file( path ) )
            {
                throw std::runtime_error( "Expected an existing Workphone scene file" );
            }
            core::IApplicationManager::instance()->getGameManager()->loadScene( path.u8string().c_str(),
                                                                                false );
            if( scene->getSceneLoadingState() != scene::IGameScene::SceneLoadingState::Loaded )
            {
                throw std::runtime_error( "The engine failed to load the requested scene" );
            }
        }

        void saveScene( Scene scene, const fs::path &path )
        {
            scene->saveScene( path.u8string().c_str() );
            if( !fs::is_regular_file( path ) || fs::file_size( path ) == 0 ||
                scene->getFilePath().empty() )
            {
                throw std::runtime_error( "The engine did not publish the scene file" );
            }
        }

        void newScene( Scene scene, const fs::path &path )
        {
            if( !isScenePath( path ) || fs::exists( path ) )
            {
                throw std::runtime_error( "The new scene requires an unused Workphone scene path" );
            }
            scene->clear( true );
            scene->setFilePath( String() );
            scene->setLabel( path.stem().u8string().c_str() );
            saveScene( scene, path );
            scene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loaded );
        }

        fs::path scenePath( const fs::path &base, const String &stored )
        {
            const auto p = fs::u8path( stored.c_str() );
            const auto path = contained( base, p.is_absolute() ? p : base / p );
            if( !isScenePath( path ) )
            {
                throw std::runtime_error( "Invalid scene file extension" );
            }
            return path;
        }

        void refresh( bool saved )
        {
            auto editor = EditorManager::getSingletonPtr();
            editor->setFileSaved( saved );
            if( auto ui = editor->getUI() )
            {
                ui->rebuildSceneTree();
            }
        }

        void switchScene( Scene scene, const fs::path &path, bool create )
        {
            if( !isScenePath( path ) || ( create ? fs::exists( path ) : !fs::is_regular_file( path ) ) )
            {
                throw std::runtime_error( create ? "Expected an unused Workphone scene file path"
                                                 : "Expected an existing Workphone scene file" );
            }
            // Both operations clear the graph. Restore unsaved edits if publication
            // or deserialization fails, as when leaving Editor play mode.
            const auto original = scene->toData();
            const auto file = scene->getFilePath();
            const auto label = scene->getLabel();
            const auto state = scene->getSceneLoadingState();
            const bool saved = EditorManager::getSingletonPtr()->getFileSaved();
            try
            {
                if( create )
                {
                    newScene( scene, path );
                }
                else
                {
                    loadScene( scene, path );
                }
            }
            catch( ... )
            {
                scene->clear( true );
                scene->fromData( original );
                scene->setLabel( label );
                scene->setFilePath( file );
                scene->setSceneLoadingState( state );
                refresh( saved );
                throw;
            }
        }

        json::object projectInfo( const SmartPtr<Project> &project, const fs::path &workspace )
        {
            const auto file = contained( workspace, fs::u8path( project->getFilePath().c_str() ) );
            return { { "project", fs::relative( file, workspace ).generic_u8string() },
                     { "scene", project->getCurrentScenePath().c_str() } };
        }

        void setStartupScene( SmartPtr<Project> project, const fs::path &relative )
        {
            const String path = relative.generic_u8string().c_str();
            project->setCurrentScenePath( path );
            auto settings = project->getProperties();
            settings->setProperty( "project.startupScene", path );
            project->setProperties( settings );
        }

        void checkProjectStorage( const fs::path &directory )
        {
            for( const char *name : { "Cache", "SettingsCache", "ProjectRuntime.settings",
                                      "GameGraphics.settings", ".workphone" } )
            {
                contained( directory, directory / name );
            }
        }

        void openProject( const fs::path &file, const fs::path &workspace, Scene scene,
                          SmartPtr<Project> project )
        {
            if( ( file.extension() != ".fbproject" && file.extension() != ".fbp" ) ||
                !fs::is_regular_file( file ) )
            {
                throw std::runtime_error( "Expected an existing .fbproject or .fbp project file" );
            }
            auto properties = workphone::make_ptr<Properties>();
            DataUtil::parse( Path::readAllText( file.u8string().c_str() ), properties.get() );
            // Normal project loading mounts search folders and executes its source
            // list. Resolve the same legacy/current metadata before invoking it.
            Project layout;
            layout.fromData( properties );
            checkProjectStorage( file.parent_path() );
            for( const auto &script : layout.getScriptFilePaths() )
            {
                contained( workspace, file.parent_path() / fs::u8path( script.c_str() ) );
            }
            for( const auto &folders :
                 { layout.getPaths(), layout.getResourceFolders(), layout.getMediaPaths() } )
            {
                for( const auto &folder : folders )
                {
                    const auto path = file.parent_path() / fs::u8path( folder.c_str() );
                    if( fs::is_directory( path ) )
                    {
                        contained( workspace, path );
                    }
                }
            }
            String startup;
            if( properties->getPropertyAsBool( "project.autoLoadLastScene", true ) )
            {
                properties->getPropertyValue( "currentScenePath", startup );
            }
            if( startup.empty() )
            {
                properties->getPropertyValue( "project.startupScene", startup );
            }
            fs::path selected;
            if( !startup.empty() )
            {
                selected = scenePath( file.parent_path(), startup );
                if( !fs::is_regular_file( selected ) )
                {
                    throw std::runtime_error( "Startup scene is missing" );
                }
            }
            auto editor = EditorManager::getSingletonPtr();
            editor->loadProject( file.u8string().c_str() );
            if( selected.empty() )
            {
                scene->clear( true );
                scene->setLabel( "Untitled" );
                scene->setFilePath( String() );
                scene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::None );
                project->setCurrentScenePath( String() );
            }
            else
            {
                // Normal project open queues a scene job. Supersede it with a synchronous
                // load so that the next MCP request sees this scene and its actual UUIDs.
                loadScene( scene, selected );
                project->setCurrentScenePath(
                    fs::relative( selected, file.parent_path() ).generic_u8string().c_str() );
            }
            refresh( true );
        }

        AddActorCmd::ActorType actorKind( const json::object &args )
        {
            using Kind = AddActorCmd::ActorType;
            const auto kind = args.contains( "kind" ) ? stringArg( args, "kind" ) : "empty";
            if( kind == "empty" )
            {
                return Kind::EmptyActor;
            }
            if( kind == "cube" )
            {
                return Kind::CubeMesh;
            }
            if( kind == "plane" )
            {
                return Kind::PlaneMesh;
            }
            if( kind == "physics_cube" )
            {
                return Kind::Cube;
            }
            if( kind == "camera" )
            {
                return Kind::Camera;
            }
            if( kind == "directional_light" )
            {
                return Kind::DirectionalLight;
            }
            if( kind == "point_light" )
            {
                return Kind::PointLight;
            }
            throw std::runtime_error( "Unrecognized actor kind" );
        }
    }  // namespace

    json::object mcpAuthor( const std::string &name, const json::object &args,
                            const fs::path &workspace )
    {
        auto app = core::IApplicationManager::instancePtr();
        auto editor = EditorManager::getSingletonPtr();
        auto game = app ? app->getGameManager() : nullptr;
        auto scene = game ? game->getCurrentScene() : nullptr;
        auto project = editor ? editor->getProject() : nullptr;
        if( !scene || !project )
        {
            throw std::runtime_error( "The Editor has no active scene or project service" );
        }
        if( name == "scene_inspect" )
        {
            json::array actors;
            visitActors( scene, [&]( const Actor &actor ) {
                actors.emplace_back( describeActor( actor ) );
                return true;
            } );
            return { { "actors", std::move( actors ) },
                     { "playing", app->isPlaying() },
                     { "name", scene->getLabel().c_str() } };
        }
        auto factories = app->getFactoryManager();
        if( name == "component_types" )
        {
            std::set<std::string> names;
            for( auto factory : factories->getFactories() )
            {
                if( factory && factory->isObjectDerivedFrom<scene::IComponent>() )
                {
                    names.emplace( factory->getObjectTypeName().c_str() );
                }
            }
            return { { "types", json::value_from( names ) } };
        }
        if( name != "component_inspect" && ( app->isPlaying() || editor->isEditingPrefab() ) )
        {
            throw std::runtime_error( "Stop play and return to scene editing before authoring" );
        }
        if( name == "project_open" || name == "project_create" )
        {
            auto path = relativeInput( workspace, stringArg( args, "path" ) );
            if( name == "project_create" )
            {
                if( fs::exists( path ) && ( !fs::is_directory( path ) || !fs::is_empty( path ) ) )
                {
                    throw std::runtime_error( "Create the project in a new or empty directory" );
                }
                project->create( path.u8string().c_str() );
                project->setLabel( path.filename().u8string().c_str() );
                project->setMediaPaths( { "Assets" } );
                project->save();
                const auto descriptor = path / "project.fbproject";
                openProject( descriptor, workspace, scene, project );
                newScene( scene, path / "Assets/Main.fbscene" );
                setStartupScene( project, "Assets/Main.fbscene" );
                project->save();
                refresh( true );
            }
            else
            {
                openProject( path, workspace, scene, project );
            }
            return projectInfo( project, workspace );
        }
        if( project->getFilePath().empty() )
        {
            throw std::runtime_error( "Open or create a workspace project first" );
        }
        const auto descriptor = contained( workspace, fs::u8path( project->getFilePath().c_str() ) );
        const auto base = descriptor.parent_path();
        if( name == "scene_open" || name == "scene_create" || name == "project_save" )
        {
            checkProjectStorage( base );
            fs::path path;
            if( name == "project_save" )
            {
                if( scene->getFilePath().empty() )
                {
                    throw std::runtime_error( "Create or open a scene before saving the project" );
                }
                path = scenePath( base, scene->getFilePath() );
                saveScene( scene, path );
            }
            else
            {
                path = relativeInput( base, stringArg( args, "path" ) );
                switchScene( scene, path, name == "scene_create" );
            }
            const auto relative = fs::relative( path, base );
            if( name == "project_save" )
            {
                project->setCurrentScenePath( relative.generic_u8string().c_str() );
            }
            else
            {
                setStartupScene( project, relative );
            }
            project->save();
            refresh( true );
            return projectInfo( project, workspace );
        }
        if( name == "actor_create" )
        {
            ActorEdit edit( args, true );
            auto command = workphone::make_ptr<AddActorCmd>();
            command->setExpectedScene( scene );
            command->setUseSelectionAsParent( false );
            command->setActorType( actorKind( args ) );
            if( args.contains( "parent" ) )
            {
                command->setParent( findActor( scene, stringArg( args, "parent" ) ) );
            }
            command->execute();
            const auto actor = command->getActor();
            if( !actor )
            {
                throw std::runtime_error( "Actor creation failed" );
            }
            edit.apply( actor );
            refresh( false );
            return describeActor( actor );
        }
        auto actor = findActor( scene, stringArg( args, "id" ) );
        if( name == "actor_edit" )
        {
            ActorEdit( args, false ).apply( actor );
            refresh( false );
            return describeActor( actor );
        }
        if( name == "actor_remove" )
        {
            auto id = uuid( actor );
            if( auto selection = app->getSelectionManager() )
            {
                selection->clearSelection();
            }
            game->destroyActor( actor );
            refresh( false );
            return { { "removed", id } };
        }
        if( name == "component_add" || name == "script_attach" )
        {
            SmartPtr<scene::IComponent> component;
            auto scripts = project->getScriptFilePaths();
            if( name == "script_attach" )
            {
                const auto path = relativeInput( base, stringArg( args, "path" ) );
                const String className = stringArg( args, "className" ).c_str();
                if( path.extension() != ".lua" || !fs::is_regular_file( path ) )
                {
                    throw std::runtime_error( "Expected a project Lua script file" );
                }
                auto manager = getProjectScriptManager();
                if( !manager || !manager->isLoaded() )
                {
                    throw std::runtime_error( "The project scripting backend is unavailable" );
                }
                const String relative = fs::relative( path, base ).generic_u8string().c_str();
                const bool listed =
                    std::find( scripts.begin(), scripts.end(), relative ) != scripts.end();
                const auto existing = manager->getRuntimeClassNames();
                if( !listed &&
                    std::find( existing.begin(), existing.end(), className ) != existing.end() )
                {
                    throw std::runtime_error(
                        "That Lua class is already registered. Use a distinct gameplay class name." );
                }
                manager->loadScripts( { path.u8string().c_str() } );
                const auto classes = manager->getRuntimeClassNames();
                if( std::find( classes.begin(), classes.end(), className ) == classes.end() )
                {
                    throw std::runtime_error(
                        "The project script did not register the requested Lua class" );
                }
                auto script = workphone::make_ptr<scene::Script>();
                script->setClassName( className );
                script->setUpdateInPlayMode( true );
                component = script;
                if( !listed )
                {
                    scripts.push_back( relative );
                }
            }
            else
            {
                const auto type = stringArg( args, "type" );
                auto factory = factories->getFactoryByName( type.c_str() );
                if( !factory || !factory->isObjectDerivedFrom<scene::IComponent>() )
                {
                    throw std::runtime_error( "Unknown component type. See component_types." );
                }
                component = factory->make_ptr<scene::IComponent>();
            }
            if( !component )
            {
                throw std::runtime_error( "Component creation failed" );
            }
            // Match IGameActor::addComponent. Physics, rendering and gameplay
            // components may consult their owning actor during load.
            actor->addComponentInstance( component );
            try
            {
                game->loadObject( component );
                if( !component->isLoaded() )
                {
                    throw std::runtime_error( "Component initialization failed" );
                }
                component->setState( scene::IComponent::State::Edit );
                if( name == "script_attach" )
                {
                    if( !component->getScriptData() )
                    {
                        throw std::runtime_error(
                            "The gameplay script constructor failed or project scripting is disabled" );
                    }
                    project->setScriptFilePaths( scripts );
                }
            }
            catch( ... )
            {
                actor->removeComponentInstance( component );
                throw;
            }
            refresh( false );
            return describeComponent( component, false );
        }
        const auto id = stringArg( args, "component" );
        for( auto component : actor->getComponents() )
        {
            if( !component || uuid( component ) != id )
            {
                continue;
            }
            if( name == "component_inspect" )
            {
                return describeComponent( component, true );
            }
            if( name == "component_remove" )
            {
                actor->removeComponentInstance( component );
                refresh( false );
                return { { "removed", id } };
            }
            if( name == "component_edit" )
            {
                const auto values = args.if_contains( "properties" );
                if( !values || !values->is_object() || values->as_object().empty() )
                {
                    throw std::runtime_error( "Specify one or more component property values" );
                }
                auto properties = component->getProperties();
                for( const auto &item : values->as_object() )
                {
                    const String key = std::string( item.key() ).c_str();
                    if( !properties || !properties->hasProperty( key ) ||
                        properties->getPropertyObject( key ).isReadOnly() || !item.value().is_string() )
                    {
                        throw std::runtime_error( "Expected an editable property and a string value: " +
                                                  std::string( item.key() ) );
                    }
                    properties->getPropertyObject( key ).setValue( item.value().as_string().c_str() );
                }
                component->setProperties( properties );
                refresh( false );
                return describeComponent( component, true );
            }
            throw std::runtime_error( "Unknown Editor authoring operation" );
        }
        throw std::runtime_error( "The actor has no component with that UUID" );
    }
}  // namespace workphone::editor
