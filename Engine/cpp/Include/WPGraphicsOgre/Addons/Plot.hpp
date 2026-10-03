/*-----------------------------------------------------------------------------
2011, Adis Hamzic
-----------------------------------------------------------------------------*/

#ifndef _plot_hpp_
#define _plot_hpp_

#include <string>
#include <OgreMaterial.h>
#include <OgreTexture.h>

class Plot
{
public:
    typedef std::list<Ogre::Real> DataType;

public:
    Plot( const Ogre::String &pMaterialName, int pPointCount, Ogre::Real pDefaultValue,
          bool pAutoscale = true,
          const Ogre::ColourValue &pTintColor = Ogre::ColourValue( 1, 0, 0, 1 ) );
    ~Plot();

    void addPoint( Ogre::Real pValue );
    void clearPoints();

    const DataType &getData() const;
    void setData( const DataType &pData );

    int getPointCount() const;
    void setPointCount( int pPointCount );

    float getDefaultValue() const;
    void setDefaultValue( Ogre::Real pDefaultValue );

    bool getAutoscale() const;
    void setAutoscale( bool pAutoscale );

    const Ogre::ColourValue &getTintColor() const;
    void setTintColor( const Ogre::ColourValue &pTintColor );

protected:
    void createTexture();
    void updateData();
    void normalizeData();

private:
    Ogre::MaterialPtr m_Material;
    Ogre::TexturePtr m_DataTexture;

    int m_PointCount;
    Ogre::Real m_DefaultValue;
    bool m_Autoscale;
    Ogre::ColourValue m_TintColor;

    DataType m_Data;
};

#endif  //_plot_hpp_
