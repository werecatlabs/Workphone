#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/AiTrack.hpp>
#include <Workphone/Interface/Ai/IAiTrackElement.hpp>
#include <fstream>
#include <stdexcept>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AiTrack, IAiTrack );

    AiTrack::AiTrack() = default;

    AiTrack::~AiTrack() = default;

    void AiTrack::addElement( SmartPtr<IAiTrackElement> element )
    {
        m_elements.push_back( element );
    }

    void AiTrack::removeElement( size_t index )
    {
        if( index < m_elements.size() )
        {
            m_elements.erase( m_elements.begin() + index );
        }
        // else: ignore or throw if you want strict behavior
    }

    SmartPtr<IAiTrackElement> AiTrack::getElement( size_t index ) const
    {
        if( index < m_elements.size() )
        {
            return m_elements[index];
        }
        return nullptr;
    }

    size_t AiTrack::getNumElements() const
    {
        return m_elements.size();
    }

    void AiTrack::clear()
    {
        m_elements.clear();
    }

    bool AiTrack::load( const std::string &filename )
    {
        std::ifstream file( filename );
        if( !file.is_open() )
            return false;
        m_elements.clear();
        std::string line;
        while( std::getline( file, line ) )
        {
            // TODO: Parse line into an IAiTrackElement
            // For now, just create a default AiTrackElement (stub)
            // auto element = make_ptr<AiTrackElement>();
            // m_elements.push_back(element);
        }
        return true;
    }

    bool AiTrack::save( const std::string &filename ) const
    {
        std::ofstream file( filename );
        if( !file.is_open() )
            return false;
        for( const auto &element : m_elements )
        {
            // TODO: Serialize element to string
            // file << element->toString() << std::endl;
            file << "ElementStub" << std::endl;
        }
        return true;
    }

}  // namespace workphone
