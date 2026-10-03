#ifndef Billboards_h__
#define Billboards_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Graphics/IBillboardSet.hpp>

namespace workphone
{
    namespace scene
    {
        /** @brief Billboards component.
         *  Billboards component.
         */
        class WPCore_API Billboards : public Component
        {
        public:
            /** @brief Constructor. */
            Billboards();

            /** @brief Destructor. */
            ~Billboards() override;

            /** @brief Load the component. */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @brief Unload the component. */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Update the component. */
            void update() override;

            /** @brief Get the billboard set. */
            SmartPtr<render::IBillboardSet> getBillboardSet() const;

            /** @brief Set the billboard set. */
            void setBillboardSet( SmartPtr<render::IBillboardSet> billboardSet );

            WP_CLASS_REGISTER_DECL;

        private:
            /** @brief The billboard set. */
            SmartPtr<render::IBillboardSet> m_billboardSet;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Billboards_h__
