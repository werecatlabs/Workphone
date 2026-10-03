#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Script/ScriptInvoker.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Interface/Script/IScriptData.hpp>
#include <initializer_list>
#include <limits>

#if WP_ENABLE_LUA
#    include <WPLuabind/WPLuabind.hpp>
#    include <WPLuabind/ParamConverter.hpp>
#    include <boost/ref.hpp>
#    include <luabind/luabind.hpp>

extern "C" {
#    include <lauxlib.h>
#    include <lualib.h>
}
#endif

using namespace workphone;

namespace
{
#if WP_ENABLE_LUA
    class LuaStateGuard
    {
    public:
        LuaStateGuard() : m_state( luaL_newstate() )
        {
            if( m_state )
            {
                luaL_openlibs( m_state );
                luabind::open( m_state );
            }
        }

        ~LuaStateGuard()
        {
            if( m_state )
            {
                lua_close( m_state );
            }
        }

        lua_State *get() const
        {
            return m_state;
        }

    private:
        lua_State *m_state = nullptr;
    };

    String roundTripWorkphoneString( const String &value )
    {
        return value;
    }

    WeakPtr<ISharedObject> roundTripWeakPointer( const WeakPtr<ISharedObject> &value )
    {
        return value;
    }

    Parameter roundTripParameter( const Parameter &value )
    {
        return value;
    }

    std::filesystem::path getCatchGameCorePath()
    {
        auto sourcePath = std::filesystem::absolute( std::filesystem::path( __FILE__ ) );
        auto repositoryPath = sourcePath.parent_path().parent_path().parent_path().parent_path();
        return repositoryPath / "Bin" / "Media" / "Scripts" / "Lua" / "Game" / "CatchGame" /
               "CatchGameCore.lua";
    }

    std::filesystem::path getCatchGameComponentPath()
    {
        return getCatchGameCorePath().parent_path() / "CatchGame.lua";
    }

    std::filesystem::path getCatchGameProjectBootstrapPath()
    {
        auto sourcePath = std::filesystem::absolute( std::filesystem::path( __FILE__ ) );
        auto repositoryPath = sourcePath.parent_path().parent_path().parent_path().parent_path();
        return repositoryPath / "Bin" / "Media" / "Projects" / "CatchGame" / "Scripts" /
               "CatchGameProject.lua";
    }

    void requireLuaFile( lua_State *state, const std::filesystem::path &path )
    {
        const auto pathString = path.string();
        const auto result = luaL_dofile( state, pathString.c_str() );
        if( result != LUA_OK )
        {
            const auto error = lua_tostring( state, -1 );
            BOOST_FAIL( "Failed to load " << pathString << ": "
                                          << ( error ? error : "unknown Lua error" ) );
            lua_pop( state, 1 );
        }
    }

    lua_Integer getParameterArraySize( const Parameter &value )
    {
        return value.type == ParameterType::PARAM_TYPE_ARRAY
                   ? static_cast<lua_Integer>( value.array.size() )
                   : static_cast<lua_Integer>( -1 );
    }

    class ComponentScriptData : public IScriptData
    {
    public:
        SmartPtr<ISharedObject> getOwner() const override { return nullptr; }
        void setOwner( SmartPtr<ISharedObject> ) override {}
        void *getObjectData() const override { return nullptr; }
    };

    // A deterministic manager double: exercise Script and ScriptInvoker while calling
    // the real Lua callbacks with the production Parameters converter.
    class ComponentScriptManager : public IScriptManager
    {
    public:
        LuaStateGuard lua;
        luabind::object instance;
        SmartPtr<IScriptData> data;
        Array<String> createdClasses;
        size_t destroyedObjects = 0;
        size_t propertyCalls = 0;

        ComponentScriptManager()
        {
            setLoadingState( LoadingState::Loaded );
            bindBaseObjects( lua.get() );
            bindCore( lua.get() );
            bindParam( lua.get() );
            bindParamList( lua.get() );
            loadScriptFromString( R"(
                class 'BaseComponent'
                function BaseComponent:__init(component) end
                IApplicationManager = { instance = function()
                    return { getFileSystem = function() return {} end,
                             getFactoryManager = function() return {} end }
                end }
            )" );
            auto path = getCatchGameCorePath().parent_path().parent_path() /
                        "Core" / "Application.lua";
            requireLuaFile( lua.get(), path );
            loadScriptFromString( R"(
                function Application:generateNew()
                    self.generated = (self.generated or 0) + 1
                end
                class 'ScriptReplacement' (Application)
                function ScriptReplacement:__init(component)
                    Application.__init(self, component)
                end
            )" );
        }

