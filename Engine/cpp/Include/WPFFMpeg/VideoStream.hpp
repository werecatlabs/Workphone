#ifndef VideoStream_h__
#define VideoStream_h__

#include "Workphone/Interface/Graphics/IVideoStream.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    class VideoStream : public render::IVideoStream
    {
    public:
        VideoStream();
        ~VideoStream();

    protected:
    };

}  // namespace workphone

#endif  // VideoStream_h__
