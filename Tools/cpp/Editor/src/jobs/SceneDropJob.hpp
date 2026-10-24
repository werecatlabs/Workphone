#ifndef SceneDropJob_h__
#define SceneDropJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace editor
    {
        class SceneDropJob : public Job
        {
        public:
            static const String resourceUUIDStr;
            static const String filePathStr;
            static const String prefabExtStr;

            SceneDropJob();
            ~SceneDropJob() override;

            void execute() override;

            String getData() const;

            void setData( const String &data );

            SmartPtr<ui::IUIElement> getDestination() const;
            void setDestination( SmartPtr<ui::IUIElement> destination );

            s32 getSiblingIndex() const;
            void setSiblingIndex( s32 siblingIndex );

            /** World ray captured over the render viewport when the asset is released. */
            void setDropRay( const Ray3<real_Num> &ray );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ui::IUIElement> m_destination;
            String m_data;
            s32 m_siblingIndex = -1;
            Ray3<real_Num> m_dropRay;
            bool m_hasDropRay = false;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // SceneDropJob_h__
