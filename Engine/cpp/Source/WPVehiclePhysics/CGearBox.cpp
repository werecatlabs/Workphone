#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CGearBox.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    CGearBox::CGearBox() = default;

    CGearBox::~CGearBox() = default;

    void CGearBox::setRatios( const Array<f32> &ratios )
    {
        WP_ASSERT( ratios.size() > 0 );
        m_ratios = ratios;
        if( m_currentGear >= m_ratios.size() )
        {
            m_currentGear = m_ratios.size() == 0 ? 0 : static_cast<u32>( m_ratios.size() - 1 );
        }
    }

    Array<f32> CGearBox::getRatios() const
    {
        return m_ratios;
    }

    f32 CGearBox::getRatio( u32 gear ) const
    {
        if( gear < m_ratios.size() )
        {
            return m_ratios[gear];
        }

        WP_ASSERT( gear < m_ratios.size() );
        WP_LOG_ERROR( "CGearBox::getRatio gear index out of range." );
        return 0.0;
    }

    u32 CGearBox::getNumGears() const
    {
        return (u32)m_ratios.size();
    }

    void CGearBox::decreamentSelectedGear()
    {
        if( m_currentGear > 0 )
        {
            --m_currentGear;
        }
    }

    void CGearBox::increamentSelectedGear()
    {
        u32 maxGear = getNumGears();
        if( maxGear > 0 && m_currentGear < maxGear - 1 )
        {
            ++m_currentGear;
        }
    }

    u32 CGearBox::getCurrentGear() const
    {
        return m_currentGear;
    }

    void CGearBox::setCurrentGear( u32 currentGear )
    {
        WP_ASSERT( m_ratios.size() == 0 || currentGear < m_ratios.size() );
        if( m_ratios.size() > 0 && currentGear >= m_ratios.size() )
        {
            WP_LOG_ERROR( "CGearBox::setCurrentGear rejected out-of-range gear." );
            return;
        }

        m_currentGear = currentGear;
    }

    SmartPtr<Properties> CGearBox::getProperties() const
    {
        auto properties = CVehicleComponent<IGearBox>::getProperties();
        WP_ASSERT( properties );

        properties->setProperty( "Current Gear", getCurrentGear() );

        return properties;
    }

    void CGearBox::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            WP_LOG_ERROR( "CGearBox::setProperties received null properties." );
            return;
        }

        CVehicleComponent<IGearBox>::setProperties( properties );

        auto currentGear = getCurrentGear();

        properties->getPropertyValue( "Current Gear", currentGear );

        setCurrentGear( currentGear );
    }
} // namespace workphone
