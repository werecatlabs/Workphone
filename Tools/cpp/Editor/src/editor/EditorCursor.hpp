#ifndef EditorCursor_H
#define EditorCursor_H

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Triangle3.hpp>

namespace workphone
{
    namespace editor
    {
        class EditorCursor : public ISharedObject
        {
        public:
            enum EditorCursorState
            {
                ECS_EDIT_NORMAL,
                ECS_EDIT_TERRAIN,
                ECS_EDIT_FOLIAGE,
                ECS_EDIT_CITY,

                ECS_COUNT
            };

            EditorCursor();
            ~EditorCursor() override;

            void update() override;

            void setState( u32 state );
            u32 getState() const;

            void setSize( f32 size );

        protected:
            Triangle3F m_curTriangle;

            DecalCursor *m_decalCursor = nullptr;

            f32 m_size = 0.0f;
            u32 m_state = 0;

            bool m_updateCursorPos = false;
        };
    }  // end namespace editor
}  // namespace workphone

#endif
