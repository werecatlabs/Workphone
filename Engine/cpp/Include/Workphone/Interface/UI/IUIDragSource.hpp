#ifndef IUIDragSource_h__
#define IUIDragSource_h__

#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class IUIDragSource
         * @brief Interface for a drag source, responsible for handling drag events in the user interface
         */
        class WPCore_API IUIDragSource : public IEventListener
        {
        public:
            /**
             * @brief Virtual destructor
             */
            ~IUIDragSource() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIDragSource_h__
