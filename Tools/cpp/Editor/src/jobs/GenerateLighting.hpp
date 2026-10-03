#ifndef GenerateLighting_h__
#define GenerateLighting_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * Generate lighting for the scene.
         */
        class GenerateLighting : public Job
        {
        public:
            /** Constructor.
             */
            GenerateLighting();

            /** Destructor.
             */
            ~GenerateLighting() override;

            /** Execute the job.
             */
            void execute() override;

            SmartPtr<scene::IGameActor> getLightActor() const;

            void setLightActor( SmartPtr<scene::IGameActor> lightActor );

        protected:
            SmartPtr<scene::IGameActor> m_lightActor;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // GenerateLighting_h__
