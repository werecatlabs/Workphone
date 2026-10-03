#ifndef __AiTrack_h__
#define __AiTrack_h__

#include <Workphone/Interface/Ai/IAiTrack.hpp>

namespace workphone
{
    class WPCore_API AiTrack : public IAiTrack
    {
    public:
        AiTrack();
        ~AiTrack() override;

        void addElement( SmartPtr<IAiTrackElement> element ) override;

        void removeElement( size_t index ) override;

        SmartPtr<IAiTrackElement> getElement( size_t index ) const override;

        size_t getNumElements() const override;

        void clear() override;

        bool load( const std::string &filename ) override;

        bool save( const std::string &filename ) const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        Array<SmartPtr<IAiTrackElement>> m_elements;
    };

}  // namespace workphone

#endif  // __AiTrack_h__
