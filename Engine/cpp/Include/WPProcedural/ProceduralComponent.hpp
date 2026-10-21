#ifndef ProceduralComponent_h__
#define ProceduralComponent_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Core/Array.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace workphone
{
    namespace procedural
    {

        class ProceduralComponent : public scene::Component
        {
        public:
            ProceduralComponent();
            virtual ~ProceduralComponent();

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            SmartPtr<ISharedObject> getObject() const;

            void setObject( SmartPtr<ISharedObject> object );

            WP_CLASS_REGISTER_DECL;

        protected:
            FSMReturnType handleComponentEvent(u32 state, FSMEvent eventType);

            atomic_s64 m_objectType = 0;
            AtomicSmartPtr<ISharedObject> m_object;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // ProceduralComponent_h__