        SmartPtr<IScriptClass> createObject( const String &name,
                                              SmartPtr<ISharedObject> object ) override
        {
            createdClasses.push_back( name );
            instance = luabind::call_function<luabind::object>( lua.get(), name.c_str() );
            data = make_ptr<ComponentScriptData>();
            object->setScriptData( data );
            return nullptr;
        }
        void destroyObject( SmartPtr<ISharedObject> object ) override
        {
            if( object->getScriptData() )
            {
                ++destroyedObjects;
                object->setScriptData( nullptr );
                data = nullptr;
                instance = luabind::object();
            }
        }
        void callObjectMember( SmartPtr<ISharedObject>, const String &name,
                               const Parameters &parameters ) override
        {
            ++propertyCalls;
            BOOST_REQUIRE_EQUAL( parameters.size(), 1 );
            BOOST_CHECK( parameters[0].type == ParameterType::PARAM_TYPE_OBJECT );
            luabind::call_member<void>( instance, name.c_str(), parameters );
        }
        void loadScriptFromString( const String &source ) override
        {
            auto status = luaL_dostring( lua.get(), source.c_str() );
            BOOST_REQUIRE_MESSAGE( status == LUA_OK,
                                   ( status == LUA_OK ? "" : lua_tostring( lua.get(), -1 ) ) );
        }
        void loadScript( const String & ) override {}
        void loadScripts( const Array<String> & ) override {}
        void callFunction( const String & ) override {}
        void callFunction( const String &, const Parameters & ) override {}
        void callFunction( const String &, const Parameters &, Parameters & ) override {}
        s32 callMember( const String &, const String & ) override { return 0; }
        s32 callMember( const String &, const String &, const Parameters & ) override { return 0; }
        s32 callMember( const String &, const String &, const Parameters &, Parameters & ) override { return 0; }
        void callObjectMember( SmartPtr<ISharedObject>, const String & ) override {}
        void callObjectMember( SmartPtr<ISharedObject>, const String &, const Parameters &, Parameters & ) override {}
        void *createInstance( const String & ) override { return nullptr; }
        void destroyInstance( void * ) override {}
        void reloadScripts() override {}
        bool reloadPending() const override { return false; }
        String getDebugInfo() override { return {}; }
        bool getDelayedCreation() const override { return false; }
        void setDelayedCreation( bool ) override {}
        bool isDebugEnabled() const override { return false; }
        void setDebugEnabled( bool ) override {}
        void removeBreakpoint( SmartPtr<IScriptBreakpoint> ) override {}
        void addBreakpoint( SmartPtr<IScriptBreakpoint> ) override {}
        Array<SmartPtr<IScriptBreakpoint>> getBreakpoints() const override { return {}; }
        void garbageCollect() override {}
        void _getObject( void **object ) override { *object = nullptr; }
        void registerClass( void * ) override {}
        Array<String> getClassNames() const override { return {}; }
        void setClassNames( const Array<String> & ) override {}
        Array<String> getSupportedFileExtensions() const override { return {}; }
        void loadObject( SmartPtr<ISharedObject>, bool ) override {}
        void unloadObject( SmartPtr<ISharedObject>, bool ) override {}
    };

    struct ComponentScriptFixture
    {
        TestGuard guard;
        SmartPtr<IScriptManager> previousManager;
        SmartPtr<ComponentScriptManager> manager;
        SmartPtr<scene::IGameActor> actor;
        SmartPtr<scene::Script> component;

