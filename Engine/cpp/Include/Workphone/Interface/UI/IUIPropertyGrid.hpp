#ifndef IUIPropertyGrid_h__
#define IUIPropertyGrid_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class IUIPropertyGrid
         * @brief Interface for a UI Property Grid, extending IUIElement and providing functionality to
         * manage and display properties
         */
        class WPCore_API IUIPropertyGrid : public IUIElement
        {
        public:
            IUIPropertyGrid() : IUIElement( IUIPropertyGrid::typeInfo() )
            {
            }

            IUIPropertyGrid( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /**
             * @brief Virtual destructor
             */
            ~IUIPropertyGrid() override;

            /**
             * @brief Gets the data as a Properties object
             * @return A shared pointer to the Properties object
             */
            SmartPtr<Properties> getProperties() const override = 0;

            /**
             * @brief Sets the data as a Properties object
             * @param properties A shared pointer to the Properties object to be associated with the grid
             */
            void setProperties( SmartPtr<Properties> properties ) override = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // IUIPropertyGrid_h__
