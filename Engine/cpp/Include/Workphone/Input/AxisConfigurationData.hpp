#ifndef AxisConfigData_h__
#define AxisConfigData_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * @class AxisConfigurationData
     * @brief Stores calibration data for a single input axis.
     *
     * AxisConfigurationData holds the configuration values used by the input
     * pipeline to map raw axis readings to engine-level channels. It carries
     * the database-derived fields (offsets, multipliers, throw limits, channel
     * metadata) along with the runtime fields used while the user is
     * calibrating a stick (real, progress and display values).
     *
     * All members are public plain data.
     */
    class AxisConfigurationData : public ISharedObject
    {
    public:
        /**
         * @brief Constructs an uncalibrated axis configuration.
         *
         * Only the throw limits and throw multipliers are initialised
         * (`max_throw` to the smallest positive float, `min_throw` to the largest float and both
         * multipliers to 1). All other members are left uninitialised and must be assigned by
         * the caller.
         */
        AxisConfigurationData();

        /**
         * @brief Constructs an axis configuration from database values.
         *
         * The runtime values (`realValue`, `progressValue`, `displayValue`, `newOffset`) are
         * set to zero, `color` defaults to "#4D8325", and the throw limits and throw
         * multipliers are initialised as in the default constructor.
         *
         * @param id Database identifier of this axis configuration.
         * @param channel_number Engine channel this axis is mapped to.
         * @param cmap Channel map identifier.
         * @param output Output channel the processed value is written to.
         * @param db_offset Stored calibration offset.
         * @param db_multiplier Stored calibration multiplier.
         * @param reverse True if the axis direction is inverted.
         * @param txMake Make/model of the transmitter providing this axis.
         * @param stickDirectionLabel Display label for the stick direction (e.g. "Throttle").
         */
        AxisConfigurationData( s32 id, s32 channel_number, s32 cmap, s32 output, f32 db_offset,
                               f32 db_multiplier, bool reverse, String txMake,
                               String stickDirectionLabel );

        /** @brief Destructor. */
        ~AxisConfigurationData() override;

        /// Database identifier of this axis configuration.
        s32 id;

        /// Engine channel number this axis is mapped to.
        s32 channel_number;

        /// Channel map identifier.
        s32 cmap;

        /// Latest raw value read from the device (runtime, calibration only).
        s32 realValue;

        /// Value tracking the calibration progress (runtime, calibration only).
        s32 progressValue;

        /// Value shown to the user in the calibration display (runtime only).
        s32 displayValue;

        /// Output channel the processed value is written to.
        s32 output;

        /// Stored calibration offset applied to the raw value.
        f32 offset;

        /// Offset computed during an in-progress calibration; replaces `offset` once accepted.
        f32 newOffset;

        /// Calibration multiplier applied to the raw value.
        f32 multiplier;

        /// Maximum throw observed on the axis during calibration.
        f32 max_throw;

        /// Minimum throw observed on the axis during calibration.
        f32 min_throw;

        /// Scale factor applied to the positive side of the axis (default 1).
        f32 max_throw_multiplier;

        /// Scale factor applied to the negative side of the axis (default 1).
        f32 min_throw_multiplier;

        /// True if the axis direction is inverted.
        bool reverse;

        /// Current value of the axis formatted for display.
        String value;

        /// Make/model of the transmitter providing this axis.
        String txMake;

        /// Display label for the stick direction (e.g. "Throttle").
        String stickDirectionLabel;

        /// Display colour of the axis as an HTML hex string (e.g. "#4D8325").
        String color;

        /// Name of the device or control scheme this axis emulates, if any.
        String emulation;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // AxisConfigData_h__
