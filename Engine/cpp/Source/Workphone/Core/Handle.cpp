#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <sstream>

namespace workphone
{
    u32 Handle::m_idExt = 1;

    Handle::Handle() : m_hash( 0 )
    {
#if _DEBUG
        constexpr auto size = sizeof( Handle );
#endif

        m_id = m_idExt++;
        m_hash = m_id;
    }

    Handle::Handle( const Handle &other )
    {
        *this = other;
    }

    Handle::~Handle() = default;

    auto Handle::getHash() const -> hash64
    {
        return m_hash;
    }

    void Handle::setHash( hash64 hash )
    {
        m_hash = hash;
    }

    auto Handle::getUUIDAsString() const -> String
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );

        auto str = StringUtil::toString( m_uuid );
        return str;
    }

    auto Handle::getUUID() const -> UUID
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return m_uuid;
    }

    void Handle::setUUID( const String &uuid )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        WP_ASSERT( !StringUtil::isNullOrEmpty( uuid ) );
        m_uuid = StringUtil::parseUUID( uuid );
    }

    void Handle::setUUID( const UUID &uuid )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        m_uuid = uuid;
    }

    auto Handle::operator==( const Handle &b ) -> bool
    {
        return m_hash == b.m_hash;
    }

    hash_type Handle::getId() const
    {
        return m_id;
    }

    void Handle::setId( hash_type id )
    {
        m_id = static_cast<hash64>( id );
    }

    void Handle::setInstanceId( u32 instanceId )
    {
        m_instanceId = instanceId;
    }

    auto Handle::operator=( const Handle &other ) -> Handle &
    {
        SpinRWMutex::ScopedLock lockA( m_mutex, true );
        SpinRWMutex::ScopedLock lockB( other.m_mutex, false );

        m_id = other.m_id;
        m_instanceId = other.m_instanceId;
        m_hash = other.m_hash;
        m_uuid = other.m_uuid;
        return *this;
    }

    auto Handle::toString() const -> String
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );

        std::stringstream stream;
        stream << "m_hash: " << m_hash << "\n";
        return stream.str().c_str();
    }

    auto Handle::getClassId() const -> UUID
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return m_classId;
    }

    void Handle::setClassId( const UUID &classId )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        m_classId = classId;
    }

    auto Handle::getFileId() const -> UUID
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return m_fileId;
    }

    void Handle::setFileId( const UUID &fileId )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        m_fileId = fileId;
    }

}  // namespace workphone