        ComponentScriptFixture()
        {
            auto application = core::IApplicationManager::instance();
            previousManager = application->getScriptManager();
            manager = make_ptr<ComponentScriptManager>();
            application->setScriptManager( manager );
            // Keep the component detached from asynchronous scene/FSM updates.
            actor = make_ptr<scene::GameActor>();
            component = make_ptr<scene::Script>();
            component->setActor( actor );
            component->setInvoker( make_ptr<ScriptInvoker>( component ) );
            BOOST_REQUIRE( component->getInvoker() );
        }
        ~ComponentScriptFixture()
        {
            auto application = core::IApplicationManager::instance();
            manager->destroyObject( component );
            component->setInvoker( nullptr );
            component->setActor( nullptr );
            component = nullptr;
            actor = nullptr;
            application->setScriptManager( previousManager );
        }
        void selectClass( const String &name )
        {
            auto properties = make_ptr<Properties>();
            properties->setProperty( "className", name );
            component->setProperties( properties );
        }
    };
#endif

    bool runScriptGeneratorIntegrationTests()
    {
        return std::getenv( "WP_RUN_INTEGRATION_TESTS" ) != nullptr;
    }
}  // namespace

#if WP_ENABLE_LUA
BOOST_FIXTURE_TEST_CASE( script_component_application_properties, ComponentScriptFixture )
{
    selectClass( "Application" );
    BOOST_REQUIRE( component->getScriptData() );
    auto properties = component->getProperties();
    BOOST_REQUIRE( properties );
    BOOST_REQUIRE( properties->hasProperty( "Generate" ) );
    BOOST_CHECK( !properties->isButtonPressed( "Generate" ) );
    properties->setButtonPressed( "Generate", true );
    component->setProperties( properties );
    luabind::object generated = manager->instance["generated"];
    BOOST_REQUIRE_EQUAL( luabind::type( generated ), LUA_TNUMBER );
    BOOST_CHECK_EQUAL( luabind::object_cast<int>( generated ), 1 );
    BOOST_CHECK_EQUAL( manager->createdClasses.size(), 1 );
}

BOOST_FIXTURE_TEST_CASE( script_component_class_replacement_and_clear, ComponentScriptFixture )
{
    selectClass( "Application" );
    auto previousData = component->getScriptData();
    selectClass( "ScriptReplacement" );
    BOOST_CHECK_EQUAL( manager->createdClasses.size(), 2 );
    BOOST_CHECK_EQUAL( manager->destroyedObjects, 1 );
    BOOST_CHECK( component->getScriptData() != previousData );
    auto calls = manager->propertyCalls;
    selectClass( "" );
    BOOST_CHECK( !component->getScriptData() );
    BOOST_CHECK_EQUAL( manager->destroyedObjects, 2 );
    BOOST_CHECK_EQUAL( manager->propertyCalls, calls );
}

BOOST_FIXTURE_TEST_CASE( script_component_partial_properties_preserve_class, ComponentScriptFixture )
{
    selectClass( "Application" );
    auto data = component->getScriptData();
    auto properties = make_ptr<Properties>();
    properties->setButtonPressed( "Generate", true );
    component->setProperties( properties );
    BOOST_CHECK_EQUAL( component->getClassName(), "Application" );
    BOOST_CHECK( component->getScriptData() == data );
    luabind::object generated = manager->instance["generated"];
    BOOST_REQUIRE_EQUAL( luabind::type( generated ), LUA_TNUMBER );
    BOOST_CHECK_EQUAL( luabind::object_cast<int>( generated ), 1 );
    BOOST_CHECK_EQUAL( manager->createdClasses.size(), 1 );
}

BOOST_AUTO_TEST_CASE( script_workphone_string_binding_round_trip )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );

    luabind::module( state )[luabind::def( "roundTripWorkphoneString", roundTripWorkphoneString )];

    constexpr auto script = R"(
        local value = "Editor label"
        assert(roundTripWorkphoneString(value) == value)
    )";
    BOOST_CHECK( !luaL_dostring( state, script ) );
}

