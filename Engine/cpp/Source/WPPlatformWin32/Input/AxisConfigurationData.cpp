#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Input/AxisConfigurationData.hpp>
#include <limits>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AxisConfigurationData, ISharedObject );

    AxisConfigurationData::AxisConfigurationData()
    {
        this->max_throw = std::numeric_limits<float>::min();
        this->min_throw = std::numeric_limits<float>::max();
        this->max_throw_multiplier = 1.0f;
        this->min_throw_multiplier = 1.0f;
    }

    AxisConfigurationData::AxisConfigurationData( s32 id, s32 channel_number, s32 cmap, s32 output,
                                                  f32 db_offset, f32 db_multiplier, bool reverse,
                                                  String txMake, String stickDirectionLabel )
    {
        this->id = id;
        this->channel_number = channel_number;
        this->cmap = cmap;
        this->realValue = 0;
        this->progressValue = 0;
        this->displayValue = 0;
        this->output = output;
        this->color = "#4D8325";
        this->offset = db_offset;
        newOffset = 0.0f;
        this->multiplier = db_multiplier;
        this->reverse = reverse;
        this->max_throw = std::numeric_limits<float>::min();
        this->min_throw = std::numeric_limits<float>::max();
        this->max_throw_multiplier = 1.0f;
        this->min_throw_multiplier = 1.0f;
        this->txMake = txMake;
        this->stickDirectionLabel = stickDirectionLabel;
    }

    AxisConfigurationData::~AxisConfigurationData()
    {
    }
}  // namespace workphone
