#ifndef InputFunction_h__
#define InputFunction_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

/**
 * @file InputFunction.hpp
 * @brief Named input function (axis or button) with its mapping id and runtime channel state.
 */

namespace workphone
{

    /**
     * @class InputFunction
     * @brief A named input function and its current runtime state.
     *
     * InputFunction stores a function name together with its hash, whether it is an axis and/or a
     * button, the id of the mapping it belongs to, whether its value is reversed, and the current
     * and previous channel values. The time of the last value change is tracked so callers can
     * query how long the value has been stable.
     *
     * Defaults: not an axis, a button, not reversed, map id -1, channel values 0.
     */
    class InputFunction : public ISharedObject
    {
    public:
        /**
         * @brief Constructs an InputFunction with default values (button, not axis, not reversed,
         *        map id -1, channel values 0).
         */
        InputFunction();

        /**
         * @brief Virtual destructor.
         */
        ~InputFunction() override;

        /**
         * @brief Gets the function name.
         *
         * @return The function name; empty if none has been set.
         */
        String getFunction() const;

        /**
         * @brief Sets the function name and updates the function hash to match.
         *
         * @param functionName Name to set; its hash becomes the new function hash.
         */
        void setFunction( const String &functionName );

        /**
         * @brief Queries whether this function is an axis input.
         *
         * @return true if axis, false otherwise
         */
        bool isAxis() const;

        /**
         * @brief Marks this function as an axis input (or not).
         *
         * @param isAxisInput true to mark as axis
         * @note Independent of setButton(); the two flags are not kept mutually exclusive.
         */
        void setAxis( bool isAxisInput );

        /**
         * @brief Queries whether this function is a button input.
         *
         * @return true if button (the default), false otherwise
         */
        bool getButton() const;

        /**
         * @brief Marks this function as a button input (or not).
         *
         * @param isButtonInput true to mark as button
         * @note Independent of setAxis(); the two flags are not kept mutually exclusive.
         */
        void setButton( bool isButtonInput );

        /**
         * @brief Gets the id of the mapping associated with this function.
         *
         * @return The map id, or -1 if none has been set
         */
        s32 getMapId() const;

        /**
         * @brief Sets the id of the mapping associated with this function.
         *
         * @param mapId Map identifier
         */
        void setMapId( s32 mapId );

        /**
         * @brief Gets the hash of the function name.
         *
         * @return The function hash
         * @note The value is only meaningful after setFunction() or setFunctionHash() has been
         *       called; the constructor does not initialize it.
         */
        hash_type getFunctionHash() const;

        /**
         * @brief Sets the function hash directly.
         *
         * @param hashValue Hash value to set
         * @note setFunction() overwrites this with the hash of the new name.
         */
        void setFunctionHash( hash_type hashValue );

        /**
         * @brief Queries whether the function is reversed (inverted).
         *
         * @return true if reversed
         */
        bool isReversed() const;

        /**
         * @brief Sets whether the function is reversed (inverted).
         *
         * @param isReversedInput true to mark the function as reversed
         * @note This only stores the flag; it does not modify the stored channel values.
         */
        void setReversed( bool isReversedInput );

        /**
         * @brief Gets the current channel value.
         *
         * @return The current value (typically normalized; button state for buttons)
         */
        f32 getChannelValue() const;

        /**
         * @brief Sets the current channel value.
         *
         * When an application manager exists, the value is only updated if it differs from the
         * current one; in that case the previous value is set to the old current value and the
         * time of change is set to the timer's current time. Without an application manager the
         * previous/current values are always updated and the time of change is not touched.
         *
         * @param channelValue New channel value
         */
        void setChannelValue( f32 channelValue );

        /**
         * @brief Gets the previous channel value (the value before the last change).
         *
         * @return The previous channel value
         */
        f32 getPrevChannelValue() const;

        /**
         * @brief Sets the previous channel value directly (e.g. for edge detection).
         *
         * @param previousChannelValue Previous channel value
         */
        void setPrevChannelValue( f32 previousChannelValue );

        /**
         * @brief Gets the time elapsed since the channel value last changed.
         *
         * @return Elapsed time (timer units), or 0 if there is no application manager or timer
         */
        time_interval timeSinceChange() const;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Timer time of the last channel value change.
        time_interval m_timeChanged;

        /// Hash of the function name.
        hash_type m_functionHash;

        /// Id of the mapping this function belongs to (-1 if none).
        s32 m_mapId;
        /// Current channel value (normalized axis value or button state).
        f32 m_channelValue;
        /// Channel value before the last change.
        f32 m_prevChannelValue;

        /// Whether this function is an axis input.
        bool m_isAxis;
        /// Whether this function is a button input.
        bool m_isButton;
        /// Whether the function is reversed (inverted).
        bool m_isReversed;

        /// Function name.
        String m_function;
    };

}  // namespace workphone

#endif  // InputFunction_h__
