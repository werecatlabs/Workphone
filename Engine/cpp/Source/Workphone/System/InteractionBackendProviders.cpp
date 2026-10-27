#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/InteractionBackendProviders.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <map>
#include <mutex>
#include <stdexcept>

namespace workphone
{
    namespace
    {
        std::mutex providerMutex;
        std::map<s32, String> audioProviders, inputProviders;
        String providerName( bool audio, s32 selection )
        {
            std::lock_guard<std::mutex> lock( providerMutex );
            const auto &providers = audio ? audioProviders : inputProviders;
            if( auto found = providers.find( selection ); found != providers.end() ) return found->second;
            if( audio ) return selection == 1 ? String( "FMODSoundManager" ) :
                selection == 2 ? String( "OpenALSoundManager" ) : String( "WPAudioManager" );
            return selection == 1 ? String( "SDLInputDeviceManager" ) : selection == 3 ?
                String( "HybridInputDeviceManager" ) : String( "InputDeviceManager" );
        }
        SmartPtr<IFactory> providerFactory( SmartPtr<IFactoryManager> factories, bool audio, s32 selection )
        {
            if( !factories || selection < 0 || selection > 3 ) return nullptr;
            auto factory = factories->getFactoryByName( providerName( audio, selection ) );
            if( factory && !factory->isObjectDerivedFromByInfo(
                audio ? ISoundManager::typeInfo() : IInputDeviceManager::typeInfo() ) ) return nullptr;
            return factory;
        }
    }
    void registerProjectSoundBackend( s32 selection, const String &factoryType )
    {
        if( selection < 0 || selection > 3 ) throw std::invalid_argument( "Invalid audio backend selection" );
        std::lock_guard<std::mutex> lock( providerMutex );
        if( factoryType.empty() ) audioProviders.erase( selection ); else audioProviders[selection] = factoryType;
    }
    void registerProjectInputBackend( s32 selection, const String &factoryType )
    {
        if( selection < 0 || selection > 3 ) throw std::invalid_argument( "Invalid input backend selection" );
        std::lock_guard<std::mutex> lock( providerMutex );
        if( factoryType.empty() ) inputProviders.erase( selection ); else inputProviders[selection] = factoryType;
    }
    bool hasProjectSoundBackend( SmartPtr<IFactoryManager> factories, s32 selection )
    { return providerFactory( factories, true, selection ) != nullptr; }
    bool hasProjectInputBackend( SmartPtr<IFactoryManager> factories, s32 selection )
    { return providerFactory( factories, false, selection ) != nullptr; }
    bool matchesProjectSoundBackend( SmartPtr<IFactoryManager> factories,
                                    SmartPtr<ISoundManager> instance, s32 selection )
    {
        auto factory = providerFactory( factories, true, selection );
        return instance && ( factory ? instance->derived( factory->getObjectTypeId() ) : selection == 0 );
    }
    bool matchesProjectInputBackend( SmartPtr<IFactoryManager> factories,
                                    SmartPtr<IInputDeviceManager> instance, s32 selection )
    {
        auto factory = providerFactory( factories, false, selection );
        return instance && ( factory ? instance->derived( factory->getObjectTypeId() ) : selection == 0 );
    }
    SmartPtr<ISoundManager> createProjectSoundBackend( SmartPtr<IFactoryManager> factories,
                                                     SmartPtr<Properties> profile )
    {
        const auto selection = profile ? profile->getPropertyAsInt( "sound.backend", 0 ) : 0;
        auto factory = providerFactory( factories, true, selection );
        SmartPtr<ISoundManager> sound;
        if( factory ) sound = dynamic_pointer_cast<ISoundManager>( factories->createById( factory->getObjectTypeId() ) );
        else if( selection == 0 ) sound = factories ? factories->make_object<ISoundManager>() : nullptr;
        if( !sound ) throw std::runtime_error( "The selected audio backend provider is not registered." );
        sound->setProperties( profile );
        return sound;
    }
    SmartPtr<IInputDeviceManager> createProjectInputBackend( SmartPtr<IFactoryManager> factories,
                                                           SmartPtr<Properties> profile )
    {
        const auto selection = profile ? profile->getPropertyAsInt( "input.system", 0 ) : 0;
        auto factory = providerFactory( factories, false, selection );
        SmartPtr<IInputDeviceManager> input;
        if( factory ) input = dynamic_pointer_cast<IInputDeviceManager>( factories->createById( factory->getObjectTypeId() ) );
        else if( selection == 0 ) input = factories ? factories->make_object<IInputDeviceManager>() : nullptr;
        if( !input ) throw std::runtime_error( "The selected input backend provider is not registered." );
        input->setProperties( profile );
        return input;
    }
}
