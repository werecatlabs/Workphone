#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Ai/IAiManager.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IAiManager, ISharedObject );

    IAiManager::~IAiManager() = default;

    String IAiManager::query( const String &prompt, Provider provider ) const
    {
        return provider == Provider::Ollama ? query( prompt )
                                            : "This AI manager does not support OpenAI.";
    }

    String IAiManager::processResponseWithFeedback( const String &response ) const
    {
        return processResponse( response ) ? String( "Engine actions applied." )
                                           : String( "No engine actions were applied." );
    }

}  // namespace workphone
