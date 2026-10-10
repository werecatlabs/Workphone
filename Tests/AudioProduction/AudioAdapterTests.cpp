#include <Workphone/Memory/TypeManager.hpp>
#include <WPAudio/WPAudioManager.hpp>
#include <WPAudio/WPAudioSound.hpp>
#include <WPAudio/WPAudioEffectVolume.hpp>
#include <cstdio>
#include <stdexcept>
#include <limits>
using namespace workphone;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)
class FixtureManager : public WPAudioManager {
public:
    void track(SmartPtr<ISound> sound) { sound->setOwner(this); m_sounds.push_back(sound); }
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
        float input[]{1,-1,0.25f}, output[]{99,99,99};
        CAudioEffectVolume volume; volume.setInput(input); volume.setOutput(output); volume.setNumSamples(3);
        volume.setVolume(0.5f); volume.process(); CHECK(output[0]==0.5f && output[1]==-0.5f);
        volume.setBypass(true); volume.process(); CHECK(output[0]==1 && output[1]==-1 && output[2]==0.25f);
        volume.setOutput(input); volume.process(); CHECK(input[0]==1);
        std::puts("WPAudio adapter registry/master gain and bypass passed (no device output)");
    } catch(const std::exception &e) { std::fprintf(stderr,"%s\n",e.what()); result=1; }
    TypeManager::setInstance(nullptr); types.unload(); return result;
}
