/*-----------------------------------------------------------------------------
2011, Adis Hamzic
-----------------------------------------------------------------------------*/
#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "Plot.hpp"
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>
#include <OgreTechnique.h>
#include <OgreHardwarePixelBuffer.h>

Plot::Plot( const Ogre::String &pMaterialName, int pPointCount, Ogre::Real pDefaultValue,
            bool pAutoscale, const Ogre::ColourValue &pTintColor ) :
    m_PointCount( pPointCount ),
    m_DefaultValue( pDefaultValue ),
    m_Autoscale( pAutoscale ),
    m_TintColor( pTintColor ),
    m_Data( pPointCount, pDefaultValue )
{
    // create a new material with the given name
    m_Material = Ogre::MaterialManager::getSingleton().create(
        pMaterialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

    Ogre::Pass *pass = m_Material->getTechnique( 0 )->getPass( 0 );
    pass->setSceneBlending( Ogre::SBT_TRANSPARENT_ALPHA );

    pass->setVertexProgram( "Plot/plotvp" );
    pass->setFragmentProgram( "Plot/plotfp" );

    Ogre::TextureUnitState *state = pass->createTextureUnitState();
    state->setTextureFiltering( Ogre::TFO_NONE );
    state->setTextureAddressingMode( Ogre::TextureUnitState::TAM_MIRROR );

    createTexture();
    updateData();
}

Plot::~Plot()
{
    m_Material->unload();
    m_DataTexture->unload();
}

void Plot::addPoint( Ogre::Real pValue )
{
    // add a point to the fornt of the list
    // remove the one on the back to preserve length
    m_Data.push_back( pValue );
    m_Data.pop_front();
    updateData();
}

void Plot::clearPoints()
{
    // set all points to the default value
    for( DataType::iterator i = m_Data.begin(); i != m_Data.end(); ++i )
        *i = m_DefaultValue;
    updateData();
}

const Plot::DataType &Plot::getData() const
{
    return m_Data;
}

void Plot::setData( const Plot::DataType &pData )
{
    // manually set data, updates the number of points used
    // may cause the data texture to be created again
    m_Data = pData;
    m_PointCount = (int)m_Data.size();
    createTexture();
    updateData();
}

int Plot::getPointCount() const
{
    return m_PointCount;
}

void Plot::setPointCount( int pPointCount )
{
    // set the number of points used
    // may cause the data texture to be created again
    m_PointCount = pPointCount;
    m_Data.resize( m_PointCount, m_DefaultValue );
    createTexture();
    updateData();
}

float Plot::getDefaultValue() const
{
    return m_DefaultValue;
}

void Plot::setDefaultValue( Ogre::Real pValue )
{
    // sets the default value
    // use clear to reset all values to it
    m_DefaultValue = pValue;
}

bool Plot::getAutoscale() const
{
    return m_Autoscale;
}

void Plot::setAutoscale( bool pAutoscale )
{
    // should we use autoscaling or did the user do it
    m_Autoscale = pAutoscale;
    updateData();
}

const Ogre::ColourValue &Plot::getTintColor() const
{
    return m_TintColor;
}

void Plot::setTintColor( const Ogre::ColourValue &pTintColor )
{
    // set the color of the chart
    m_TintColor = pTintColor;
    updateData();
}

void Plot::createTexture()
{
    // get a dimension that is a power of two
    //  http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    int textureSize = m_Data.size();
    textureSize--;
    textureSize |= textureSize >> 1;
    textureSize |= textureSize >> 2;
    textureSize |= textureSize >> 4;
    textureSize |= textureSize >> 8;
    textureSize |= textureSize >> 16;
    textureSize++;

    // do we need to recreate the texture
    if( !m_DataTexture.isNull() && textureSize != m_DataTexture->getWidth() )
    {
        m_DataTexture->unload();
        m_DataTexture.setNull();
    }

    if( m_DataTexture.isNull() )
    {
        // create a new texture and add it to the material
        auto textureName = "DataTextureFor" + m_Material->getName();
        m_DataTexture = Ogre::TextureManager::getSingleton().createManual(
            textureName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_1D,
            textureSize, 1, 0, Ogre::PF_L8 );
        m_Material->getTechnique( 0 )->getPass( 0 )->getTextureUnitState( 0 )->setTextureName(
            textureName );
    }
}

void Plot::normalizeData()
{
    // get buffer to the data texture
    Ogre::HardwarePixelBufferSharedPtr buffer = m_DataTexture->getBuffer();
    buffer->lock( Ogre::HardwareBuffer::HBL_NORMAL );
    Ogre::uint8 *data = (Ogre::uint8 *)buffer->getCurrentLock().data;

    if( m_Autoscale )
    {
        // if we are using autoscale, get the axis span used for scaling
        std::pair<DataType::iterator, DataType::iterator>
            minMaxElement;  // =
                            // std::minmax_element(m_Data.begin(), m_Data.end());
        Ogre::Real minElement = *minMaxElement.first;
        Ogre::Real maxElement = *minMaxElement.second;
        Ogre::Real span = maxElement - minElement;

        if( span > 0.0001f )
        {
            // scale using the span to [0, 1]
            int dataI = 0;
            for( DataType::iterator i = m_Data.begin(); i != m_Data.end(); ++i, ++dataI )
                data[dataI] = Ogre::uint8( ( ( *i - minElement ) / span ) * 255 );
        }
        else
        {
            // the span is too small, fill values with zeros
            memset( buffer->getCurrentLock().data, 0, buffer->getSizeInBytes() );
        }
    }
    else
    {
        // not using autoscaling, use raw values, but clamp them first to avoid overflowing
        int dataI = 0;
        for( DataType::iterator i = m_Data.begin(); i != m_Data.end(); ++i, ++dataI )
        {
            Ogre::Real value = *i;
            if( value > 1.0f )
                value = 1.0f;
            if( value < 0.0f )
                value = 0.0f;
            data[dataI] = Ogre::uint8( value * 255 );
        }
    }

    buffer->unlock();
}

void Plot::updateData()
{
    // set gpu parameters and update texture data
    Ogre::GpuProgramParametersSharedPtr fragmentProgParams =
        m_Material->getTechnique( 0 )->getPass( 0 )->getFragmentProgramParameters();
    fragmentProgParams->setNamedConstant( "pointCount", float( m_Data.size() ) );
    fragmentProgParams->setNamedConstant( "texWidth", float( m_DataTexture->getWidth() ) );
    fragmentProgParams->setNamedConstant( "tintColor", m_TintColor );

    normalizeData();
}
