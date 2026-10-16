#ifndef SceneDropJob_h__
#define SceneDropJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

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

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ui::IUIElement> m_destination;
            String m_data;
            s32 m_siblingIndex = -1;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // SceneDropJob_h__
