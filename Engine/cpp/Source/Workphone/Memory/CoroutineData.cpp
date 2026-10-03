#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Memory/CoroutineData.hpp>

namespace workphone
{

    CoroutineData::CoroutineData( RawPtr<IObject> pObject ) : m_object( pObject )
    {
        m_lineNumber = 0;
    }

    CoroutineData::CoroutineData()
    {
        m_lineNumber = 0;
    }

    CoroutineData::~CoroutineData() = default;

    void CoroutineData::operator()()
    {
        WP_ASSERT( false );  // please use macro yield
    }

    auto CoroutineData::getLineNumber() const -> s32
    {
        return m_lineNumber;
    }

    void CoroutineData::setLineNumber( s32 lineNumber )
    {
        WP_ASSERT( lineNumber > m_lineNumber );
        m_lineNumber = lineNumber;
    }

    void CoroutineData::stop()
    {
    }

    void CoroutineData::yield()
    {
    }
}  // namespace workphone