BOOST_AUTO_TEST_CASE( script_parameters_binding_round_trip )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );

    bindParam( state );
    bindParamList( state );

    constexpr auto script = R"(
        function inspectParameters(parameters)
            assert(type(parameters) == "userdata", type(parameters))
            assert(type(parameters.at) == "function", type(parameters.at))
            assert(type(parameters.size) == "function", type(parameters.size))
            assert(parameters:size() == 3, parameters:size())
            return parameters:at(1)
        end

        ParameterReceiver = {}
        function ParameterReceiver:inspectParameters(parameters, results)
            assert(type(parameters) == "userdata", type(parameters))
            assert(type(parameters.at) == "function", type(parameters.at))
            assert(type(results) == "userdata", type(results))
            assert(type(results.at) == "function", type(results.at))
            self.value = parameters:at(1)
        end
    )";
    BOOST_REQUIRE( luaL_dostring( state, script ) == LUA_OK );

    Parameters parameters;
    parameters.emplace_back( static_cast<u32>( 10u ) );
    parameters.emplace_back( static_cast<u32>( 20u ) );
    parameters.emplace_back( static_cast<u32>( 30u ) );

    const lua_Integer value =
        luabind::call_function<lua_Integer>( state, "inspectParameters", parameters );
    BOOST_CHECK_EQUAL( value, 20 );

    Parameters results;
    luabind::object receiver = luabind::globals( state )["ParameterReceiver"];
    luabind::call_member<void>( receiver, "inspectParameters", parameters, boost::ref( results ) );
    BOOST_CHECK_EQUAL( luabind::object_cast<lua_Integer>( receiver["value"] ), 20 );
}

BOOST_AUTO_TEST_CASE( script_parameters_binding_lua_construction )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );

    bindParam( state );
    bindParamList( state );

    constexpr auto script = R"(
        local parameters = Parameters()
        parameters:push_back("first")
        parameters:addAsString("second")
        parameters:add(42)
        parameters:addAsBool(true)

        assert(parameters:size() == 4, parameters:size())
        assert(parameters:at(0) == "first", tostring(parameters:at(0)))
        assert(parameters:getAsString(1) == "second", parameters:getAsString(1))
        assert(parameters:at(2) == 42, tostring(parameters:at(2)))
        assert(parameters:at(3) == true, tostring(parameters:at(3)))
    )";
    BOOST_CHECK( !luaL_dostring( state, script ) );
}

BOOST_AUTO_TEST_CASE( script_parameter_userdata_conversion )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );

    bindBaseObjects( state );
    bindParam( state );
    bindParamList( state );
    luabind::module( state )[luabind::def( "roundTripParameter", roundTripParameter ),
                             luabind::def( "getParameterArraySize", getParameterArraySize )];

    auto object = make_ptr<ISharedObject>();
    object->setName( "Parameter object" );
    luabind::globals( state )["parameterObject"] = object;

    constexpr auto script = R"(
        local explicitValue = Parameter()
        explicitValue:setS32(77)

        local destination = Parameters()
        destination:add(explicitValue)
        assert(destination:size() == 1, destination:size())
        assert(destination:at(0) == 77, tostring(destination:at(0)))

        local source = Parameters()
        source:add(1)
        source:add("two")
        assert(getParameterArraySize(source) == 2)

        local objectResult = roundTripParameter(parameterObject)
        assert(objectResult ~= nil)
        assert(objectResult:getName() == "Parameter object", objectResult:getName())
    )";
    BOOST_CHECK( !luaL_dostring( state, script ) );
}

