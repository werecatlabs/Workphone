#include <WPLua/LuaManager.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Scene/Components/ProceduralRaceScene.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <luabind/luabind.hpp>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
extern "C"
{
#include <lauxlib.h>
#include <lualib.h>
}

int main()
{
    using namespace workphone;
    struct RuntimeTypes
    {
        TypeManager types;
        RuntimeTypes() { types.load(); TypeManager::setInstance(&types); }
        ~RuntimeTypes() { TypeManager::setInstance(nullptr); types.unload(); }
    } runtimeTypes;
    LuaManager manager;
    try
    {
        manager.load( nullptr );
        auto state = manager.getLuaState();
        if ( !state ) throw std::runtime_error( "Lua manager did not create a state" );
        auto race = make_ptr<scene::ProceduralRaceScene>();
        luabind::globals( state )["raceScene"] = race;
        const auto core =
            std::filesystem::path( WP_TEST_SOURCE_ROOT ) / "bin/Media/Scripts/Lua/Game/Core";
        for ( const auto file :
              { "BaseComponent.lua", "RaceSession.lua", "SampleVehicleAdvanced.lua" } )
            if ( luaL_dofile( state, ( core / file ).string().c_str() ) != LUA_OK )
                throw std::runtime_error( lua_tostring( state, -1 ) );
        const char* test = R"(
            assert(raceScene:getAudioEnabled() and raceScene:getEffectsEnabled())
            assert(not raceScene:isAudioAvailable() and not raceScene:isEffectsAvailable())
            assert(raceScene:getParticleCount() == 0 and raceScene:getSkidDecalCount() == 0)
            local sample = SampleVehicleAdvanced(raceScene)
            sample.raceScene = raceScene
            sample:setPresentationOptions(false, true)
            assert(not raceScene:getAudioEnabled() and raceScene:getEffectsEnabled())
            sample:setPresentationOptions(true, false)
            assert(raceScene:getAudioEnabled() and not raceScene:getEffectsEnabled())
            sample:setPresentationOptions(false, false)
            assert(not raceScene:getAudioEnabled() and not raceScene:getEffectsEnabled())
            -- Remove the test reference before Lua finalization invokes sample cleanup.
            sample.raceScene = nil
            collectgarbage('collect')
        )";
        if ( luaL_dostring( state, test ) != LUA_OK )
            throw std::runtime_error( lua_tostring( state, -1 ) );
        manager.unload( nullptr );
        std::puts( "Vehicle Lua presentation bindings: PASS" );
        return 0;
    }
    catch ( const std::exception& e )
    {
        std::fprintf( stderr, "Vehicle Lua presentation bindings: FAIL: %s\n", e.what() );
        manager.unload( nullptr );
        return 1;
    }
}
