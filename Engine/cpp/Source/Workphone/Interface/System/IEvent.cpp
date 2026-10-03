#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IEvent, ISharedObject );

    const hash_type IEvent::loadingStateChanged = StringUtil::getHash( "loadingStateChanged" );
    const hash_type IEvent::loadScene = StringUtil::getHash( "loadScene" );
    const hash_type IEvent::unloadScene = StringUtil::getHash( "unloadScene" );
    const hash_type IEvent::handlePropertyButtonClick =
        StringUtil::getHash( "handlePropertyButtonClick" );

    const hash_type IEvent::handleTreeSelectionActivated =
        StringUtil::getHash( "handleTreeSelectionActivated" );
    const hash_type IEvent::handleTreeSelectionRelease =
        StringUtil::getHash( "handleTreeSelectionRelease" );
    const hash_type IEvent::handleTreeNodeDoubleClicked =
        StringUtil::getHash( "handleTreeNodeDoubleClicked" );

    const hash_type IEvent::actorLoaded = StringUtil::getHash( "actorLoaded" );
    const hash_type IEvent::actorUnloaded = StringUtil::getHash( "actorUnloaded" );

    const hash_type IEvent::componentLoaded = StringUtil::getHash( "componentLoaded" );
    const hash_type IEvent::componentUnloaded = StringUtil::getHash( "componentUnloaded" );

    const hash_type IEvent::addActor = StringUtil::getHash( "addActor" );
    const hash_type IEvent::removeActor = StringUtil::getHash( "removeActor" );
    const hash_type IEvent::enabled = StringUtil::getHash( "enabled" );
    const hash_type IEvent::sceneChanged = StringUtil::getHash( "sceneChanged" );

    const hash_type IEvent::createUI = StringUtil::getHash( "createUI" );
    const hash_type IEvent::destroyUI = StringUtil::getHash( "destroyUI" );

    const hash_type IEvent::handleWindowClose = StringUtil::getHash( "handleWindowClose" );
    const hash_type IEvent::handleWindowResize = StringUtil::getHash( "handleWindowResize" );
    const hash_type IEvent::handleWindowMove = StringUtil::getHash( "handleWindowMove" );

    const hash_type IEvent::handlePropertyChanged = StringUtil::getHash( "handlePropertyChanged" );
    const hash_type IEvent::handleValueChanged = StringUtil::getHash( "handleValueChanged" );

    const hash_type IEvent::handleMouseClicked = StringUtil::getHash( "handleMouseClicked" );
    const hash_type IEvent::handleMouseReleased = StringUtil::getHash( "handleMouseReleased" );

    const hash_type IEvent::handleSelection = StringUtil::getHash( "handleSelection" );
    const hash_type IEvent::handleToggle = StringUtil::getHash( "handleToggle" );

    const hash_type IEvent::handleDrop = StringUtil::getHash( "handleDrop" );
    const hash_type IEvent::handleDrag = StringUtil::getHash( "handleDrag" );

    const hash_type IEvent::inputEvent = StringUtil::getHash( "inputEvent" );
    const hash_type IEvent::updateEvent = StringUtil::getHash( "updateEvent" );

    const hash_type IEvent::handleEnterFrame = StringUtil::getHash( "handleEnterFrame" );

    const hash_type IEvent::CLICK_HASH = StringUtil::getHash( "click" );
    const hash_type IEvent::ACTIVATE_HASH = StringUtil::getHash( "activate" );
    const hash_type IEvent::UPDATE_HASH = StringUtil::getHash( "update" );
    const hash_type IEvent::HANDLE_MESSAGE_HASH = StringUtil::getHash( "handleMessage" );
    const hash_type IEvent::INITIALISE_START_HASH = StringUtil::getHash( "initialiseStart" );
    const hash_type IEvent::INITIALISE_END_HASH = StringUtil::getHash( "initialiseEnd" );

    const hash_type IEvent::ADD_CHILD_HASH = StringUtil::getHash( "addChild" );
    const hash_type IEvent::REMOVE_CHILD_HASH = StringUtil::getHash( "removeChild" );

    const hash_type IEvent::CHANGED_STATE_HASH = StringUtil::getHash( "changedState" );
    const hash_type IEvent::CHILD_CHANGED_STATE_HASH = StringUtil::getHash( "childChangedState" );

    const hash_type IEvent::TOGGLE_ENABLED_HASH = StringUtil::getHash( "toggleEnabled" );

    const hash_type IEvent::TOGGLE_HIGHLIGHT_HASH = StringUtil::getHash( "toggleHighlight" );

    const hash_type IEvent::VISIBLE_HASH = StringUtil::getHash( "visible" );

    const hash_type IEvent::SHOW_HASH = StringUtil::getHash( "show" );
    const hash_type IEvent::HIDE_HASH = StringUtil::getHash( "hide" );

    const hash_type IEvent::DEACTIVATE_HASH = StringUtil::getHash( "deactivate" );

    const hash_type IEvent::SELECT_HASH = StringUtil::getHash( "select" );
    const hash_type IEvent::DESELECT_HASH = StringUtil::getHash( "deselect" );

    const hash_type IEvent::GAIN_FOCUS_HASH = StringUtil::getHash( "gainFocus" );
    const hash_type IEvent::LOST_FOCUS_HASH = StringUtil::getHash( "lostFocus" );
    const hash_type IEvent::childChangedState = StringUtil::getHash( "childChangedState" );

    const hash_type IEvent::materialSetup = StringUtil::getHash( "materialSetup" );
    const hash_type IEvent::contactStart = StringUtil::getHash( "contactStart" );
    const hash_type IEvent::contactEnd = StringUtil::getHash( "contactEnd" );
    const hash_type IEvent::contactBreak = StringUtil::getHash( "contactBreak" );
    const hash_type IEvent::noContact = StringUtil::getHash( "noContact" );
    const hash_type IEvent::contact = StringUtil::getHash( "contact" );
    const hash_type IEvent::contactProcess = StringUtil::getHash( "contactProcess" );

    const hash_type IEvent::queued = StringUtil::getHash( "queued" );
    const hash_type IEvent::execute = StringUtil::getHash( "execute" );
    const hash_type IEvent::completed = StringUtil::getHash( "completed" );

    const hash_type IEvent::stateChanged = StringUtil::getHash( "stateChanged" );
    const hash_type IEvent::stateMessage = StringUtil::getHash( "stateMessage" );

    const hash_type IEvent::transform = StringUtil::getHash( "transform" );

    const hash_type IEvent::addSelectedObject = StringUtil::getHash( "addSelectedObject" );
    const hash_type IEvent::addSelectedObjects = StringUtil::getHash( "addSelectedObjects" );
    const hash_type IEvent::deselectObjects = StringUtil::getHash( "deselectObjects" );
    const hash_type IEvent::deselectAll = StringUtil::getHash( "deselectAll" );

    const hash_type IEvent::addCommand = StringUtil::getHash( "addCommand" );
    const hash_type IEvent::getNextCommand = StringUtil::getHash( "getNextCommand" );
    const hash_type IEvent::getPreviousCommand = StringUtil::getHash( "getPreviousCommand" );

    const hash_type IEvent::refreshAll = StringUtil::getHash( "refreshAll" );
    const hash_type IEvent::refreshPath = StringUtil::getHash( "refreshPath" );

    const hash_type IEvent::addUIElement = StringUtil::getHash( "addUIElement" );
    const hash_type IEvent::removeUIElement = StringUtil::getHash( "removeUIElement" );
    const hash_type IEvent::transformUIElement = StringUtil::getHash( "transformUIElement" );

    const hash_type IEvent::windowMovedOrResized = StringUtil::getHash( "windowMovedOrResized" );

    const hash_type IEvent::renderTargetTextureLoaded =
        StringUtil::getHash( "renderTargetTextureLoaded" );
    const hash_type IEvent::renderTargetTextureUnloaded =
        StringUtil::getHash( "renderTargetTextureUnloaded" );

    const hash_type IEvent::meshLoaded = StringUtil::getHash( "meshLoaded" );
    const hash_type IEvent::meshesImported = StringUtil::getHash( "meshesImported" );

    const hash_type IEvent::cameraManagerReset = StringUtil::getHash( "cameraManagerReset" );

    const hash_type IEvent::fileAction = StringUtil::getHash( "fileAction" );

    const hash_type IEvent::jobStarted = StringUtil::getHash( "jobStarted" );
    const hash_type IEvent::jobCompleted = StringUtil::getHash( "jobCompleted" );

    const hash_type IEvent::addChild = StringUtil::getHash( "addChild" );
    const hash_type IEvent::removeChild = StringUtil::getHash( "removeChild" );
    const hash_type IEvent::changedState = StringUtil::getHash( "changedState" );
    const hash_type IEvent::toggleEnabled = StringUtil::getHash( "toggleEnabled" );
    const hash_type IEvent::toggleVisibility = StringUtil::getHash( "toggleVisibility" );
    const hash_type IEvent::toggleHighlight = StringUtil::getHash( "toggleHighlight" );

    IEvent::IEvent() : ISharedObject( IEvent::typeInfo() )
    {
    }

    IEvent::IEvent( u32 poolTypeId ) : ISharedObject( poolTypeId )
    {
    }

    IEvent::~IEvent() = default;

    SmartPtr<ISharedObject> IEvent::getTarget() const
    {
        return m_target.load();
    }

    void IEvent::setTarget( SmartPtr<ISharedObject> target )
    {
        m_target = target;
    }

    bool IEvent::isTarget( SmartPtr<ISharedObject> object ) const
    {
        auto target = getTarget();
        return target && target == object;
    }

}  // namespace workphone
