#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshPose.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MeshPose, IMeshPose );

    MeshPose::MeshPose( u16 target, const String &name ) :
        m_poseName( name ),
        m_target( target ),
        m_includesNormals( false )
    {
    }

    MeshPose::~MeshPose() = default;

    void MeshPose::unload( SmartPtr<ISharedObject> data )
    {
        // clear() retires nodes without allocating. The unordered-map sentinel
        // and bucket capacity remain charged until this live object is freed.
        m_vertexOffsets.clear();
        m_normalOffsets.clear();
        m_includesNormals = false;
    }

    u16 MeshPose::getTarget() const
    {
        return m_target;
    }

    void MeshPose::setTarget( u16 target )
    {
        m_target = target;
    }

    bool MeshPose::getIncludesNormals() const
    {
        return m_includesNormals;
    }

    void MeshPose::setIncludesNormals( bool includesNormals )
    {
        m_includesNormals = includesNormals;
    }

    void MeshPose::addVertex( u32 vertexIndex, const Vector3F &offset )
    {
        m_vertexOffsets[vertexIndex] = offset;
    }

    void MeshPose::addVertex( u32 vertexIndex, const Vector3F &offset, const Vector3F &normal )
    {
        auto position = m_vertexOffsets.find( vertexIndex );
        auto normals = m_normalOffsets.find( vertexIndex );
        if( position != m_vertexOffsets.end() && normals != m_normalOffsets.end() )
        {
            position->second = offset;
            normals->second = normal;
            m_includesNormals = true;
            return;
        }
        const auto fitsBuckets = []( const VertexOffsetMap &map, bool inserting ) {
            return !inserting || static_cast<long double>( map.size() + 1 ) <=
                static_cast<long double>( map.max_load_factor() ) * map.bucket_count();
        };
        if( fitsBuckets( m_vertexOffsets, position == m_vertexOffsets.end() ) &&
            fitsBuckets( m_normalOffsets, normals == m_normalOffsets.end() ) )
        {
            // With bucket growth excluded, the only new storage is independent
            // nodes. A second-node denial can retire the first node exactly,
            // leaving existing values, buckets and node addresses untouched.
            bool insertedPosition = false;
            try
            {
                if( position == m_vertexOffsets.end() )
                {
                    auto inserted = m_vertexOffsets.emplace( vertexIndex, offset );
                    position = inserted.first;
                    insertedPosition = inserted.second;
                }
                if( normals == m_normalOffsets.end() )
                    normals = m_normalOffsets.emplace( vertexIndex, normal ).first;
            }
            catch( ... )
            {
                if( insertedPosition ) m_vertexOffsets.erase( position );
                throw;
            }
            position->second = offset;
            normals->second = normal;
            m_includesNormals = true;
            return;
        }
        // Both allocations and potential bucket growth must succeed before
        // either value or the includes-normals flag becomes visible.
        auto positions = m_vertexOffsets;
        auto normalOffsets = m_normalOffsets;
        positions[vertexIndex] = offset;
        normalOffsets[vertexIndex] = normal;
        m_vertexOffsets.swap( positions );
        m_normalOffsets.swap( normalOffsets );
        m_includesNormals = true;
    }

    u32 MeshPose::getNumVertexOffsets() const
    {
        return static_cast<u32>( m_vertexOffsets.size() );
    }

    const MeshPose::VertexOffsetMap &MeshPose::getVertexOffsets() const
    {
        return m_vertexOffsets;
    }

    const MeshPose::VertexOffsetMap &MeshPose::getNormalOffsets() const
    {
        return m_normalOffsets;
    }

    SmartPtr<IMeshPose> MeshPose::clone() const
    {
        auto clonePose = workphone::make_ptr<MeshPose>( m_target, m_poseName );
        clonePose->m_includesNormals = m_includesNormals;
        clonePose->m_vertexOffsets = m_vertexOffsets;
        clonePose->m_normalOffsets = m_normalOffsets;
        return clonePose;
    }

}  // namespace workphone
