#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMaterialTechnique, MaterialTechnique );

    ClawMaterialTechnique::ClawMaterialTechnique() : m_scheme( 0 )
    {
    }

    ClawMaterialTechnique::~ClawMaterialTechnique()
    {
    }

    hash32 ClawMaterialTechnique::getScheme() const
    {
        return m_scheme;
    }

    void ClawMaterialTechnique::setScheme( hash32 scheme )
    {
        m_scheme = scheme;
    }

    u32 ClawMaterialTechnique::getNumPasses() const
    {
        return static_cast<u32>( m_passes.size() );
    }

    SmartPtr<IMaterialPass> ClawMaterialTechnique::createPass()
    {
        auto pass = workphone::make_ptr<ClawMaterialPass>();
            pass->setMaterial( getMaterial() );
            pass->setParent( this );
        addPass( pass );
        return pass;
    }

    void ClawMaterialTechnique::addPass( SmartPtr<IMaterialPass> pass )
    {
        if( pass )
        {
            m_passes.push_back( pass );
        }
    }

    void ClawMaterialTechnique::removePass( SmartPtr<IMaterialPass> pass )
    {
        if( !pass )
            return;

        for( size_t i = 0; i < m_passes.size(); ++i )
        {
            if( m_passes[i] == pass )
            {
                m_passes.erase( m_passes.begin() + i );
                break;
            }
        }
    }

    void ClawMaterialTechnique::removePasses()
    {
        m_passes.clear();
    }

    Array<SmartPtr<IMaterialPass>> ClawMaterialTechnique::getPasses() const
    {
        return m_passes;
    }

    void ClawMaterialTechnique::setPasses( Array<SmartPtr<IMaterialPass>> passes )
    {
        m_passes = passes;
    }

    SmartPtr<IMaterialPass> ClawMaterialTechnique::getPass( u32 index ) const
    {
        if( index < m_passes.size() )
        {
            return m_passes[index];
        }
        return nullptr;
    }

    SmartPtr<Properties> ClawMaterialTechnique::getProperties() const
    {
        auto props = workphone::make_ptr<Properties>();
        props->setProperty( "scheme", m_scheme );
        props->setProperty( "numPasses", (u32)m_passes.size() );

        // Add children properties for each pass if needed by the inspector
        return props;
    }

    void ClawMaterialTechnique::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
            return;

        //hash32 scheme = 0;
        //if (properties->getProperty("scheme", scheme))
        //{
        //    setScheme(scheme);
        //}
    }

    Array<SmartPtr<ISharedObject>> ClawMaterialTechnique::getChildObjects() const
    {
        Array<SmartPtr<ISharedObject>> children;
        for( const auto &pass : m_passes )
        {
            children.push_back( pass );
        }
        return children;
    }

    bool ClawMaterialTechnique::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        // Pass message to children
        for( auto &pass : m_passes )
        {
            if( pass->handleStateMessage( message ) )
            {
                return true;
            }
        }
        return false;
    }

    bool ClawMaterialTechnique::handleStateChanged( SmartPtr<IState> &state )
    {
        // Pass state change to children
        for( auto &pass : m_passes )
        {
            if( pass->handleStateChanged( state ) )
            {
                return true;
            }
        }
        return false;
    }

}  // namespace workphone::render
