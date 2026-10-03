#ifndef TestFixture_h__
#define TestFixture_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    class UnitTestsFixture : public ISharedObject
    {
    public:
        UnitTestsFixture();

        ~UnitTestsFixture();

        void update();
        void iterate();

        void unload( SmartPtr<ISharedObject> data );

        void createTasks();
    };

}  // namespace workphone

#endif  // TestFixture_h__
