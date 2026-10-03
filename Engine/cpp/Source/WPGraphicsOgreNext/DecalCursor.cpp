/**
 * @file DecalCursor.cpp
 * @author Jesse Wright - www.cutthroatstudios.com
 * @note Reimplemented for OgreNext (2.x) API
 *
 * Projective decal using an orthographic Ogre::Frustum.
 * The frustum is parked PROJECTOR_HEIGHT units above the terrain and oriented
 * to face straight down. An additive, depth-biased "Decal" pass is inserted
 * into the terrain material's first technique. The projective TextureUnitState
 * is created lazily on show() and torn down on hide() so there is no GPU cost
 * when the cursor is invisible.
 */
#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/DecalCursor.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreFrustum.h>
#include <OgreTextureUnitState.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreHlmsDatablock.h>
#include <OgreId.h>

namespace
{
    /// World-unit scale — 1 unit == 1 metre.
    constexpr float METER_SCALE = 1.0f;

    /// How far above the target the frustum node is parked.
    constexpr float PROJECTOR_HEIGHT = 10.0f * METER_SCALE;

    /// Near clip just in front of the node so nothing inside the frustum is clipped.
    constexpr float PROJECTOR_NEAR = 0.5f * METER_SCALE;

    /// Far clip must reach the terrain below (height + safety margin).
    constexpr float PROJECTOR_FAR = ( PROJECTOR_HEIGHT + 5.0f ) * METER_SCALE;
}  // namespace

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

DecalCursor::DecalCursor( Ogre::SceneManager *man, Ogre::MaterialPtr terrainMat,
                          const Ogre::Vector2 &size, const std::string &tex )
    : m_bVisible( false )
    , m_nodeProj( nullptr )
    , m_pass( nullptr )
    , m_frustProj( nullptr )
    , m_texState( nullptr )
    , m_sceneMgr( man )
    , m_terrainMat( terrainMat )
    , m_pos( Ogre::Vector3::ZERO )
    , m_size( Ogre::Vector2::ZERO )
{
    assert( m_sceneMgr && "DecalCursor: scene manager must not be null" );
    assert( !m_terrainMat.isNull() && "DecalCursor: terrain material must not be null" );
    assert( m_terrainMat->getNumTechniques() > 0 && "DecalCursor: terrain material has no techniques" );

    init( size, tex );
}

