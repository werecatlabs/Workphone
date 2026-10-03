#ifndef FrameGrabberStandard_h__
#define FrameGrabberStandard_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IFrameGrabber.hpp>

namespace workphone
{
    /**
     * @class FrameGrabberStandard
     * @brief Default implementation of the IFrameGrabber interface.
     *
     * The FrameGrabberStandard collects, stores and provides access to frame
     * messages (IStateMessage) produced by various producers. It also maintains
     * an optional video stream used for rendering or streaming captured frames.
     *
     * Usage notes:
     * - Call @c addFrame(...) to push a captured frame into the grabber.
     * - Call @c popFrame() to retrieve and remove the next available frame.
     * - Call @c update() regularly (e.g. per-frame) to allow internal processing.
     *
     * Thread-safety:
     * - Thread-safety is not guaranteed by this class unless specified by the
     *   wider application. Synchronization should be provided by the caller if
     *   frames are produced and consumed from different threads.
     */
    class WPCore_API FrameGrabberStandard : public IFrameGrabber
    {
    public:
        /**
         * @brief Constructs a new FrameGrabberStandard instance.
         *
         * Initializes internal state. Does not start any background threads.
         */
        FrameGrabberStandard();

        /**
         * @brief Destructor.
         *
         * Cleans up resources held by the frame grabber. Any outstanding frames
         * or the associated video stream will be released according to the
         * SmartPtr semantics used by the engine.
         */
        ~FrameGrabberStandard() override;

        /**
         * @brief Perform periodic processing for the frame grabber.
         *
         * This method should be called regularly (for example once per engine
         * frame) to allow the grabber to process queued frames, update the
         * associated video stream, or perform bookkeeping.
         */
        void update() override;

        /**
         * @brief Add a frame message to the grabber's queue.
         *
         * The grabber takes ownership of the message via the SmartPtr. The
         * message typically contains the captured frame data and any metadata
         * required by consumers.
         *
         * @param message SmartPtr to the IStateMessage representing the frame.
         */
        void addFrame( SmartPtr<IStateMessage> message ) override;

        /**
         * @brief Pop the next available frame message.
         *
         * Retrieves and removes the oldest frame message from the grabber's
         * internal queue. If no frames are available, a null SmartPtr is
         * returned.
         *
         * @return SmartPtr<IStateMessage> The next frame message, or null if none.
         */
        SmartPtr<IStateMessage> popFrame() const override;

    protected:
        /**
         * @brief Optional video stream associated with the frame grabber.
         *
         * The video stream can be used to present or encode frames grabbed by
         * this object. The pointer is managed by SmartPtr; ownership semantics
         * follow the project's SmartPtr conventions (reference-counted).
         */
        SmartPtr<render::IVideoStream> m_videoStream;
    };
}  // namespace workphone

#endif  // FrameGrabberStandard_h__
