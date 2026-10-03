#ifndef AxisData_h__
#define AxisData_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @file AxisData.hpp
     * @brief Declaration of the AxisData class which encapsulates configuration for a single input axis.
     */

    /**
     * @class AxisData
     * @brief Represents configuration and metadata for a single controller/joystick axis.
     *
     * AxisData stores transformation parameters used to convert raw axis values into
     * application-level inputs: an offset, a global multiplier and separate low/high
     * multipliers that can be used to shape the response curve. It also stores mapping
     * information such as the source axis index, device mapping id and the associated
     * function name or function hash this axis is bound to.
     *
     * The object inherits from `ISharedObject` to allow shared ownership semantics
     * required by the system's memory/interface layer.
     */
    class AxisData : public ISharedObject
    {
    public:
        /**
         * @brief Constructs an AxisData instance with sensible defaults.
         *
         * Defaults:
         * - offset = 0.0f
         * - multiplier = 1.0f
         * - lowMultiplier = 1.0f
         * - highMultiplier = 1.0f
         * - axisIndex = 0
         * - deviceMap = 0
         * - isReversed = false
         */
        AxisData();

        /**
         * @brief Virtual destructor to allow proper cleanup in derived classes.
         */
        ~AxisData() override;

        /**
         * @brief Set a constant offset applied to the raw axis value before scaling.
         * @param offset Constant value added to raw input (units depend on input source).
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setOffset( f32 offset ) noexcept;

        /**
         * @brief Set the global multiplier applied to the axis value.
         * @param multiplier Scale factor applied after the offset.
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setMultiplier( f32 multiplier ) noexcept;

        /**
         * @brief Set a multiplier applied when the axis value is in the low range.
         * @param lowMultiplier Scale factor for the low-side of the response curve.
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setLowMultiplier( f32 lowMultiplier ) noexcept;

        /**
         * @brief Set a multiplier applied when the axis value is in the high range.
         * @param highMultiplier Scale factor for the high-side of the response curve.
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setHighMultiplier( f32 highMultiplier ) noexcept;

        /**
         * @brief Get the currently configured offset.
         * @return Current offset value.
         */
        f32 getOffset() const noexcept;

        /**
         * @brief Get the currently configured global multiplier.
         * @return Current multiplier.
         */
        f32 getMultiplier() const noexcept;

        /**
         * @brief Get the low-range multiplier.
         * @return Current low multiplier.
         */
        f32 getLowMultiplier() const noexcept;

        /**
         * @brief Get the high-range multiplier.
         * @return Current high multiplier.
         */
        f32 getHighMultiplier() const noexcept;

        /**
         * @brief Query whether the axis input is reversed.
         * @return true if the axis is reversed; false otherwise.
         */
        bool isReversed() const noexcept;

        /**
         * @brief Set whether the axis input should be reversed.
         * @param reversed Pass true to invert the axis sign.
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setReversed( bool reversed ) noexcept;

        /**
         * @brief Get the source axis index on the device this mapping refers to.
         * @return Index of the axis on the device.
         */
        s32 getAxisIndex() const noexcept;

        /**
         * @brief Set the source axis index on the device.
         * @param axisIndex Index of the axis on the device.
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setAxisIndex( s32 axisIndex ) noexcept;

        /**
         * @brief Get the device mapping identifier this axis belongs to.
         * @return Device map id (implementation-specific).
         */
        s32 getDeviceMap() const noexcept;

        /**
         * @brief Set the device mapping identifier for this axis.
         * @param deviceMap Device map id (implementation-specific).
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setDeviceMap( s32 deviceMap ) noexcept;

        /**
         * @brief Get the precomputed hash of the bound function name.
         * @return Hash value representing the input function.
         */
        hash_type getFunctionHash() const noexcept;

        /**
         * @brief Set the precomputed hash of the bound function name.
         * @param functionHash Hash value representing the input function.
         * @note This operation is noexcept (simple primitive assignment).
         */
        void setFunctionHash( hash_type functionHash ) noexcept;

        /**
         * @brief Get the textual function name this axis is bound to.
         *
         * The returned `String` holds the human-readable name used for debugging
         * and editor displays. The canonical identity used at runtime may be the
         * `functionHash`.
         *
         * @return Bound function name.
         */
        String getFunction() const;

        /**
         * @brief Set the textual function name this axis is bound to.
         * @param functionName Function name to bind; the class may also store a hash
         *                     representation separately.
         * @note May allocate / throw depending on String implementation.
         */
        void setFunction( const String &functionName );

    private:
        /** Hash of the bound function name used for fast lookup (0 if unset). */
        hash_type m_functionHash = 0;

        /** Constant offset applied to the raw axis input before scaling. */
        f32 m_offset = 0.0f;

        /** Global scale applied to the axis after offset. */
        f32 m_multiplier = 1.0f;

        /** Scale applied to the low portion of the response curve. */
        f32 m_lowMultiplier = 1.0f;

        /** Scale applied to the high portion of the response curve. */
        f32 m_highMultiplier = 1.0f;

        /** Index of the axis on the input device (0-based). */
        s32 m_axisIndex = 0;

        /** Device map identifier to distinguish input devices or profiles. */
        s32 m_deviceMap = 0;

        /** If true, the axis value will be inverted. */
        bool m_isReversed = false;

        /**
         * @brief Fixed-size container for the textual function name.
         *
         * A small fixed buffer (`FixedString<32>`) is used to avoid dynamic
         * allocations for short names while still providing a `String`-compatible
         * representation via `getFunction()`.
         */
        FixedString<32> m_function;
    };
}  // namespace workphone

#endif  // AxisData_h__
