#include <EditorPCH.hpp>
#include "EditorCursor.hpp"

namespace workphone::editor
{
    EditorCursor::EditorCursor() = default;

    EditorCursor::~EditorCursor() = default;

    void EditorCursor::update()
    {
    }

    void EditorCursor::setState( u32 state )
    {
        m_state = state;
    }

    u32 EditorCursor::getState() const
    {
        return m_state;
    }

    void EditorCursor::setSize( f32 size )
    {
        m_size = size;
    }
}  // namespace workphone::editor
