#include <EditorPCH.hpp>
#include <commands/EditCommandsUtil.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>

namespace workphone::editor
{
    void prepareActorDataForDuplicate( SmartPtr<Properties> actorData )
    {
        if( !actorData )
        {
            return;
        }

        String value;
        const bool isActorData = actorData->getPropertyValue( scene::GameActorUtil::labelStr, value ) ||
                                 actorData->getPropertyValue( scene::GameActorUtil::uuidStr, value );
        if( isActorData )
        {
            actorData->removeProperty( scene::GameActorUtil::uuidStr );

            if( actorData->getPropertyValue( scene::GameActorUtil::labelStr, value ) && !value.empty() )
            {
                actorData->setProperty( scene::GameActorUtil::labelStr, value + " (Copy)" );
            }
        }

        const auto childrenData = actorData->getChildrenByName( scene::GameActorUtil::childStr );
        for( auto childData : childrenData )
        {
            prepareActorDataForDuplicate( childData );
        }
    }
}  // namespace workphone::editor
