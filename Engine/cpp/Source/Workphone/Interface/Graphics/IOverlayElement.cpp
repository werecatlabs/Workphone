#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IOverlayElement.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IOverlayElement, ISharedObject );

    // Static property key strings
    const String IOverlayElement::materialNameStr = "materialName";
    const String IOverlayElement::colourStr = "colour";
    const String IOverlayElement::positionStr = "position";
    const String IOverlayElement::sizeStr = "size";
    const String IOverlayElement::zorderStr = "zorder";
    const String IOverlayElement::visibleStr = "visible";

    const hash_type IOverlayElement::STATE_MESSAGE_LEFT = StringUtil::getHash( "left" );

    const hash_type IOverlayElement::STATE_MESSAGE_TOP = StringUtil::getHash( "top" );

    const hash_type IOverlayElement::STATE_MESSAGE_WIDTH = StringUtil::getHash( "width" );

    const hash_type IOverlayElement::STATE_MESSAGE_HEIGHT = StringUtil::getHash( "height" );

    const hash_type IOverlayElement::STATE_MESSAGE_METRICSMODE = StringUtil::getHash( "metricsmode" );

    const hash_type IOverlayElement::STATE_MESSAGE_ALIGN_HORIZONTAL =
        StringUtil::getHash( "align_horizontal" );

    const hash_type IOverlayElement::STATE_MESSAGE_ALIGN_VERTICAL =
        StringUtil::getHash( "align_vertical" );

    const hash_type IOverlayElement::STATE_MESSAGE_TEXT = StringUtil::getHash( "text" );

    const hash_type IOverlayElement::STATE_MESSAGE_ADDCHILD = StringUtil::getHash( "addChild" );
    const hash_type IOverlayElement::STATE_MESSAGE_REMOVECHILD = StringUtil::getHash( "removeChild" );

    const hash_type IOverlayElement::STATE_MESSAGE_ATTACH_OBJECT = StringUtil::getHash( "attachObject" );
    const hash_type IOverlayElement::STATE_MESSAGE_DETACH_OBJECT = StringUtil::getHash( "detachObject" );
    const hash_type IOverlayElement::STATE_MESSAGE_DETACH_ALL_OBJECTS =
        StringUtil::getHash( "detachAllObject" );

    IOverlayElement::~IOverlayElement() = default;

}  // namespace workphone::render