BOOST_AUTO_TEST_CASE( script_parameter_converter_edge_cases )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );

    luabind::module( state )[luabind::class_<ColourF>( "ConverterColourF" ),
                             luabind::class_<Vector2D>( "ConverterVector2D" ),
                             luabind::class_<AABB3D>( "ConverterAABB3D" ),
                             luabind::class_<Transform3F>( "ConverterTransform3F" )];

    luabind::default_converter<Parameter> converter;

    Parameter unsignedValue( std::numeric_limits<u32>::max() );
    converter.to( state, unsignedValue );
    BOOST_REQUIRE( lua_isinteger( state, -1 ) );
    BOOST_CHECK_EQUAL( lua_tointeger( state, -1 ),
                       static_cast<lua_Integer>( std::numeric_limits<u32>::max() ) );
    lua_pop( state, 1 );

    int pointedValue = 42;
    Parameter pointerValue( static_cast<void *>( &pointedValue ) );
    converter.to( state, pointerValue );
    BOOST_REQUIRE( lua_islightuserdata( state, -1 ) );
    BOOST_CHECK_EQUAL( lua_touserdata( state, -1 ), &pointedValue );
    const auto pointerRoundTrip = converter.from( state, -1 );
    BOOST_CHECK( pointerRoundTrip.type == ParameterType::PARAM_TYPE_PTR );
    BOOST_CHECK_EQUAL( pointerRoundTrip.getPtr(), &pointedValue );
    lua_pop( state, 1 );

    lua_pushnumber( state, 1.0e30 );
    const auto largeNumber = converter.from( state, -1 );
    BOOST_CHECK( largeNumber.type == ParameterType::PARAM_TYPE_F64 );
    BOOST_CHECK_EQUAL( largeNumber.getF64(), 1.0e30 );
    lua_pop( state, 1 );

    lua_pushnumber( state, std::numeric_limits<lua_Number>::infinity() );
    const auto infiniteNumber = converter.from( state, -1 );
    BOOST_CHECK( infiniteNumber.type == ParameterType::PARAM_TYPE_F64 );
    BOOST_CHECK( std::isinf( infiniteNumber.getF64() ) );
    lua_pop( state, 1 );

    constexpr char embeddedNull[] = { 'a', '\0', 'b' };
    lua_pushlstring( state, embeddedNull, sizeof( embeddedNull ) );
    const auto stringValue = converter.from( state, -1 );
    BOOST_REQUIRE( stringValue.type == ParameterType::PARAM_TYPE_STR );
    BOOST_REQUIRE_EQUAL( stringValue.str.size(), sizeof( embeddedNull ) );
    BOOST_CHECK_EQUAL( stringValue.str[1], '\0' );
    lua_pop( state, 1 );

    converter.to( state, stringValue );
    size_t stringLength = 0;
    const auto *luaString = lua_tolstring( state, -1, &stringLength );
    BOOST_REQUIRE( luaString );
    BOOST_REQUIRE_EQUAL( stringLength, sizeof( embeddedNull ) );
    BOOST_CHECK_EQUAL( luaString[1], '\0' );
    lua_pop( state, 1 );

    const auto makeComplexParameter = []( ParameterType type, std::initializer_list<f64> values ) {
        Parameter value;
        value.type = type;
        value.data.pData = nullptr;
        for( const auto component : values )
        {
            value.array.emplace_back( component );
        }
        return value;
    };

    converter.to( state, makeComplexParameter( ParameterType::PARAM_TYPE_COLOURI,
                                               { -1.0, 300.0, 128.0, 255.0 } ) );
    auto colourObject = luabind::object( luabind::from_stack( state, -1 ) );
    const auto colourValue = luabind::object_cast<ColourF>( colourObject );
    BOOST_CHECK_EQUAL( colourValue.r, 0.0f );
    BOOST_CHECK_EQUAL( colourValue.g, 1.0f );
    BOOST_CHECK_CLOSE( colourValue.b, static_cast<f32>( 128.0 / 255.0 ), 0.001 );
    BOOST_CHECK_EQUAL( colourValue.a, 1.0f );
    lua_pop( state, 1 );

    converter.to( state, makeComplexParameter( ParameterType::PARAM_TYPE_VEC2D,
                                               { 1.0000000000001, -2.0000000000002 } ) );
    auto vectorObject = luabind::object( luabind::from_stack( state, -1 ) );
    const auto vectorValue = luabind::object_cast<Vector2D>( vectorObject );
    BOOST_CHECK_SMALL( vectorValue.X() - 1.0000000000001, 1.0e-12 );
    BOOST_CHECK_SMALL( vectorValue.Y() + 2.0000000000002, 1.0e-12 );
    lua_pop( state, 1 );

    converter.to( state, makeComplexParameter( ParameterType::PARAM_TYPE_AABB3D,
                                               { -1.0, -2.0, -3.0, 4.0, 5.0, 6.0 } ) );
    auto aabbObject = luabind::object( luabind::from_stack( state, -1 ) );
    const auto aabbValue = luabind::object_cast<AABB3D>( aabbObject );
    BOOST_CHECK_EQUAL( aabbValue.getMinimum().X(), -1.0 );
    BOOST_CHECK_EQUAL( aabbValue.getMaximum().Z(), 6.0 );
    lua_pop( state, 1 );

    converter.to( state,
                  makeComplexParameter( ParameterType::PARAM_TYPE_TRANSFORM3,
                                        { 10.0, 20.0, 30.0, 1.0, 0.0, 0.0, 0.0, 2.0, 3.0, 4.0 } ) );
    auto transformObject = luabind::object( luabind::from_stack( state, -1 ) );
    const auto transformValue = luabind::object_cast<Transform3F>( transformObject );
    BOOST_CHECK_EQUAL( transformValue.getPosition().X(), 10.0f );
    BOOST_CHECK_EQUAL( transformValue.getScale().Z(), 4.0f );
    BOOST_CHECK_EQUAL( transformValue.getOrientation().W(), 1.0f );
    lua_pop( state, 1 );

    const auto setGlobal = [&]( const char *name, const Parameter &value ) {
        converter.to( state, value );
        lua_setglobal( state, name );
    };

    setGlobal( "quaternionDValue", makeComplexParameter( ParameterType::PARAM_TYPE_QUATD,
                                                         { 1.0000000000001, 2.0, 3.0, 4.0 } ) );
    setGlobal( "transformDValue",
               makeComplexParameter( ParameterType::PARAM_TYPE_TRANSFORM3D,
                                     { 10.0, 20.0, 30.0, 1.0, 0.0, 0.0, 0.0, 2.0, 3.0, 4.0 } ) );

    constexpr auto script = R"(
        local epsilon = 1.0e-12

        assert(type(quaternionDValue) == "table", type(quaternionDValue))
        assert(math.abs(quaternionDValue.w - 1.0000000000001) < epsilon, quaternionDValue.w)
        assert(quaternionDValue[4] == 4.0, quaternionDValue[4])

        assert(type(transformDValue) == "table", type(transformDValue))
        assert(transformDValue.position.x == 10.0, transformDValue.position.x)
        assert(transformDValue.orientation.w == 1.0, transformDValue.orientation.w)
        assert(transformDValue.scale.z == 4.0, transformDValue.scale.z)
    )";
    BOOST_CHECK( !luaL_dostring( state, script ) );
}

