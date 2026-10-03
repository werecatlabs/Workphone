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
     */
    class AxisConfigurationData : public ISharedObject
    {
    public:
        AxisConfigurationData();

        AxisConfigurationData( s32 id, s32 channel_number, s32 cmap, s32 output, f32 db_offset,
                               f32 db_multiplier, bool reverse, String txMake,
                               String stickDirectionLabel );
        ~AxisConfigurationData() override;

        s32 id;
        s32 channel_number;
        s32 cmap;
        s32 realValue;
        s32 progressValue;
        s32 displayValue;
        s32 output;
        f32 offset;
        f32 newOffset;
        f32 multiplier;
        f32 max_throw;
        f32 min_throw;
        f32 max_throw_multiplier;
        f32 min_throw_multiplier;
        bool reverse;
        String value;
        String txMake;
        String stickDirectionLabel;
        String color;
        String emulation;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // AxisConfigData_h__
