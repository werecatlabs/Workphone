#ifndef ISkySphere_h__
#define ISkySphere_h__

#include <Workphone/Interface/Graphics/ISky.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Represents a sky sphere.
         */
        class WPCore_API ISkySphere : public ISky
        {
        public:
            /** Virtual destructor. */
            ~ISkySphere() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ISkySphere_h__
