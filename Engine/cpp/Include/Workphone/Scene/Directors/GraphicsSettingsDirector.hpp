#ifndef GraphicsSettingsDirector_h__
#define GraphicsSettingsDirector_h__

#include <Workphone/System/Director.hpp>
#include <Workphone/Graphics/GraphicsPipeline.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>

namespace workphone
{
    namespace scene
    {

        /** Graphics settings director implementation. */
        class WPCore_API GraphicsSettingsDirector : public Director
        {
        public:
            /** Constructor. */
            GraphicsSettingsDirector();

            /** Destructor. */
            ~GraphicsSettingsDirector() override;

            /** @copydoc IBuildDirector::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IBuildDirector::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;
            render::IGraphicsSystem::RenderApi getRenderApi() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<render::GraphicsPipeline> m_pipeline;
            s32 m_renderApi = 1;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // GraphicsSettingsDirector_h__
