#ifndef IAiSensoryMemory_h__
#define IAiSensoryMemory_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** A class that represents the memory of an AI agent.
     */
    class WPCore_API IAiSensoryMemory : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IAiSensoryMemory() override;

        virtual void updateMemory() = 0;

        virtual void addMemory( SmartPtr<ISharedObject> memory ) = 0;

        virtual void removeMemory( SmartPtr<ISharedObject> memory ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAiSensoryMemory_h__
