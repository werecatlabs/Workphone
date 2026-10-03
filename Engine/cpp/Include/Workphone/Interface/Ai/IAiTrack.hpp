#ifndef ITrack_h__
#define ITrack_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <vector>
#include <string>

namespace workphone
{
    /**
     * Interface for an ai agent to store and
     * load track data and provide logic to aid tracking.
     */
    class WPCore_API IAiTrack : public ISharedObject
    {
    public:
        ~IAiTrack() override;

        // Add a track element
        virtual void addElement( SmartPtr<IAiTrackElement> element ) = 0;

        // Remove a track element by index
        virtual void removeElement( size_t index ) = 0;

        // Get a track element by index
        virtual SmartPtr<IAiTrackElement> getElement( size_t index ) const = 0;

        // Get the number of elements
        virtual size_t getNumElements() const = 0;

        // Clear all elements
        virtual void clear() = 0;

        // Load track from file
        virtual bool load( const std::string &filename ) = 0;

        // Save track to file
        virtual bool save( const std::string &filename ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ITrack_h__
