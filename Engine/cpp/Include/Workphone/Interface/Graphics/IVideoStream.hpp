#ifndef IVideoStream_h__
#define IVideoStream_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IVideo.hpp>

namespace workphone
{
    namespace render
    {

        /** An interface for a video stream class. */
        class WPCore_API IVideoStream : public IVideo
        {
        public:
            /** Destructor. */
            ~IVideoStream() override;

            /** Add a frame to the video stream. */
            virtual void addFrame( SmartPtr<IStateMessage> message ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IVideoStream_h__
