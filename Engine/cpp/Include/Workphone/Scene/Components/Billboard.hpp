#ifndef Billboard_h__
#define Billboard_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{
    namespace scene
    {
        /** @brief Billboard component.
         *  Billboard component.
         */
        class WPCore_API Billboard : public Component
        {
        public:
            static const String TextureStr;
            static const String SizeStr;
            static const String ColorStr;

            /** @brief Constructor. */
            Billboard();

            /** @brief Destructor. */
            ~Billboard() override;

            /** @brief Load the component. */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @brief Unload the component. */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Get the component properties. */
            SmartPtr<Properties> getProperties() const override;

            /** @brief Set the component properties. */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @brief Get the texture. */
            SmartPtr<render::ITexture> getTexture() const;

            /** @brief Set the texture. */
            void setTexture( SmartPtr<render::ITexture> texture );

            /** @brief Get the size. */
            Vector2F getSize() const;

            /** @brief Set the size. */
            void setSize( const Vector2F &size );

            /** @brief Get the color. */
            ColourF getColor() const;

            /** @brief Set the color. */
            void setColor( const ColourF &color );

            /** @brief Update the billboard. */
            void updateBillboard();

            WP_CLASS_REGISTER_DECL;

        private:
            SmartPtr<render::IGraphicsSceneNode> m_sceneNode;
            SmartPtr<render::ITexture> m_texture;
            Vector2F m_size;
            ColourF m_color;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Billboard_h__
