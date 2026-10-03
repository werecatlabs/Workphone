#ifndef _IGraphicsWindowEvent_H
#define _IGraphicsWindowEvent_H

#include <Workphone/Interface/System/IEvent.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a window event.
         */
        class WPCore_API IGraphicsWindowEvent : public IEvent
        {
        public:
            IGraphicsWindowEvent();

            /** Virtual destructor. */
            ~IGraphicsWindowEvent() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
