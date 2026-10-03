#ifndef InputFunction_h__
#define InputFunction_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

/**
 * @file InputFunction.hpp
 * @brief Represents a mapped input function (axis/button) with runtime state.
 */

namespace workphone
{

    /**
     * @brief Holds information about an input function mapping and its state.
     *
     * InputFunction stores metadata about a named input function (its string
     * name and hash), whether it represents an axis or button, the mapping id,
     * current and previous channel values and whether the input is reversed.
     * It also tracks the time of the last change to allow queries for time
     * since the last change.
     */
    class InputFunction : public ISharedObject
    {
    public:
        /**
         * @brief Construct a new InputFunction with default values.
         */
        InputFunction();

        /**
         * @brief Virtual destructor.
         */
        ~InputFunction() override;

        /**
         * @brief Get the human-readable function name.
         *
         * @return String Function name
         */
        String getFunction() const;

        /**
         * @brief Set the human-readable function name.
         *
         * @param functionName Name to set
         */
        void setFunction( const String &functionName );

        /**
         * @brief Query whether this function represents an axis input.
         *
         * @return true if axis, false otherwise
         */
        bool isAxis() const;

        /**
         * @brief Mark this function as an axis input.
         *
         * @param isAxisInput true to mark as axis
         */
        void setAxis( bool isAxisInput );

        /**
         * @brief Query whether this function represents a button input.
         *
         * @return true if button, false otherwise
         */
        bool getButton() const;

        /**
         * @brief Mark this function as a button input.
         *
         * @param isButtonInput true to mark as button
         */
        void setButton( bool isButtonInput );

        /**
         * @brief Get the mapping id associated with this function.
         *
         * @return s32 Map identifier
         */
        s32 getMapId() const;

        /**
         * @brief Set the mapping id associated with this function.
         *
         * @param mapId Map identifier
         */
        void setMapId( s32 mapId );

        /**
         * @brief Get the hashed function identifier.
         *
         * @return hash_type Function hash
         */
        hash_type getFunctionHash() const;

        /**
         * @brief Set the hashed function identifier.
         *
         * @param hashValue Hash value to set
         */
        void setFunctionHash( hash_type hashValue );

        /**
         * @brief Query whether the function is reversed (inverted axis).
         *
         * @return true if reversed
         */
        bool isReversed() const;

        /**
         * @brief Set whether the function should be treated as reversed.
         *
         * @param isReversedInput true to invert axis values
         */
        void setReversed( bool isReversedInput );

        /**
         * @brief Get the current channel value for this function.
         *
         * @return f32 Current value (typically normalized)
         */
        f32 getChannelValue() const;

        /**
         * @brief Set the current channel value and update timeSinceChange.
         *
         * @param channelValue New channel value
         */
        void setChannelValue( f32 channelValue );

        /**
         * @brief Get the previous channel value (before last update).
         *
         * @return f32 Previous channel value
         */
        f32 getPrevChannelValue() const;

        /**
         * @brief Set the previous channel value (used for edge detection).
         *
         * @param previousChannelValue Previous channel value
         */
        void setPrevChannelValue( f32 previousChannelValue );

        /**
         * @brief Time interval since the channel last changed value.
         *
         * @return time_interval Duration since last change
         */
        time_interval timeSinceChange() const;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Timestamp of the last channel change.
        time_interval m_timeChanged;

        /// Hash value identifying the function name.
        hash_type m_functionHash;

        /// Mapping id used to reference configuration or control maps.
        s32 m_mapId;
        /// Current channel value (normalized axis or button state).
        f32 m_channelValue;
        /// Previous channel value recorded prior to the last change.
        f32 m_prevChannelValue;

        /// Whether this function represents an axis input.
        bool m_isAxis;
        /// Whether this function represents a button input.
        bool m_isButton;
        /// Whether axis values should be inverted.
        bool m_isReversed;

        /// Human-readable function name.
        String m_function;
    };

}  // namespace workphone

#endif  // InputFunction_h__