DecalCursor::~DecalCursor()
{
    // Ensure the texture unit state is removed before we tear down the pass.
    if( m_bVisible )
        hideTerrainDecal();

    // Remove the decal pass from the material.
    if( m_pass && !m_terrainMat.isNull() )
    {
        Ogre::Technique *tech = m_terrainMat->getTechnique( 0 );
        if( tech )
            tech->removePass( m_pass->getIndex() );
        m_pass = nullptr;
    }

    // Detach and destroy the projection scene node.
    if( m_nodeProj )
    {
        m_nodeProj->detachAllObjects();
        m_sceneMgr->destroySceneNode( m_nodeProj );
        m_nodeProj = nullptr;
    }

    // Free the frustum using Ogre's allocator (OGRE_NEW was used to create it).
    if( m_frustProj )
    {
        OGRE_DELETE m_frustProj;
        m_frustProj = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void DecalCursor::init( const Ogre::Vector2 &size, const std::string &tex )
{
    // --- Frustum -----------------------------------------------------------
    // OgreNext Frustum ctor requires (IdType, ObjectMemoryManager*).
    // Use SCENE_DYNAMIC since the frustum moves every frame with the cursor.
    m_frustProj = OGRE_NEW Ogre::Frustum(
        Ogre::Id::generateNewId<Ogre::Frustum>(),
        &m_sceneMgr->_getEntityMemoryManager( Ogre::SCENE_DYNAMIC ) );

    m_frustProj->setProjectionType( Ogre::PT_ORTHOGRAPHIC );
    m_frustProj->setNearClipDistance( PROJECTOR_NEAR );
    m_frustProj->setFarClipDistance( PROJECTOR_FAR );

    // --- Projection node ---------------------------------------------------
    // Rotate 90° around X so the frustum's -Z axis points straight down.
    m_nodeProj = m_sceneMgr->getRootSceneNode()->createChildSceneNode();
    m_nodeProj->setOrientation(
        Ogre::Quaternion( Ogre::Degree( 90.0f ), Ogre::Vector3::UNIT_X ) );
    m_nodeProj->attachObject( m_frustProj );

    // Exclude from ray/scene queries so it doesn't interfere with picking.
    m_frustProj->setQueryFlags( 0u );
    m_frustProj->setVisibilityFlags( 0u );

    // --- Decal pass --------------------------------------------------------
    Ogre::Technique *tech = m_terrainMat->getTechnique( 0 );

    // Re-use an existing "Decal" pass if the material already has one
    // (e.g. from a previous DecalCursor instance using the same material).
    m_pass = tech->getPass( "Decal" );
    if( !m_pass )
    {
        m_pass = tech->createPass();
        m_pass->setName( "Decal" );

        // No depth writes — decal sits on top of existing geometry.
        // Positive depth bias to avoid z-fighting with coplanar terrain.
        Ogre::HlmsMacroblock macroblock;
        macroblock.mDepthWrite        = false;
        macroblock.mDepthBiasConstant = 2.5f;
        macroblock.mDepthBiasSlopeScale = 2.5f;
        m_pass->setMacroblock( macroblock );

        // Additive alpha blending so the decal layers over the terrain colour.
        Ogre::HlmsBlendblock blendblock;
        blendblock.setBlendType( Ogre::SBT_TRANSPARENT_ALPHA );
        m_pass->setBlendblock( blendblock );

        // Suppress scene fog so the decal colour is not washed out at distance.
        m_pass->setFog( true, Ogre::FOG_NONE );

        // Opaque base layer required so the alpha compositing is applied
        // correctly when the projective TUS is added.
        m_pass->createTextureUnitState( "decalBase.png" );
    }

    // Initialise size and store texture name (no TUS yet — created lazily in show()).
    m_sTextureName = tex;
    setSize( size );

    m_bVisible = false;
}

void DecalCursor::showTerrainDecal()
{
    if( m_texState || !m_pass )
        return;

    m_texState = m_pass->createTextureUnitState( m_sTextureName );
    m_texState->setProjectiveTexturing( true, m_frustProj );

    // TAM_BORDER with a fully-transparent border colour clips the decal cleanly
    //// at the frustum edges — TAM_CLAMP would smear the edge pixel across the terrain.
    //m_texState->setTextureAddressingMode( Ogre::TextureUnitState::TAM_BORDER );
    //m_texState->setTextureBorderColour( Ogre::ColourValue::ZERO );

    //m_texState->setTextureFiltering( Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_NONE );

    // Additive alpha: result = texture_alpha + current_alpha
    m_texState->setAlphaOperation( Ogre::LBX_ADD, Ogre::LBS_TEXTURE, Ogre::LBS_CURRENT );
}

void DecalCursor::hideTerrainDecal()
{
    if( !m_texState || !m_pass )
        return;

    m_pass->removeTextureUnitState( m_pass->getTextureUnitStateIndex( m_texState ) );
    m_texState = nullptr;
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

auto DecalCursor::getPosition() const noexcept -> Ogre::Vector3
{
    return m_pos;
}

auto DecalCursor::isVisible() const noexcept -> bool
{
    return m_bVisible;
}

auto DecalCursor::getTextureName() const -> Ogre::String
{
    return m_sTextureName;
}

void DecalCursor::setTextureName( const Ogre::String &textureName )
{
    m_sTextureName = textureName;
}

// ---------------------------------------------------------------------------
// Visibility
// ---------------------------------------------------------------------------

void DecalCursor::show()
{
    if( m_bVisible )
        return;

    m_bVisible = true;
    showTerrainDecal();
    setPosition( m_pos );  // sync projection node to the stored position
}

void DecalCursor::hide()
{
    if( !m_bVisible )
        return;

    m_bVisible = false;
    hideTerrainDecal();
}

// ---------------------------------------------------------------------------
// Mutators
// ---------------------------------------------------------------------------

void DecalCursor::setPosition( const Ogre::Vector3 &pos )
{
    m_pos = pos;
    if( m_nodeProj )
        m_nodeProj->setPosition( pos.x, pos.y + PROJECTOR_HEIGHT, pos.z );
}

void DecalCursor::setSize( const Ogre::Vector2 &size )
{
    // Ignore degenerate sizes — an ortho window with zero extent would project
    // nothing and could produce a divide-by-zero in the projection matrix.
    if( size.x <= 0.0f || size.y <= 0.0f )
        return;

    if( m_size == size )
        return;

    m_size = size;

    if( m_frustProj )
        m_frustProj->setOrthoWindow( m_size.x, m_size.y );
}

