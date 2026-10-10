#pragma once
#include <Workphone/Sound/Sound.hpp>
#include <Workphone/Sound/SoundManager.hpp>

// Production LuaManager and SoundBind conversions with a device-independent
// sound fixture. Device playback is tested separately.
inline void runLuaAudioContracts(workphone::LuaManager &lua)
{
    using namespace workphone;
    class AudioFixture : public Sound
    {
    public:
        void unload(SmartPtr<ISharedObject>) override
        {
            stop(); setLoadingState(LoadingState::Unloaded);
        }
    };
    class ManagerFixture : public SoundManager
    {
    public:
        SmartPtr<IResource> create(const String &path) override
        {
            auto sound = make_ptr<AudioFixture>();
            sound->setFilePath(path); sound->setOwner(this);
            sound->setLoadingState(path == "missing.wav" ? LoadingState::Error : LoadingState::Loaded);
            m_sounds.push_back(sound);
            return sound;
        }
        size_t count() { return m_sounds.snapshot().size(); }
    };
    auto owner = make_ptr<ManagerFixture>();
    auto other = make_ptr<ManagerFixture>();
    auto state = lua.getLuaState();
    luabind::globals(state)["audioOwner"] = SmartPtr<ISoundManager>(owner);
    luabind::globals(state)["audioOther"] = SmartPtr<ISoundManager>(other);
    if(!lua.executeSource(R"(
        assert(audioOwner:createSound('missing.wav', false) == nil)
        audioFirst = assert(audioOwner:createSound('shared.wav', true))
        audioSecond = assert(audioOwner:createSound('shared.wav', false))
        assert(audioFirst:isLoaded() and audioSecond:isLoaded())
        assert(audioFirst:getLoop() and not audioSecond:getLoop())
        audioFirst:setVolume(0.25)
        assert(audioSecond:getVolume() == 1)
        audioFirst:play()
        assert(audioFirst:isPlaying() and not audioSecond:isPlaying())
        audioOther:destroySound(audioFirst)
        assert(audioFirst:isLoaded())
        audioOwner:destroySound(audioFirst)
        assert(not audioFirst:isLoaded() and not audioFirst:isPlaying())
        audioOwner:destroySound(audioFirst)
        audioOwner:destroySound(audioSecond)
        audioFirst = nil; audioSecond = nil
        audioOwner = nil; audioOther = nil
        collectgarbage('collect')
    )", "@Tests/LuaAudioContracts.lua"))
        throw std::runtime_error(lua.getLastDiagnostic().c_str());
    if(owner->count() != 0 || other->count() != 0)
        throw std::runtime_error("Lua audio instance cleanup left registered resources");
}
