#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Systems/UI/LayoutTransformSystem.hpp>
#include <Workphone/Scene/Components/UI/LayoutTransform.hpp>
#include <Workphone/Scene/Components/UI/Layout.hpp>
#include <Workphone/Scene/Components/UI/UIComponent.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>
#include <Workphone/State/States/UIAnchorStateData.hpp>
#include <Workphone/State/States/UILayoutStateData.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    namespace
    {
        const Vector2<real_Num> DefaultReferenceSize( 1920.0f, 1080.0f );

        Vector2<real_Num> makeReferenceSize( const Layout *layout )
        {
            if( layout )
            {
                const auto referenceSize = layout->getReferenceSize();
                const auto size = Vector2<real_Num>( static_cast<f32>( referenceSize.x ),
                                                     static_cast<f32>( referenceSize.y ) );
                if( size.x > 0.0f && size.y > 0.0f )
                {
                    return size;
                }
            }

            return DefaultReferenceSize;
        }

        Vector2<real_Num> safeDivide( const Vector2<real_Num> &value, const Vector2<real_Num> &divisor )
        {
            if( MathF::equals( divisor.x, 0.0f ) || MathF::equals( divisor.y, 0.0f ) )
            {
                return Vector2<real_Num>::zero();
            }

            return Vector2<real_Num>( value.x / divisor.x, value.y / divisor.y );
        }

        u32 getHierarchyDepth( const LayoutTransform *layoutTransform )
        {
            u32 depth = 0;

            if( layoutTransform )
            {
                auto actor = layoutTransform->getActor();
                while( actor )
                {
                    actor = actor->getParent();
                    ++depth;
                }
            }

            return depth;
        }

        LayoutTransform *findParentLayoutTransform( IGameActor *actor )
        {
            if( !actor )
            {
                return nullptr;
            }

            auto parent = actor->getParentPtr();
            while( parent )
            {
                if( auto parentLayoutTransform = parent->getComponentPtr<LayoutTransform>() )
                {
                    return parentLayoutTransform;
                }

                parent = parent->getParentPtr();
            }

            return nullptr;
        }

        void normalizeRect( Vector2<real_Num> &position, Vector2<real_Num> &size )
        {
            auto rectMin = position;
            auto rectMax = position + size;

            if( rectMin.x > rectMax.x )
            {
                std::swap( rectMin.x, rectMax.x );
            }

            if( rectMin.y > rectMax.y )
            {
                std::swap( rectMin.y, rectMax.y );
            }

            position = rectMin;
            size = rectMax - rectMin;
        }
    }  // namespace

    LayoutTransformSystem::LayoutTransformSystem() = default;

    LayoutTransformSystem::~LayoutTransformSystem() = default;

    auto LayoutTransformSystem::calculateElementPosition(
        const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
        const Vector2<real_Num> &position, const Vector2<real_Num> &size,
        const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
        const Vector2<real_Num> &anchorMax ) -> Vector2<real_Num>
    {
        Vector2<real_Num> elementPosition;
        Vector2<real_Num> elementSize;
        calculateElementPositionAndSize( parentPosition, parentSize, position, size, anchor, anchorMin,
                                         anchorMax, elementPosition, elementSize );

        return elementPosition;
    }

    //void LayoutTransformSystem::calculateElementPositionAndSize(
    //    const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
    //    const Vector2<real_Num> &position, const Vector2<real_Num> &size,
    //    const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
    //    const Vector2<real_Num> &anchorMax, f32 &left, f32 &right, f32 &top, f32 &bottom )
    //{
    //    // Calculate the absolute position of the element within the parent's coordinate system
    //    Vector2<real_Num> elementPosition;
    //    elementPosition.x = parentPosition.x + ( parentSize.x * anchorMin.x ) + position.x;
    //    elementPosition.y = parentPosition.y + ( parentSize.y * anchorMin.y ) + position.y;

    //    // Calculate the size of the element relative to the parent's size
    //    Vector2<real_Num> relativeSize;
    //    relativeSize.x = size.x + ( parentSize.x * ( anchorMax.x - anchorMin.x ) );
    //    relativeSize.y = size.y + ( parentSize.y * ( anchorMax.y - anchorMin.y ) );

    //    // Calculate the left, right, top, and bottom values
    //    left = elementPosition.x - ( relativeSize.x * anchor.x );
    //    right = elementPosition.x + ( ( 1.0f - anchor.x ) * relativeSize.x );
    //    top = elementPosition.y - ( relativeSize.y * anchor.y );
    //    bottom = elementPosition.y + ( ( 1.0f - anchor.y ) * relativeSize.y );
    //}

    void LayoutTransformSystem::calculateElementPositionAndSize(
        const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
        const Vector2<real_Num> &position, const Vector2<real_Num> &size,
        const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
        const Vector2<real_Num> &anchorMax, f32 &left, f32 &right, f32 &top, f32 &bottom )
    {
        Vector2<real_Num> elementPosition;
        Vector2<real_Num> elementSize;
        calculateElementPositionAndSize( parentPosition, parentSize, position, size, anchor, anchorMin,
                                         anchorMax, elementPosition, elementSize );

        left = elementPosition.x;
        right = elementPosition.x + elementSize.x;
        top = elementPosition.y;
        bottom = elementPosition.y + elementSize.y;
    }

    //void LayoutTransformSystem::calculateElementPositionAndSize(
    //    const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
    //    const Vector2<real_Num> &position, const Vector2<real_Num> &size,
    //    const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
    //    const Vector2<real_Num> &anchorMax, f32 &left, f32 &right, f32 &top, f32 &bottom )
    //{
    //    // Calculate the absolute position of the element within the parent's coordinate system
    //    Vector2<real_Num> elementPosition;
    //    elementPosition.x = parentPosition.x + ( parentSize.x * anchorMin.x ) + position.x;
    //    elementPosition.y = parentPosition.y + ( parentSize.y * anchorMin.y ) + position.y;

    //    // Calculate the size of the element relative to the parent's size
    //    // Adjusted to ensure clarity between absolute size and scaled size
    //    Vector2<real_Num> relativeSize;
    //    relativeSize.x = size.x + ( parentSize.x * ( anchorMax.x - anchorMin.x ) );
    //    relativeSize.y = size.y + ( parentSize.y * ( anchorMax.y - anchorMin.y ) );

    //    // Calculate the left, right, top, and bottom values based on the anchor point
    //    left = elementPosition.x - ( relativeSize.x * anchor.x );
    //    right = elementPosition.x + ( relativeSize.x * ( 1.0f - anchor.x ) );
    //    top = elementPosition.y - ( relativeSize.y * anchor.y );
    //    bottom = elementPosition.y + ( relativeSize.y * ( 1.0f - anchor.y ) );
    //}

    //void LayoutTransformSystem::calculateElementPositionAndSize(
    //    const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
    //    const Vector2<real_Num> &position, const Vector2<real_Num> &size,
    //    const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
    //    const Vector2<real_Num> &anchorMax, f32 &left, f32 &right, f32 &top, f32 &bottom )
    //{
    //    // Calculate the absolute position of the element within the parent's coordinate system
    //    Vector2<real_Num> elementPosition;
    //    elementPosition.x = parentPosition.x + ( parentSize.x * anchorMin.x ) + position.x;
    //    // Adjust y-coordinate calculation for top-origin coordinate system
    //    elementPosition.y = parentPosition.y + ( parentSize.y * anchorMin.y ) + position.y;

    //    // Calculate the size of the element relative to the parent's size
    //    Vector2<real_Num> relativeSize;
    //    relativeSize.x = size.x + ( parentSize.x * ( anchorMax.x - anchorMin.x ) );
    //    relativeSize.y = size.y + ( parentSize.y * ( anchorMax.y - anchorMin.y ) );

    //    // Calculate the left, right, top, and bottom values
    //    left = elementPosition.x - ( relativeSize.x * anchor.x );
    //    right = elementPosition.x + ( ( 1.0f - anchor.x ) * relativeSize.x );

    //    // Adjust top and bottom calculations for top-origin coordinate system
    //    top = elementPosition.y - ( relativeSize.y * anchor.y );
    //    bottom = elementPosition.y + ( ( 1.0f - anchor.y ) * relativeSize.y );
    //}

    void LayoutTransformSystem::calculateElementPositionAndSize(
        const Vector2<real_Num> &parentPosition, const Vector2<real_Num> &parentSize,
        const Vector2<real_Num> &position, const Vector2<real_Num> &size,
        const Vector2<real_Num> &anchor, const Vector2<real_Num> &anchorMin,
        const Vector2<real_Num> &anchorMax, Vector2<real_Num> &elementPosition,
        Vector2<real_Num> &elementSize )
    {
        const auto anchorMinX = std::min( anchorMin.x, anchorMax.x );
        const auto anchorMaxX = std::max( anchorMin.x, anchorMax.x );
        const auto anchorMinY = std::min( anchorMin.y, anchorMax.y );
        const auto anchorMaxY = std::max( anchorMin.y, anchorMax.y );

        const Vector2<real_Num> topLeftAnchor( anchorMinX, 1.0f - anchorMaxY );
        const Vector2<real_Num> bottomRightAnchor( anchorMaxX, 1.0f - anchorMinY );
        const Vector2<real_Num> pivot( anchor.x, 1.0f - anchor.y );

        elementSize.x = size.x + ( parentSize.x * ( bottomRightAnchor.x - topLeftAnchor.x ) );
        elementSize.y = size.y + ( parentSize.y * ( bottomRightAnchor.y - topLeftAnchor.y ) );

        elementPosition.x = parentPosition.x + ( parentSize.x * topLeftAnchor.x ) + position.x -
                            ( elementSize.x * pivot.x );
        elementPosition.y = parentPosition.y + ( parentSize.y * topLeftAnchor.y ) + position.y -
                            ( elementSize.y * pivot.y );

        normalizeRect( elementPosition, elementSize );
    }

    void LayoutTransformSystem::calculateElementPositionAndSize( const Vector2<real_Num> &position,
                                                                 const Vector2<real_Num> &size,
                                                                 const Vector2<real_Num> &anchor,
                                                                 const Vector2<real_Num> &anchorMin,
                                                                 const Vector2<real_Num> &anchorMax,
                                                                 Vector2<real_Num> &elementPosition,
                                                                 Vector2<real_Num> &elementSize )
    {
        calculateElementPositionAndSize( Vector2<real_Num>::zero(), size, position, size, anchor,
                                         anchorMin, anchorMax, elementPosition, elementSize );
    }

    void LayoutTransformSystem::load( SmartPtr<ISharedObject> data )
    {
        IComponentSystem::setLoadingState( LoadingState::Loading );

        const u32 size = 64;
        m_layoutTransformDataPool.setNextSize( size );
        m_layoutPool.setNextSize( size );
        m_transformPool.setNextSize( size );
        m_anchorPool.setNextSize( size );

        ComponentSystem::load( nullptr );

        setDirty( true );

        IComponentSystem::setLoadingState( LoadingState::Loaded );
    }

    void LayoutTransformSystem::unload( SmartPtr<ISharedObject> data )
    {
        IComponentSystem::setLoadingState( LoadingState::Unloading );

        m_layoutTransformDataPool.clear();
        m_layoutPool.clear();
        m_transformPool.clear();
        m_anchorPool.clear();

        ComponentSystem::unload( nullptr );
        IComponentSystem::setLoadingState( LoadingState::Unloaded );
    }

    void LayoutTransformSystem::update()
    {
#if 1
        updateSmart();
#else
        updateBruteForce();
#endif
    }

    void LayoutTransformSystem::updateSmart()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto gameManager = applicationManager->getGameManagerPtr();
        auto gameScene = gameManager->getCurrentScenePtr();

        static bool reviewReported = false;
        const auto reviewDirty = getDirtyComponents();
        if( !reviewReported && !reviewDirty.empty() )
        {
            reviewReported = true;
            auto first = reviewDirty.front();
            WP_LOG( "Racing layout review: scene=" + StringUtil::toString( static_cast<u32>( gameScene->getSceneLoadingState() ) ) +
                    " dirty=" + StringUtil::toString( static_cast<u32>( reviewDirty.size() ) ) +
                    " loaded=" + StringUtil::toString( first->isLoaded() ) +
                    " elapsed=" + StringUtil::toString( applicationManager->getTimerPtr()->getTimeSinceSceneLoad() ) );
        }

        if( gameScene->getSceneLoadingState() != IGameScene::SceneLoadingState::Loaded )
        {
            return;
        }

        ScopedLock gameSceneLock( gameScene );

        ScopedLock lock( this );

        auto timer = applicationManager->getTimerPtr();
        WP_ASSERT( timer );

        // Early exit if scene just loaded
        auto timeSinceSceneLoad = timer->getTimeSinceSceneLoad();
        if( timeSinceSceneLoad < 1.0 )
        {
            return;
        }

        auto dirtyComponents = getDirtyComponents();
        const auto forceFullPass = isDirty() && dirtyComponents.empty();
        if( forceFullPass )
        {
            dirtyComponents.reserve( m_components.size() );
            for( auto component : m_components )
            {
                if( component && component->isLoaded() )
                {
                    dirtyComponents.push_back( component );
                }
            }
        }

        if( dirtyComponents.empty() )
        {
            setDirty( false );
            return;
        }

        Array<IComponent *> cleanComponents;
        cleanComponents.reserve( dirtyComponents.size() );

        std::sort( dirtyComponents.begin(), dirtyComponents.end(), []( IComponent *a, IComponent *b ) {
            auto componentA = (LayoutTransform *)a;
            auto componentB = (LayoutTransform *)b;
            if( componentA && componentB )
            {
                const auto depthA = getHierarchyDepth( componentA );
                const auto depthB = getHierarchyDepth( componentB );
                if( depthA != depthB )
                {
                    return depthA < depthB;
                }

                return componentA->getZOrder() < componentB->getZOrder();
            }
            return false;
        } );

        for( auto component : dirtyComponents )
        {
            if( !component )
            {
                continue;
            }

            if( !component->isLoaded() )
            {
                continue;
            }

            auto layoutTransformData = (LayoutTransformData *)component->getComponentSystemData();
            if( !layoutTransformData )
            {
                cleanComponents.push_back( component );
                continue;
            }

            auto layoutState = layoutTransformData->layoutState;
            auto transformState = layoutTransformData->transformState;
            auto anchorState = layoutTransformData->anchorState;
            static u32 reviewStateCount = 0;
            if( reviewStateCount++ < 60 )
                WP_LOG( "Racing layout record: valid=" + StringUtil::toString( bool(layoutState) && bool(transformState) && bool(anchorState) ) +
                        " flags=" + StringUtil::toString( layoutState ? static_cast<u32>(layoutState->flags) : 0u ) );
            if( !layoutState || !transformState || !anchorState )
            {
                cleanComponents.push_back( component );
                continue;
            }

            const auto isLayoutDirty =
                BitUtil::getFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag );
            if( !forceFullPass && !isLayoutDirty )
            {
                cleanComponents.push_back( component );
                continue;
            }

            auto canvasTransform = (LayoutTransform *)component;
            layoutState->owner = canvasTransform;

            auto actor = canvasTransform->getActorPtr();
            static u32 reviewCount = 0;
            if( actor && reviewCount < 60 )
            {
                ++reviewCount;
                WP_LOG( "Racing layout element: " + actor->getName() +
                        " enabled=" + StringUtil::toString( actor->isEnabledInScene() ) +
                        " size=" + StringUtil::toString( transformState->size.x ) + "," + StringUtil::toString( transformState->size.y ) );
            }
            if( !actor )
            {
                layoutState->flags =
                    BitUtil::setFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag, false );
                cleanComponents.push_back( component );
                continue;
            }

            if( !actor->isEnabledInScene() )
            {
                layoutState->flags =
                    BitUtil::setFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag, false );
                cleanComponents.push_back( component );
                continue;
            }

            if( auto uiComponent = actor->getComponentPtr<UIComponent>() )
            {
                layoutState->uiComponent = uiComponent;
            }

            auto canvas = (Layout *)canvasTransform->getLayoutPtr();
            const auto referenceSize = makeReferenceSize( canvas );

            auto parentAbsolutePos = Vector2<real_Num>::zero();
            auto parentAbsoluteSize = referenceSize;

            if( auto parentCanvasTransform = findParentLayoutTransform( actor ) )
            {
                parentAbsolutePos = parentCanvasTransform->getAbsolutePosition();
                parentAbsoluteSize = parentCanvasTransform->getAbsoluteSize();

                if( MathF::equals( parentAbsoluteSize.x, 0.0f ) ||
                    MathF::equals( parentAbsoluteSize.y, 0.0f ) )
                {
                    parentAbsoluteSize = referenceSize;
                }
            }

            auto elementPos = Vector2<real_Num>::zero();
            auto elementSize = Vector2<real_Num>::zero();

            calculateElementPositionAndSize( parentAbsolutePos, parentAbsoluteSize,
                                             transformState->position, transformState->size,
                                             anchorState->anchor, anchorState->anchorMin,
                                             anchorState->anchorMax, elementPos, elementSize );

            const auto elementMin = elementPos;
            const auto elementMax = elementPos + elementSize;

            transformState->absolutePosition = elementPos;
            transformState->absoluteSize = elementSize;
            transformState->absoluteMin = elementMin;
            transformState->absoluteMax = elementMax;
            static bool reviewBounds = false;
            if( !reviewBounds && actor->getName() == "__StartMenuGenerated" )
            {
                reviewBounds = true;
                WP_LOG( "Racing layout bounds: calculated=" + StringUtil::toString(elementSize.x) + "," + StringUtil::toString(elementSize.y) +
                        " getter=" + StringUtil::toString(canvasTransform->getAbsoluteSize().x) + "," + StringUtil::toString(canvasTransform->getAbsoluteSize().y) );
            }

            const auto relativePos = safeDivide( elementPos, referenceSize );
            const auto relativeSize = safeDivide( elementSize, referenceSize );

            WP_ASSERT( relativePos.isValid() );
            WP_ASSERT( relativeSize.isValid() );

            auto uiComponents = actor->getComponentsByTypePtr<UIComponent>();
            for( auto uiComponent : uiComponents )
            {
                if( uiComponent )
                {
                    if( auto element = uiComponent->getElementPtr() )
                    {
                        element->setPosition( relativePos );
                        element->setSize( relativeSize );
                        uiComponent->updateElementState();
                    }
                }
            }

            layoutState->flags =
                BitUtil::setFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag, false );
            cleanComponents.push_back( component );
        }

        for( auto component : cleanComponents )
        {
            removeDirtyComponent( component );
        }

        if( getDirtyComponents().empty() )
        {
            setDirty( false );
        }
    }

    void LayoutTransformSystem::updateBruteForce()
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto timer = applicationManager->getTimerPtr();
        WP_ASSERT( timer );

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        auto timeSinceSceneLoad = timer->getTimeSinceSceneLoad();
        if( timeSinceSceneLoad < 1.0 )
        {
            return;
        }

        Array<IComponent *> dirtyComponents;
        dirtyComponents.reserve( 32 );

        for( auto component : m_components )
        {
            if( component && component->isLoaded() )
            {
                auto layoutState = component->getComponentStateByType<UILayoutStateData>();
                if( BitUtil::getFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag ) )
                {
                    dirtyComponents.push_back( component );
                }
            }
        }

        std::sort( dirtyComponents.begin(), dirtyComponents.end(), []( IComponent *a, IComponent *b ) {
            auto componentA = (LayoutTransform *)a;
            auto componentB = (LayoutTransform *)b;
            if( componentA && componentB )
            {
                return componentA->getZOrder() < componentB->getZOrder();
            }

            return false;
        } );

        for( auto component : dirtyComponents )
        {
            if( component )
            {
                auto layoutTransformData = (LayoutTransformData *)component->getComponentSystemData();
                SafePtr layoutState = layoutTransformData->layoutState;

                if( BitUtil::getFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag ) )
                {
                    SafePtr transformState = layoutTransformData->transformState;
                    SafePtr anchorState = layoutTransformData->anchorState;

                    auto canvasTransform = (LayoutTransform *)layoutState->owner;
                    if( canvasTransform )
                    {
                        if( auto actor = canvasTransform->getActorPtr() )
                        {
                            auto enabled = actor->isEnabledInScene();
                            if( !enabled )
                            {
                                layoutState->flags = BitUtil::setFlagValue<u8>(
                                    layoutState->flags, UILayoutStateData::dirtyFlag, false );
                                continue;
                            }

                            auto parentAbsolutePos = Vector2<real_Num>::zero();
                            auto parentAbsoluteSize = Vector2<real_Num>( 1920.0f, 1080.0f );

                            LayoutTransform *parentCanvasTransform = nullptr;

                            if( auto parent = actor->getParentPtr() )
                            {
                                parentCanvasTransform = parent->getComponentPtr<LayoutTransform>();
                                while( !parentCanvasTransform && parent )
                                {
                                    parent = parent->getParentPtr();

                                    if( parent )
                                    {
                                        parentCanvasTransform =
                                            parent->getComponentPtr<LayoutTransform>();
                                    }
                                }

                                if( parentCanvasTransform )
                                {
                                    parentAbsolutePos = parentCanvasTransform->getAbsolutePosition();
                                    parentAbsoluteSize = parentCanvasTransform->getAbsoluteSize();
                                }
                            }

                            auto screenPos = Vector2<real_Num>::zero();
                            auto position = transformState->position;
                            auto size = transformState->size;

                            auto horizontalAlignment =
                                static_cast<HorizontalAlignment>( (u8)layoutState->horizontalAlignment );

                            auto verticalAlignment =
                                static_cast<VerticalAlignment>( (u8)layoutState->verticalAlignment );

                            auto relativePos = Vector2<real_Num>::zero();
                            auto relativeSize = Vector2<real_Num>::zero();

                            if( !( MathF::equals( parentAbsoluteSize.x, 0.0f ) &&
                                   MathF::equals( parentAbsoluteSize.y, 0.0f ) ) )
                            {
                                if( parentAbsoluteSize.x > 0.0f && parentAbsoluteSize.y > 0.0f )
                                {
                                    relativePos = Vector2<real_Num>(
                                        screenPos.x / static_cast<f32>( parentAbsoluteSize.x ),
                                        screenPos.y / static_cast<f32>( parentAbsoluteSize.y ) );

                                    relativeSize = Vector2<real_Num>(
                                        size.x / static_cast<f32>( parentAbsoluteSize.x ),
                                        size.y / static_cast<f32>( parentAbsoluteSize.y ) );
                                }
                            }

                            auto elementMin = Vector2<real_Num>::zero();
                            auto elementMax = Vector2<real_Num>::zero();
                            auto elementPos = Vector2<real_Num>::zero();
                            auto elementSize = Vector2<real_Num>::zero();

                            f32 left = 0.0f;
                            f32 right = 0.0f;
                            f32 top = 0.0f;
                            f32 bottom = 0.0f;

                            if( parentCanvasTransform )
                            {
                                calculateElementPositionAndSize(
                                    parentAbsolutePos, parentAbsoluteSize, position, size,
                                    anchorState->anchor, anchorState->anchorMin, anchorState->anchorMax,
                                    left, right, top, bottom );

                                elementPos = Vector2<real_Num>( left, top );
                                elementSize = Vector2<real_Num>( right, bottom ) - elementPos;

                                elementMin = elementPos;
                                elementMax = elementPos + elementSize;
                            }
                            else
                            {
                                calculateElementPositionAndSize(
                                    Vector2<real_Num>::zero(), Vector2<real_Num>( 1920.0f, 1080.0f ),
                                    position, size, anchorState->anchor, anchorState->anchorMin,
                                    anchorState->anchorMax, left, right, top, bottom );

                                elementPos = Vector2<real_Num>( left, top );
                                elementSize = Vector2<real_Num>( right, bottom ) - elementPos;

                                elementMin = elementPos;
                                elementMax = elementPos + elementSize;
                            }

                            if( elementSize.x < 0.0f )
                            {
                                elementSize.x = MathF::Abs( elementSize.x );
                            }

                            if( elementSize.y < 0.0f )
                            {
                                elementSize.y = MathF::Abs( elementSize.y );
                            }

                            auto canvas = (Layout *)canvasTransform->getLayoutPtr();
                            if( canvas )
                            {
                                auto canvasSize = canvas->getReferenceSize();
                                auto fCanvasSize = Vector2<real_Num>( static_cast<f32>( canvasSize.x ),
                                                                      static_cast<f32>( canvasSize.y ) );

                                if( !( MathF::equals( static_cast<f32>( canvasSize.x ), 0.0f ) &&
                                       MathF::equals( static_cast<f32>( canvasSize.y ), 0.0f ) ) )
                                {
                                    relativePos = Vector2<real_Num>(
                                        elementPos.x / static_cast<f32>( canvasSize.x ),
                                        elementPos.y / static_cast<f32>( canvasSize.y ) );
                                    relativeSize = Vector2<real_Num>(
                                        elementSize.x / static_cast<f32>( canvasSize.x ),
                                        elementSize.y / static_cast<f32>( canvasSize.y ) );
                                }

                                auto absolutePosition = relativePos * fCanvasSize;
                                auto absoluteSize = relativeSize * fCanvasSize;

                                transformState->absolutePosition = absolutePosition;
                                transformState->absoluteSize = absoluteSize;
                                transformState->absoluteMin = elementMin;
                                transformState->absoluteMax = elementMax;

                                WP_ASSERT( relativePos.isValid() );
                                WP_ASSERT( relativeSize.isValid() );

                                auto uiComponents = actor->getComponentsByTypePtr<UIComponent>();
                                for( auto uiComponent : uiComponents )
                                {
                                    if( uiComponent )
                                    {
                                        if( auto element = uiComponent->getElementPtr() )
                                        {
                                            element->setPosition( relativePos );
                                            element->setSize( relativeSize );

                                            layoutState->flags = BitUtil::setFlagValue<u8>(
                                                layoutState->flags, UILayoutStateData::dirtyFlag,
                                                false );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    u32 LayoutTransformSystem::addComponent( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this );

        auto data = new( m_layoutTransformDataPool.allocate_object() ) LayoutTransformData;
        component->setComponentSystemData( data );

        data->component = component.get();

        WP_ASSERT( isValid() );

        const auto retries = 10;
        for( size_t retry = 0; retry < retries; ++retry )
        {
            const auto size = getSize();
            for( u32 i = m_lastFreeSlot.load(); i < size; ++i )
            {
                if( isFreeSlot( i ) )
                {
                    setLoadingState( i, LoadingState::Allocated );

                    WP_ASSERT( getObject( i ) == nullptr );
                    setObject( i, component );
                    data->slot = i;

                    auto pLayoutState = m_layoutPool.allocate_object();
                    auto layoutState = new( pLayoutState.get() ) UILayoutStateData;
                    layoutState->owner = component.get();
                    layoutState->flags = BitUtil::setFlagValue<u8>( layoutState->flags,
                                                                    UILayoutStateData::dirtyFlag, true );
                    component->addState( layoutState );

                    auto pTransformState = m_transformPool.allocate_object();
                    auto transformState = new( pTransformState.get() ) UITransformStateData;
                    component->addState( transformState );

                    auto pAnchorState = m_anchorPool.allocate_object();
                    auto anchorState = new( pAnchorState.get() ) UIAnchorStateData;
                    component->addState( anchorState );

                    data->layoutState = layoutState;
                    data->transformState = transformState;
                    data->anchorState = anchorState;

                    component->setComponentSystem( this );
                    addDirtyComponent( component );

                    m_lastFreeSlot = i + 1;
                    return i;
                }
            }

            for( u32 i = 0; i < m_lastFreeSlot; ++i )
            {
                if( isFreeSlot( i ) )
                {
                    setLoadingState( i, LoadingState::Allocated );

                    WP_ASSERT( getObject( i ) == nullptr );
                    setObject( i, component );
                    data->slot = i;

                    auto pLayoutState = m_layoutPool.allocate_object();
                    auto layoutState = new( pLayoutState.get() ) UILayoutStateData;
                    layoutState->owner = component.get();
                    layoutState->flags = BitUtil::setFlagValue<u8>( layoutState->flags,
                                                                    UILayoutStateData::dirtyFlag, true );
                    component->addState( layoutState );

                    auto pTransformState = m_transformPool.allocate_object();
                    auto transformState = new( pTransformState.get() ) UITransformStateData;
                    component->addState( transformState );

                    auto pAnchorState = m_anchorPool.allocate_object();
                    auto anchorState = new( pAnchorState.get() ) UIAnchorStateData;
                    component->addState( anchorState );

                    data->layoutState = layoutState;
                    data->transformState = transformState;
                    data->anchorState = anchorState;

                    component->setComponentSystem( this );
                    addDirtyComponent( component );

                    m_lastFreeSlot = i + 1;
                    return i;
                }
            }

            WP_ASSERT( isValid() );

            // grow the arrays
            auto growSize = getGrowSize();
            const auto currentSize = getSize();
            reserve( ( currentSize + growSize ) * 2 );

            WP_ASSERT( isValid() );
        }

        WP_ASSERT( isValid() );
        return std::numeric_limits<u32>::max();
    }

    void LayoutTransformSystem::removeComponent( SmartPtr<IComponent> component )
    {
        ScopedLock lock( this );

        u32 slot = std::numeric_limits<u32>::max();
        if( auto data = component->getComponentSystemData() )
        {
            auto layoutData = (LayoutTransformData *)data;
            WP_ASSERT( layoutData->component == component.get() );
            slot = layoutData->slot;

            m_layoutTransformDataPool.free_object( (LayoutTransformData *)data );
            component->setComponentSystemData( nullptr );
        }

        WP_ASSERT( slot < getSize() );
        WP_ASSERT( getObject( slot ).get() == component.get() );

        auto states = component->getStates();
        for( auto &state : states )
        {
            component->removeState( state );

            if( state->isDerived<UILayoutStateData>() )
            {
                m_layoutPool.free_object( (UILayoutStateData *)state.get() );
            }
            else if( state->isDerived<UITransformStateData>() )
            {
                m_transformPool.free_object( (UITransformStateData *)state.get() );
            }
            else if( state->isDerived<UIAnchorStateData>() )
            {
                m_anchorPool.free_object( (UIAnchorStateData *)state.get() );
            }
        }

        WP_ASSERT( isValid() );

        component->setComponentSystem( nullptr );

        setObject( slot, nullptr );
        setLoadingState( slot, LoadingState::Unallocated );

        m_dirtyComponents.erase(
            std::remove( m_dirtyComponents.begin(), m_dirtyComponents.end(), component.get() ),
            m_dirtyComponents.end() );
        m_componentPointers.erase(
            std::remove( m_componentPointers.begin(), m_componentPointers.end(), component.get() ),
            m_componentPointers.end() );
    }

    void LayoutTransformSystem::updateDirtyList()
    {
        auto componentData = getComponents();

        Array<IComponent *> &components = m_componentPointers;
        components.clear();
        components.reserve( 12 );

        for( auto component : componentData )
        {
            if( component )
            {
                auto layoutState = component->getComponentStateByType<UILayoutStateData>();
                if( layoutState )
                {
                    if( BitUtil::getFlagValue<u8>( layoutState->flags, UILayoutStateData::dirtyFlag ) )
                    {
                        components.push_back( component );
                    }
                }
            }
        }

        //std::sort( components.begin(), components.end(), []( IComponent *a, IComponent *b ) {
        //    return ( (UIComponent *)a )->getZOrder() < ( (UIComponent *)b )->getZOrder();
        //} );
    }

    Parameter LayoutTransformSystem::handleEvent( EventType eventType, hash_type eventValue,
                                                  const Array<Parameter> &arguments,
                                                  SmartPtr<ISharedObject> sender,
                                                  SmartPtr<ISharedObject> object,
                                                  SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::addActor || eventValue == IEvent::removeActor ||
            eventValue == IEvent::cameraManagerReset || eventValue == IEvent::loadScene )
        {
            auto components = getComponents();
            for( auto &component : components )
            {
                if( component && component->isLoaded() )
                {
                    if( auto data = (LayoutTransformData *)component->getComponentSystemData() )
                    {
                        if( auto layoutState = data->layoutState )
                        {
                            layoutState->flags = BitUtil::setFlagValue<u8>(
                                layoutState->flags, UILayoutStateData::dirtyFlag, true );
                        }
                    }

                    ComponentSystem::addDirtyComponent( component );
                }
            }

            makeDirty();
            return {};
        }

        return ComponentSystem::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    void LayoutTransformSystem::addDirtyComponent( SmartPtr<IComponent> component )
    {
        if( !component )
        {
            return;
        }

        ComponentSystem::addDirtyComponent( component );

        if( auto actor = component->getActorPtr() )
        {
            auto childComponents = actor->getAllComponentsAndInChildren<LayoutTransform>();
            for( auto childComponent : childComponents )
            {
                ComponentSystem::addDirtyComponent( childComponent );
            }
        }
    }

}  // namespace workphone::scene