BOOST_AUTO_TEST_CASE( script_weak_pointer_binding_round_trip )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );

    bindBaseObjects( state );
    luabind::module( state )[luabind::def( "roundTripWeakPointer", roundTripWeakPointer )];

    auto object = make_ptr<ISharedObject>();
    object->setName( "Weak pointer target" );
    luabind::globals( state )["weakPointerTarget"] = object;

    constexpr auto script = R"(
        local result = roundTripWeakPointer(weakPointerTarget)
        assert(result ~= nil)
        assert(result:getName() == "Weak pointer target", result:getName())
        assert(roundTripWeakPointer(nil) == nil)
    )";
    BOOST_CHECK( !luaL_dostring( state, script ) );
}

BOOST_AUTO_TEST_CASE( script_catch_game_movement_and_bounds )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );
    requireLuaFile( state, getCatchGameCorePath() );

    constexpr auto script = R"(
        local game = CatchGameCore.new({
            width = 400,
            playerWidth = 100,
            playerSpeed = 200,
            spawnInterval = 100,
            maxFrameDelta = 10,
        }, function() return 0.5 end)

        game:update(0.5, 1)
        assert(game.playerX == 100, game.playerX)
        game:update(10, 1)
        assert(game.playerX == 150, game.playerX)
        game:update(10, -1)
        assert(game.playerX == -150, game.playerX)
        game:update(1, 10)
        assert(game.playerX == 50, game.playerX)
        game:update(1, 0 / 0)
        assert(game.playerX == 50, game.playerX)
    )";
    BOOST_CHECK( !luaL_dostring( state, script ) );
}

