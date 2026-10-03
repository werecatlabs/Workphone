#ifndef IMassData3_h__
#define IMassData3_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace physics
    {

        /// This holds the mass data computed for a shape.
        class WPCore_API IMassData3 : public ISharedObject
        {
        public:
            ~IMassData3() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IMassData3_h__
