#ifndef StateFrameData_h__
#define StateFrameData_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    /**
     * @class StateFrameData
     * @brief Message carrying raw video and sound frame buffers for a single frame.
     *
     * This message is used to transport a video buffer and a sound buffer together
     * as part of the state messaging system. Both buffers are represented as raw
     * pointers and their sizes in bytes.
     *
     * Ownership semantics:
     * - This class stores raw pointers (`u8 *`) for buffers. It does not enforce
     *   any ownership or lifetime policy. The caller is responsible for ensuring
     *   that the pointed-to memory remains valid for the lifetime of the message
     *   (or until replaced).
     *
     * @ingroup Workphone_State_Messages
     */
    class WPCore_API StateFrameData : public StateMessage
    {
    public:
        /**
         * @brief Default constructor. Initializes buffers to null and sizes to 0.
         */
        StateFrameData();

        /**
         * @brief Construct a StateFrameData with expected buffer sizes.
         * @param videoBufferSize Size in bytes of the video buffer (may be 0).
         * @param soundBufferSize Size in bytes of the sound buffer (may be 0).
         *
         * Note: This constructor records the expected sizes. It does not allocate
         * or transfer ownership of any buffer memory by itself.
         */
        StateFrameData( s32 videoBufferSize, s32 soundBufferSize );

        /**
         * @brief Virtual destructor.
         *
         * Does not free buffer memory. See ownership note above.
         */
        ~StateFrameData() override;

        /**
         * @brief Get pointer to the video buffer (raw bytes).
         * @return Pointer to video buffer, or nullptr if not set.
         */
        u8 *getVideoBuffer() const;

        /**
         * @brief Set pointer to the video buffer.
         * @param value Pointer to the video buffer. May be nullptr to clear.
         *
         * Caller must ensure the buffer remains valid for the required lifetime.
         */
        void setVideoBuffer( u8 *value );

        /**
         * @brief Get the video buffer size in bytes.
         * @return Size in bytes (may be 0).
         */
        s32 getVideoBufferSize() const;

        /**
         * @brief Set the video buffer size in bytes.
         * @param value Size in bytes (should match the actual buffer length).
         */
        void setVideoBufferSize( s32 value );

        /**
         * @brief Get pointer to the sound buffer (raw bytes).
         * @return Pointer to sound buffer, or nullptr if not set.
         */
        u8 *getSoundBuffer() const;

        /**
         * @brief Set pointer to the sound buffer.
         * @param value Pointer to the sound buffer. May be nullptr to clear.
         *
         * Caller must ensure the buffer remains valid for the required lifetime.
         */
        void setSoundBuffer( u8 *value );

        /**
         * @brief Get the sound buffer size in bytes.
         * @return Size in bytes (may be 0).
         */
        s32 getSoundBufferSize() const;

        /**
         * @brief Set the sound buffer size in bytes.
         * @param value Size in bytes (should match the actual buffer length).
         */
        void setSoundBufferSize( s32 value );

        /** Macro used to declare runtime/reflection/registration boilerplate. */
        WP_CLASS_REGISTER_DECL;

    protected:
        /** Pointer to raw video frame bytes. May be nullptr. */
        u8 *m_videoBuffer = nullptr;

        /** Pointer to raw sound frame bytes. May be nullptr. */
        u8 *m_soundBuffer = nullptr;

        /** Size in bytes of the video buffer (videoBuffer). */
        s32 m_videoBufferSize = 0;

        /** Size in bytes of the sound buffer (soundBuffer). */
        s32 m_soundBufferSize = 0;
    };

}  // namespace workphone

#endif  // StateFrameData_h__
