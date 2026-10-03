#ifndef IUIDropTarget_h__
#define IUIDropTarget_h__

#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIDropTarget
         * @brief Interface for a drop target, responsible for handling drop events in the user interface
         */
        class WPCore_API IUIDropTarget : public IEventListener
        {
        public:
            /**
             * @brief Virtual destructor
             */
            ~IUIDropTarget() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIDropTarget_h__
