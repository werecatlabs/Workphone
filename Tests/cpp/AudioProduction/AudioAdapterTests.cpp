#include <Workphone/Memory/TypeManager.hpp>
#include <WPAudio/WPAudioManager.hpp>
#include <WPAudio/WPAudioSound.hpp>
#include <WPAudio/WPAudioEffectVolume.hpp>
#include <WPAudio/WPAudioSoundListener.hpp>
#include <Workphone/Sound/Sound.hpp>
#include <Workphone/Core/Properties.hpp>
#include <cstdio>
#include <stdexcept>
#include <limits>
#include <cmath>
using namespace workphone;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)
class FixtureManager : public WPAudioManager {
public:
    void track(SmartPtr<ISound> sound) { sound->setOwner(this); m_sounds.push_back(sound); }
    void trackListener(SmartPtr<ISoundListener3> listener) { m_listeners.push_back(listener); }
};
class UpdatingSound : public Sound {
public:
    unsigned updates=0;
    UpdatingSound() { setLoadingState(LoadingState::Loaded); }
    void update() override { ++updates; }
};
int main() {
    TypeManager types; types.load(); TypeManager::setInstance(&types);
    int result=0;
    try {
        auto manager=make_ptr<FixtureManager>(); auto sound=make_ptr<WPAudioSound>();
        sound->setFilePath("registered.wav"); sound->setVolume(0.3f); manager->track(sound);
        CHECK(manager->getByName("registered.wav")==sound);
        manager->setVolume(0.5f); CHECK(sound->getVolume()==0.3f && manager->getVolume()==0.5f);
        manager->setMute(true); manager->setVolume(0.25f); manager->setMute(false);
        CHECK(sound->getVolume()==0.3f && manager->getVolume()==0.25f && !manager->isMute());
        manager->setVolume(std::numeric_limits<float>::quiet_NaN()); CHECK(manager->getVolume()==0);
        manager->destroyResource(sound); CHECK(!sound->getOwner() && !manager->getByName("registered.wav"));
        manager->track(sound); manager->removeAll(); CHECK(!manager->getByName("registered.wav") && !sound->getOwner());
        CHECK(!manager->isRealtime());
        // No device is required to verify the concrete owner's suspended update
        // boundary or that holding it never changes individual voice intent.
        {
            auto playing=make_ptr<UpdatingSound>(), paused=make_ptr<UpdatingSound>();
            manager->track(playing); manager->track(paused);
            playing->play(); paused->play(); paused->pause();
            manager->update();
            CHECK(playing->updates==1 && paused->updates==1);
            CHECK(manager->setDebuggerSuspended(true) && manager->getDebuggerSuspended());
            for(unsigned frame=0;frame<4;++frame) manager->update();
            CHECK(playing->updates==1 && paused->updates==1 && playing->isPlaying() && !paused->isPlaying());
            CHECK(manager->setDebuggerSuspended(true));
            CHECK(manager->setDebuggerSuspended(false) && !manager->getDebuggerSuspended());
            manager->update();
            CHECK(playing->updates==2 && paused->updates==2 && playing->isPlaying() && !paused->isPlaying());
            manager->removeAll();
        }
        auto settings=make_ptr<Properties>();
        settings->setProperty("sound.musicVolume", s32(60));
        settings->setProperty("sound.sfxVolume", s32(20));
        settings->setProperty("sound.voiceVolume", s32(80));
        settings->setProperty("sound.voiceLimit", s32(1));
        settings->setProperty("sound.bufferSize", s32(512));
        manager->setProperties(settings);
        auto music=make_ptr<Sound>(), second=make_ptr<Sound>();
        manager->track(music); manager->track(second);
        music->setCategory("music"); music->setVolume(0.5f);
        float gain=0, pan=0;
        manager->getMixParameters(music.get(), gain, pan);
        CHECK(std::abs(gain-0.3f)<0.0001f && music->getVolume()==0.5f);
        music->setCategory("sfx"); manager->getMixParameters(music.get(), gain, pan);
        CHECK(std::abs(gain-0.1f)<0.0001f);
        music->play(); CHECK(!manager->admitVoice(second.get()));
        music->stop(); CHECK(manager->admitVoice(second.get()));
        auto listener=make_ptr<WPAudioSoundListener>();
        listener->setPosition(Vector3F::zero()); listener->setForwardVector(Vector3F(0,0,-1));
        manager->trackListener(listener);
        music->setFlag(ISound::SOUND_FLAG_3D, true);
        music->setPosition(Vector3F(6,0,0)); music->setMinMaxDistance(1,11);
        manager->getMixParameters(music.get(), gain, pan);
        CHECK(std::abs(gain-0.05f)<0.0001f && pan>0.99f);
        settings->setProperty("sound.spatialization", false); manager->setProperties(settings);
        manager->getMixParameters(music.get(), gain, pan);
        CHECK(std::abs(gain-0.1f)<0.0001f && pan==0);
        CHECK(manager->getPlaybackBufferFrames()==512);
        manager->removeAll();
        float input[]{1,-1,0.25f}, output[]{99,99,99};
        CAudioEffectVolume volume; volume.setInput(input); volume.setOutput(output); volume.setNumSamples(3);
        volume.setVolume(0.5f); volume.process(); CHECK(output[0]==0.5f && output[1]==-0.5f);
        volume.setBypass(true); volume.process(); CHECK(output[0]==1 && output[1]==-1 && output[2]==0.25f);
        volume.setOutput(input); volume.process(); CHECK(input[0]==1);
        std::puts("WPAudio adapter registry/master gain and bypass passed (no device output)");
    } catch(const std::exception &e) { std::fprintf(stderr,"%s\n",e.what()); result=1; }
    TypeManager::setInstance(nullptr); types.unload(); return result;
}
