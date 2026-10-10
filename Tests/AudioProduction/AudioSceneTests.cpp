#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Sound/SoundManager.hpp>
#include <Workphone/Sound/Sound.hpp>
#include <Workphone/Scene/Components/AudioEmitter.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <cstdio>
#include <stdexcept>

using namespace workphone;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)
// Device-independent scene fixture. Production emitter, clone, registry and
// weak ownership code; this test deliberately does not claim device playback.
class FixtureSound : public Sound {
public:
    void load(SmartPtr<ISharedObject>) override { setLoadingState(LoadingState::Loaded); }
    void unload(SmartPtr<ISharedObject>) override { stop(); setLoadingState(LoadingState::Unloaded); }
};
class FixtureManager : public SoundManager {
public:
    using SoundManager::create;
    SmartPtr<IResource> create(const String &path) override {
        auto sound=make_ptr<FixtureSound>(); sound->setFilePath(path); sound->setOwner(this);
        sound->load(nullptr); m_sounds.push_back(sound); return sound;
    }
    Array<SmartPtr<ISound>> sounds() { return m_sounds.snapshot(); }
};
int main() {
    TypeManager types; types.load(); TypeManager::setInstance(&types);
    auto application=make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance(application);
    int result=0;
    try {
        auto manager=make_ptr<FixtureManager>();
        auto asset=dynamic_pointer_cast<ISound>(manager->create("shared.wav"));
        asset->setVolume(0.25f); asset->setLoop(true);
        auto unrelated=make_ptr<FixtureManager>(); unrelated->destroyResource(asset);
        CHECK(asset->isLoaded() && asset->getOwner()==manager);
        auto first=make_ptr<scene::AudioEmitter>(), second=make_ptr<scene::AudioEmitter>();
        first->setSound(asset); second->setSound(asset);
        first->play(); second->play();
        auto sounds=manager->sounds(); CHECK(sounds.size()==3);
        CHECK(first->getSound()==asset && second->getSound()==asset);
        CHECK(!asset->isPlaying() && sounds[1]->isPlaying() && sounds[2]->isPlaying());
        CHECK(sounds[1]->getVolume()==0.25f && sounds[2]->getLoop());
        first->pause(); CHECK(!sounds[1]->isPlaying() && sounds[2]->isPlaying());
        first->unpause(); CHECK(sounds[1]->isPlaying());
        second->stop(); CHECK(sounds[1]->isPlaying() && !sounds[2]->isPlaying());
        first->setSound(nullptr); CHECK(!sounds[1]->isLoaded() && manager->sounds().size()==2);
        second->unload(nullptr); CHECK(!sounds[2]->isLoaded() && manager->sounds().size()==1);
        second->unload(nullptr); CHECK(manager->sounds().size()==1);
        manager->destroyAll(); CHECK(!asset->isLoaded() && !asset->getOwner());
        // Dropping the last manager reference must not be kept alive by a sound.
        auto retained=dynamic_pointer_cast<ISound>(manager->create("other.wav"));
        WeakPtr<FixtureManager> weak=manager; manager=nullptr;
        CHECK(weak.expired() && !retained->getOwner());
        std::puts("Audio scene independence/cleanup passed (no device output)");
    } catch(const std::exception &e) { std::fprintf(stderr,"%s\n",e.what()); result=1; }
    core::IApplicationManager::setInstance(nullptr); application=nullptr;
    TypeManager::setInstance(nullptr); types.unload(); return result;
}