BOOST_AUTO_TEST_CASE( script_catch_game_spawn_catch_miss_and_reset )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );
    requireLuaFile( state, getCatchGameCorePath() );

    constexpr auto script = R"(
        local game = CatchGameCore.new({
            width = 400,
            height = 300,
            playerWidth = 100,
            playerHeight = 20,
            playerY = 100,
            itemSize = 20,
            itemFallSpeed = 100,
            speedIncreasePerCatch = 0,
            spawnInterval = 0.5,
            maxFrameDelta = 10,
            maxMisses = 2,
        }, function() return 0.25 end)

        local spawned = game:update(0.5, 0)
        assert(#spawned == 1 and spawned[1].type == "spawned")
        assert(math.abs(spawned[1].item.x + 95) < 0.001, spawned[1].item.x)

        game.items = {}
        game.spawnElapsed = 0
        game:spawnItem(0, 80)
        local caught = game:update(0.2, 0)
        assert(#caught == 1 and caught[1].type == "caught", #caught)
        assert(game.score == 1 and #game.items == 0)

        game:spawnItem(180, 170)
        local firstMiss = game:update(0.01, 0)
        assert(firstMiss[1].type == "missed")
        assert(game.misses == 1 and game.running)

        game:spawnItem(-180, 170)
        local gameOver = game:update(0.01, 0)
        assert(gameOver[1].type == "missed")
        assert(gameOver[2].type == "game_over")
        assert(game.misses == 2 and not game.running)

        local frozenX = game.playerX
        assert(#game:update(1, 1) == 0 and game.playerX == frozenX)
        game:reset()
        assert(game.running and game.score == 0 and game.misses == 0)
        assert(game.playerX == 0 and #game.items == 0)
    )";
    BOOST_CHECK( !luaL_dostring( state, script ) );
}

BOOST_AUTO_TEST_CASE( script_catch_game_component_is_valid_lua )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );
    requireLuaFile( state, getCatchGameCorePath() );

    constexpr auto componentEnvironment = R"(
        BaseComponent = {
            __init = function() end,
            __finalize = function() end,
        }
        function class(name)
            local result = {}
            _G[name] = result
            return function(base)
                setmetatable(result, { __index = base })
            end
        end
        function include() end
    )";
    BOOST_REQUIRE( luaL_dostring( state, componentEnvironment ) == LUA_OK );
    requireLuaFile( state, getCatchGameComponentPath() );

    constexpr auto assertions = R"(
        assert(type(CatchGame) == "table")
        assert(type(CatchGame.update) == "function")
        assert(type(CatchGame.reset) == "function")
        assert(type(CatchGame.shutdown) == "function")
        assert(type(launchCatchGame) == "function")
    )";
    BOOST_CHECK( !luaL_dostring( state, assertions ) );
}

BOOST_AUTO_TEST_CASE( script_catch_game_project_bootstrap_launches_game )
{
    LuaStateGuard lua;
    auto state = lua.get();
    BOOST_REQUIRE( state );

    auto bootstrapPath = getCatchGameProjectBootstrapPath();
    if( !std::filesystem::exists( bootstrapPath ) )
    {
        BOOST_TEST_MESSAGE( "CatchGame project bootstrap is not available - skipping test" );
        return;
    }
    constexpr auto projectEnvironment = R"(
        CatchGame = {}
        catchGameLaunches = 0
        function include()
            error("Bootstrap unexpectedly reloaded an available CatchGame component")
        end
        function launchCatchGame()
            catchGameLaunches = catchGameLaunches + 1
        end
    )";
    BOOST_REQUIRE( luaL_dostring( state, projectEnvironment ) == LUA_OK );
    requireLuaFile( state, getCatchGameProjectBootstrapPath() );
    BOOST_CHECK( !luaL_dostring( state, "assert(catchGameLaunches == 1)" ) );
}

#endif

