#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/SoundBind.hpp"
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    namespace
    {
        SmartPtr<ISound> createSoundInstance( ISoundManager *manager, const String &path, bool loop )
        {
            if( !manager ) return nullptr;
            auto resource = manager->create( path );
            auto sound = workphone::dynamic_pointer_cast<ISound>( resource );
            if( !sound || !sound->isLoaded() )
            {
                if( resource ) manager->destroyResource( resource );
                return nullptr;
            }
            sound->setLoop( loop );
            return sound;
        }

        void destroySoundInstance( ISoundManager *manager, SmartPtr<ISound> sound )
        {
            if( manager && sound ) manager->destroyResource( sound );
        }
    }

    void bindSound( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<ISound, IResource, SmartPtr<ISound>>( "ISound" )
                        .def( "play", &ISound::play )
                        .def( "pause", &ISound::pause )
                        .def( "stop", &ISound::stop )
                        .def( "isPlaying", &ISound::isPlaying )
                        .def( "setVolume", &ISound::setVolume )
                        .def( "getVolume", &ISound::getVolume )
                        .def( "setLoop", &ISound::setLoop )
                        .def( "getLoop", &ISound::getLoop )
                        .def( "setPan", &ISound::setPan )
                        .def( "setPosition", &ISound::setPosition )
                        .def( "getPosition", &ISound::getPosition )
                        .def( "setMinMaxDistance", &ISound::setMinMaxDistance )
                        .def( "getOwner", &ISound::getOwner )
                        .def( "setOwner", &ISound::setOwner )
                        .def( "getFlags", &ISound::getFlags )
                        .def( "setFlags", &ISound::setFlags )
                        .def( "setFlag", &ISound::setFlag )
                        .def( "getFlag", &ISound::getFlag )
                        .def( "getName", &ISound::getName )
                        .scope[def( "typeInfo", ISound::typeInfo )]];

        module( L )[class_<ISoundManager, IResourceManager, SmartPtr<ISoundManager>>( "ISoundManager" )
                        .def( "createSound", &createSoundInstance )
                        .def( "destroySound", &destroySoundInstance )
                        .def( "addListener3", &ISoundManager::addListener3 )
                        .def( "findListener3", &ISoundManager::findListener3 )
                        .def( "setVolume", &ISoundManager::setVolume )
                        .def( "getVolume", &ISoundManager::getVolume )
                        .def( "startRecording", &ISoundManager::startRecording )
                        .def( "stopRecording", &ISoundManager::stopRecording )
                        .def( "getBufferSize", &ISoundManager::getBufferSize )
                        .def( "isRealtime", &ISoundManager::isRealtime )
                        .def( "setMute", &ISoundManager::setMute )
                        .def( "isMute", &ISoundManager::isMute )
                        .def( "loadObject", &ISoundManager::loadObject )
                        .def( "unloadObject", &ISoundManager::unloadObject )
                        .def( "setFlags", &ISoundManager::setFlags )
                        .def( "getFlags", &ISoundManager::getFlags )
                        .def( "setFlag", &ISoundManager::setFlag )
                        .def( "getFlag", &ISoundManager::getFlag )
                        .def( "getFactoryManager", &ISoundManager::getFactoryManager )
                        .def( "setFactoryManager", &ISoundManager::setFactoryManager )
                        .scope[def( "typeInfo", ISoundManager::typeInfo )]];

        module(
            L )[class_<IAudioBusBuffers, ISharedObject, SmartPtr<IAudioBusBuffers>>( "IAudioBusBuffers" )
                    .def( "getNumChannels", &IAudioBusBuffers::getNumChannels )
                    .def( "setNumChannels", &IAudioBusBuffers::setNumChannels )
                    .def( "getSilenceFlags", &IAudioBusBuffers::getSilenceFlags )
                    .def( "setSilenceFlags", &IAudioBusBuffers::setSilenceFlags )];

        module( L )[class_<IAudioEffect, ISharedObject, SmartPtr<IAudioEffect>>( "IAudioEffect" )
                        .def( "process", &IAudioEffect::process )
                        .def( "getNumSamples", &IAudioEffect::getNumSamples )
                        .def( "setNumSamples", &IAudioEffect::setNumSamples )
                        .def( "getBypass", &IAudioEffect::getBypass )
                        .def( "setBypass", &IAudioEffect::setBypass )
                        .def( "getSampleRate", &IAudioEffect::getSampleRate )
                        .def( "setSampleRate", &IAudioEffect::setSampleRate )];

        module( L )[class_<IAudioEffectDelay, IAudioEffect, SmartPtr<IAudioEffectDelay>>(
                        "IAudioEffectDelay" )
                        .def( "getNumChannels", &IAudioEffectDelay::getNumChannels )
                        .def( "setNumChannels", &IAudioEffectDelay::setNumChannels )];

        module( L )[class_<IAudioEffectVolume, IAudioEffect, SmartPtr<IAudioEffectVolume>>(
                        "IAudioEffectVolume" )
                        .def( "getVolume", &IAudioEffectVolume::getVolume )
                        .def( "setVolume", &IAudioEffectVolume::setVolume )];

        module( L )[class_<IAudioProcessData, ISharedObject, SmartPtr<IAudioProcessData>>(
                        "IAudioProcessData" )
                        .def( "getProcessMode", &IAudioProcessData::getProcessMode )
                        .def( "setProcessMode", &IAudioProcessData::setProcessMode )
                        .def( "getSymbolicSampleSize", &IAudioProcessData::getSymbolicSampleSize )
                        .def( "setSymbolicSampleSize", &IAudioProcessData::setSymbolicSampleSize )
                        .def( "getNumSamples", &IAudioProcessData::getNumSamples )
                        .def( "setNumSamples", &IAudioProcessData::setNumSamples )
                        .def( "getNumInputs", &IAudioProcessData::getNumInputs )
                        .def( "setNumInputs", &IAudioProcessData::setNumInputs )
                        .def( "getNumOutputs", &IAudioProcessData::getNumOutputs )
                        .def( "setNumOutputs", &IAudioProcessData::setNumOutputs )
                        .def( "getInputs", &IAudioProcessData::getInputs )
                        .def( "getOutputs", &IAudioProcessData::getOutputs )];

        module( L )[class_<ISoundEvent, ISharedObject, SmartPtr<ISoundEvent>>( "ISoundEvent" )
                        .def( "start", &ISoundEvent::start )
                        .def( "stop", &ISoundEvent::stop )
                        .def( "isPlaying", &ISoundEvent::isPlaying )
                        .def( "setMute", &ISoundEvent::setMute )
                        .def( "getVolume", &ISoundEvent::getVolume )
                        .def( "setVolume", &ISoundEvent::setVolume )
                        .def( "getParameter", &ISoundEvent::getParameter )
                        .def( "set3DAttributes", &ISoundEvent::set3DAttributes )];

        module(
            L )[class_<ISoundEventGroup, ISharedObject, SmartPtr<ISoundEventGroup>>( "ISoundEventGroup" )
                    .def( "setVolume", &ISoundEventGroup::setVolume )
                    .def( "setMute", &ISoundEventGroup::setMute )];

        module(
            L )[class_<ISoundEventParam, ISharedObject, SmartPtr<ISoundEventParam>>( "ISoundEventParam" )
                    .def( "getValue", &ISoundEventParam::getValue )
                    .def( "setValue", &ISoundEventParam::setValue )];

        module(
            L )[class_<ISoundListener3, ISharedObject, SmartPtr<ISoundListener3>>( "ISoundListener3" )
                    .def( "setPosition", &ISoundListener3::setPosition )
                    .def( "getPosition", &ISoundListener3::getPosition )
                    .def( "setForwardVector", &ISoundListener3::setForwardVector )
                    .def( "getForwardVector", &ISoundListener3::getForwardVector )
                    .def( "setVelocity", &ISoundListener3::setVelocity )
                    .def( "getVelocity", &ISoundListener3::getVelocity )];

        module( L )[class_<ISoundPlayer, ISharedObject, SmartPtr<ISoundPlayer>>( "ISoundPlayer" )
                        .def( "play", &ISoundPlayer::play )
                        .def( "stop", &ISoundPlayer::stop )
                        .def( "pause", &ISoundPlayer::pause )
                        .def( "forward", &ISoundPlayer::forward )
                        .def( "rewind", &ISoundPlayer::rewind )
                        .def( "restart", &ISoundPlayer::restart )
                        .def( "skip", &ISoundPlayer::skip )
                        .def( "setRepeat", &ISoundPlayer::setRepeat )
                        .def( "getRepeat", &ISoundPlayer::getRepeat )
                        .def( "setBeepDelay", &ISoundPlayer::setBeepDelay )
                        .def( "getBeepDelay", &ISoundPlayer::getBeepDelay )
                        .def( "setVolume", &ISoundPlayer::setVolume )
                        .def( "getVolume", &ISoundPlayer::getVolume )
                        .def( "getTrackLength", &ISoundPlayer::getTrackLength )
                        .def( "getPlaybackTime", &ISoundPlayer::getPlaybackTime )
                        .def( "setPlaybackTime", &ISoundPlayer::setPlaybackTime )
                        .def( "setDelayTime", &ISoundPlayer::setDelayTime )
                        .def( "getDelayTime", &ISoundPlayer::getDelayTime )
                        .def( "getIdTag", &ISoundPlayer::getIdTag )];

        module( L )[class_<ISoundProject, ISharedObject, SmartPtr<ISoundProject>>( "ISoundProject" )
                        .def( "addSoundEvent", &ISoundProject::addSoundEvent )
                        .def( "removeSoundEvent", &ISoundProject::removeSoundEvent )
                        .def( "getMasterVolume", &ISoundProject::getMasterVolume )
                        .def( "setMasterVolume", &ISoundProject::setMasterVolume )
                        .def( "setupReverb", &ISoundProject::setupReverb )
                        .def( "getEnableReverb", &ISoundProject::getEnableReverb )
                        .def( "setEnableReverb", &ISoundProject::setEnableReverb )];
    }
} // namespace workphone
