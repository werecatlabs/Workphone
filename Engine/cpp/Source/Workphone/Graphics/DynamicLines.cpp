#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/DynamicLines.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, DynamicLines, GraphicsObject<IDynamicLines> );

    DynamicLines::DynamicLines() = default;
    DynamicLines::~DynamicLines() = default;

    void DynamicLines::addPoint( const Vector3<real_Num> &point )
    {
        m_points.push_back( point );
        setDirty();
    }

    void DynamicLines::setPoint( u32 index, const Vector3<real_Num> &point )
    {
        if( index < m_points.size() )
        {
            m_points[index] = point;
            setDirty();
        }
    }

    Vector3<real_Num> DynamicLines::getPoint( u32 index ) const
    {
        if( index < m_points.size() )
        {
            return m_points[index];
        }
        return Vector3<real_Num>::zero();
    }

    u32 DynamicLines::getNumPoints() const
    {
        return static_cast<u32>( m_points.size() );
    }

    void DynamicLines::clear()
    {
        m_points.clear();
        setDirty();
    }

    void DynamicLines::setDirty()
    {
        m_dirty = true;
    }

    void DynamicLines::setOperationType( u32 opType )
    {
        m_operationType = opType;
        setDirty();
    }

    u32 DynamicLines::getOperationType() const
    {
        return m_operationType;
    }

    SmartPtr<IGraphicsObject> DynamicLines::clone(
        const String &name /*= StringUtil::EmptyString */ ) const
    {
        auto newLines = workphone::make_ptr<DynamicLines>();
        newLines->m_points = m_points;
        newLines->m_operationType = m_operationType;
        // Optionally set name if needed
        return newLines;
    }

    void DynamicLines::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = nullptr;
        }
    }
}  // namespace workphone::render
