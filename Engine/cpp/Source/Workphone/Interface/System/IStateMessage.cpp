#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IStateMessage, ISharedObject );

    const hash_type IStateMessage::STATE_MESSAGE_LEFT = StringUtil::getHash( "left" );
    const hash_type IStateMessage::STATE_MESSAGE_TOP = StringUtil::getHash( "top" );
    const hash_type IStateMessage::STATE_MESSAGE_WIDTH = StringUtil::getHash( "width" );
    const hash_type IStateMessage::STATE_MESSAGE_HEIGHT = StringUtil::getHash( "height" );
    const hash_type IStateMessage::STATE_MESSAGE_METRICSMODE = StringUtil::getHash( "metricsmode" );
    const hash_type IStateMessage::STATE_MESSAGE_ALIGN_HORIZONTAL =
        StringUtil::getHash( "align_horizontal" );
    const hash_type IStateMessage::STATE_MESSAGE_ALIGN_VERTICAL =
        StringUtil::getHash( "align_vertical" );
    const hash_type IStateMessage::STATE_MESSAGE_TEXT = StringUtil::getHash( "text" );

    IStateMessage::~IStateMessage() = default;

}  // namespace workphone