BOOST_AUTO_TEST_CASE( script_components )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto script = String( "lua_unit_tests.lua" );

        auto scriptManager = applicationManager->getScriptManager();
        if( scriptManager )
        {
            scriptManager->loadScript( script );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( script_generator_houdini )
{
    if( !runScriptGeneratorIntegrationTests() )
    {
        BOOST_TEST_MESSAGE( "Skipping Houdini script generator integration test." );
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();

        auto workingDirectory = Path::getWorkingDirectory();
        BOOST_CHECK( !StringUtil::isNullOrEmpty( workingDirectory ) );
        fileSystem->addFolder( workingDirectory, true );

        ScriptGenerator scriptGenerator;
        auto csharpPath = "F:/dev/ProceduralWorld/Assets/HoudiniEngineUnity/Scripts";
        auto cppPath = "E:/dev/HoudiniEngineUnity";
        if( !std::filesystem::exists( csharpPath ) )
        {
            BOOST_TEST_MESSAGE(
                "Skipping Houdini script generator test because source path is unavailable." );
            return;
        }

        fileSystem->addFolder( csharpPath, true );
        fileSystem->addFolder( cppPath, true );

        scriptGenerator.setProjectPath( "FBHoudini" );
        scriptGenerator.setReplaceFileName( "HEU_" );
        scriptGenerator.setReplacementFileName( "" );

        auto namespaceNames = Array<String>();
        namespaceNames.push_back( "fb" );
        namespaceNames.push_back( "houdini" );
        scriptGenerator.setNamespaceNames( namespaceNames );

        scriptGenerator.addMapEntry( "HAPI_ObjectInfo[]", "Array<HAPI_ObjectInfo>" );
        scriptGenerator.addMapEntry( "HAPI_Transform[]", "Array<HAPI_Transform>" );
        scriptGenerator.addMapEntry( "List<HEU_VolumeCachePreset>", "Array<VolumeCachePreset>" );
        scriptGenerator.addMapEntry( "List<HEU_InputPreset>", "Array<InputPreset>" );
        scriptGenerator.addMapEntry( "List<HEU_Handle>", "Array<Handle>" );

        scriptGenerator.convertCSharp( csharpPath, cppPath );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( script_generator_vehicle )
{
    if( !runScriptGeneratorIntegrationTests() )
    {
        BOOST_TEST_MESSAGE( "Skipping vehicle script generator integration test." );
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();

        auto workingDirectory = Path::getWorkingDirectory();
        BOOST_CHECK( !StringUtil::isNullOrEmpty( workingDirectory ) );
        fileSystem->addFolder( workingDirectory, true );

        ScriptGenerator scriptGenerator;
        auto csharpPath = "F:/dev/ProceduralWorld/Assets/NWH";
        auto cppPath = "E:/dev/vehicle_code";
        if( !std::filesystem::exists( csharpPath ) )
        {
            BOOST_TEST_MESSAGE(
                "Skipping vehicle script generator test because source path is unavailable." );
            return;
        }

        fileSystem->addFolder( csharpPath, true );
        fileSystem->addFolder( cppPath, true );

        scriptGenerator.setProjectPath( "FBVehicle" );
        scriptGenerator.setReplaceFileName( "" );
        scriptGenerator.setReplacementFileName( "" );

        auto namespaceNames = Array<String>();
        namespaceNames.push_back( "fb" );
        scriptGenerator.setNamespaceNames( namespaceNames );

        scriptGenerator.convertCSharp( csharpPath, cppPath );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( script_generator_game )
{
    if( !runScriptGeneratorIntegrationTests() )
    {
        BOOST_TEST_MESSAGE( "Skipping game script generator integration test." );
        return;
    }

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();

        auto workingDirectory = Path::getWorkingDirectory();
        BOOST_CHECK( !StringUtil::isNullOrEmpty( workingDirectory ) );
        fileSystem->addFolder( workingDirectory, true );

        ScriptGenerator scriptGenerator;
        auto csharpPath = "F:/dev/WP_v3/Assets/_game/scripts";
        auto cppPath = "E:/dev/game_code";
        if( !std::filesystem::exists( csharpPath ) )
        {
            BOOST_TEST_MESSAGE(
                "Skipping game script generator test because source path is unavailable." );
            return;
        }

        fileSystem->addFolder( csharpPath, true );

        // if (fileSystem->isExistingFolder(csharpPath))
        {
            if( !fileSystem->isExistingFolder( cppPath ) )
            {
                fileSystem->createDirectories( cppPath );
            }

            fileSystem->addFolder( cppPath, true );

            scriptGenerator.setProjectPath( "FBApplication" );
            scriptGenerator.setReplaceFileName( "" );
            scriptGenerator.setReplacementFileName( "" );

            auto namespaceNames = Array<String>();
            namespaceNames.push_back( "fb" );
            scriptGenerator.setNamespaceNames( namespaceNames );

            scriptGenerator.convertCSharp( csharpPath, cppPath );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( script_lua_manager )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();

        auto scriptMgr = factoryManager->make_object<IScriptManager>( "Lua" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
