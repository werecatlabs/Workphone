#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/LayoutContainer.hpp>
#include <Workphone/Scene/UiUtil.hpp>

namespace workphone::scene
{
    const String LayoutContainer::offsetStr = String( "offset" );
    const String LayoutContainer::spacingStr = String( "spacing" );
    const String LayoutContainer::childVerticalAlignmentStr = String( "childVerticalAlignment" );
    const String LayoutContainer::childHorizontalAlignmentStr = String( "childHorizontalAlignment" );
    const String LayoutContainer::useChildStartOffsetStr = String( "useChildStartOffset" );
    const String LayoutContainer::useChildVerticalAlignmentStr = String( "useChildVerticalAlignment" );
    const String LayoutContainer::useChildHorizontalAlignmentStr =
        String( "useChildHorizontalAlignment" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, LayoutContainer, Component );

    LayoutContainer::LayoutContainer() = default;

    LayoutContainer::~LayoutContainer() = default;

    void LayoutContainer::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void LayoutContainer::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void LayoutContainer::updateFlags( u32 flags, u32 oldFlags )
    {
        updateTransform();
    }

    void LayoutContainer::updateTransform()
    {
        /* do nothing */
    }

    SmartPtr<Properties> LayoutContainer::getProperties() const
    {
        auto properties = Component::getProperties();

        properties->setProperty( LayoutContainer::offsetStr, m_offset );
        properties->setProperty( LayoutContainer::spacingStr, m_spacing );

        properties->setPropertyAsEnum( "childVerticalAlignment",
                                       static_cast<u32>( m_childVerticalAlignment ),
                                       UiUtil::verticalAlignmentTypes );
        properties->setPropertyAsEnum( "childHorizontalAlignment",
                                       static_cast<u32>( m_childHorizontalAlignment ),
                                       UiUtil::horizontalAlignmentTypes );
        properties->setProperty( LayoutContainer::useChildStartOffsetStr, m_useChildStartOffset );
        properties->setProperty( LayoutContainer::useChildVerticalAlignmentStr,
                                 m_useChildVerticalAlignment );
        properties->setProperty( LayoutContainer::useChildHorizontalAlignmentStr,
                                 m_useChildHorizontalAlignment );

        return properties;
    }

    void LayoutContainer::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        properties->getPropertyValue( LayoutContainer::offsetStr, m_offset );
        properties->getPropertyValue( LayoutContainer::spacingStr, m_spacing );
        properties->getPropertyValue( LayoutContainer::childVerticalAlignmentStr,
                                      (u32 &)m_childVerticalAlignment );
        properties->getPropertyValue( LayoutContainer::childHorizontalAlignmentStr,
                                      (u32 &)m_childHorizontalAlignment );
        properties->getPropertyValue( LayoutContainer::useChildStartOffsetStr, m_useChildStartOffset );
        properties->getPropertyValue( LayoutContainer::useChildVerticalAlignmentStr,
                                      m_useChildVerticalAlignment );
        properties->getPropertyValue( LayoutContainer::useChildHorizontalAlignmentStr,
                                      m_useChildHorizontalAlignment );

        m_childVerticalAlignment = static_cast<VerticalAlignment>(
            Math<u32>::clamp( (u32)m_childVerticalAlignment, (u32)VerticalAlignment::TOP,
                              (u32)VerticalAlignment::CUSTOM ) );

        m_childHorizontalAlignment = static_cast<HorizontalAlignment>(
            Math<u32>::clamp( (u32)m_childHorizontalAlignment, (u32)HorizontalAlignment::LEFT,
                              (u32)HorizontalAlignment::CUSTOM ) );

        updateTransform();
    }

    void LayoutContainer::setSpacing( f32 spacing )
    {
        ScopedLock lock( this );
        m_spacing = spacing;
    }

    f32 LayoutContainer::getSpacing() const
    {
        ScopedLock lock( this );
        return m_spacing;
    }

    void LayoutContainer::setChildVerticalAlignment( VerticalAlignment childVerticalAlignment )
    {
        ScopedLock lock( this );
        m_childVerticalAlignment = childVerticalAlignment;
    }

    VerticalAlignment LayoutContainer::getChildVerticalAlignment() const
    {
        ScopedLock lock( this );
        return m_childVerticalAlignment;
    }

    void LayoutContainer::setChildHorizontalAlignment( HorizontalAlignment childHorizontalAlignment )
    {
        ScopedLock lock( this );
        m_childHorizontalAlignment = childHorizontalAlignment;
    }

    HorizontalAlignment LayoutContainer::getChildHorizontalAlignment() const
    {
        ScopedLock lock( this );
        return m_childHorizontalAlignment;
    }

    void LayoutContainer::setOffset( f32 offset )
    {
        ScopedLock lock( this );
        m_offset = offset;
    }

    f32 LayoutContainer::getOffset() const
    {
        ScopedLock lock( this );
        return m_offset;
    }

    void LayoutContainer::setUseChildStartOffset( bool useChildStartOffset )
    {
        ScopedLock lock( this );
        m_useChildStartOffset = useChildStartOffset;
    }

    bool LayoutContainer::getUseChildStartOffset() const
    {
        ScopedLock lock( this );
        return m_useChildStartOffset;
    }

    void LayoutContainer::setUseChildVerticalAlignment( bool useChildVerticalAlignment )
    {
        ScopedLock lock( this );
        m_useChildVerticalAlignment = useChildVerticalAlignment;
    }

    bool LayoutContainer::getUseChildVerticalAlignment() const
    {
        ScopedLock lock( this );
        return m_useChildVerticalAlignment;
    }

    void LayoutContainer::setUseChildHorizontalAlignment( bool useChildHorizontalAlignment )
    {
        ScopedLock lock( this );
        m_useChildHorizontalAlignment = useChildHorizontalAlignment;
    }

    bool LayoutContainer::getUseChildHorizontalAlignment() const
    {
        ScopedLock lock( this );
        return m_useChildHorizontalAlignment;
    }

    void LayoutContainer::setPadding( f32 padding )
    {
        ScopedLock lock( this );
        m_padding = padding;
    }

    f32 LayoutContainer::getPadding() const
    {
        ScopedLock lock( this );
        return m_padding;
    }
}  // namespace workphone::scene
