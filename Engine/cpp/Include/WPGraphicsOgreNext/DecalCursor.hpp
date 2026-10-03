/**
 * @file DecalCursor.hpp
 * @author Jesse Wright - www.cutthroatstudios.com
 * @note Modified from Brocan's example on the Ogre forums
 * @note Reimplemented for OgreNext (2.x) API
 */

#ifndef SANGUIS_DECALCURSOR_H
#define SANGUIS_DECALCURSOR_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <OgreMaterial.h>
#include <OgreVector3.h>
#include <OgreVector2.h>

/**
 * @class DecalCursor
 * @brief Projects a texture decal onto terrain using an orthographic frustum.
 *
 * A scene node is created 10 units above the target position, oriented to
 * face downward. An orthographic @c Ogre::Frustum attached to that node
 * drives projective texturing on a dedicated pass that is added to the
 * supplied terrain material.
 *
 * The decal pass (named "Decal") is created once in the terrain material's
 * first technique. The projective texture unit state is lazily added on the
 * first call to @c show() and removed on @c hide(), so the GPU cost is zero
 * while the cursor is not visible.
 */
class DecalCursor
{
public:
    /**
     * @brief Constructs the decal cursor and registers a pass in @p terrainMat.
     * @param man        The scene manager that owns the projection node.
     * @param terrainMat Material whose first technique receives the decal pass.
     * @param size       World-space width (x) and height (y) of the projected area.
     * @param tex        Name of the decal texture resource.
     * @pre  @p man is not null.
     * @pre  @p terrainMat is not null and has at least one technique.
     */
    DecalCursor( Ogre::SceneManager *man, Ogre::MaterialPtr terrainMat,
                 const Ogre::Vector2 &size, const std::string &tex );

    /** @brief Destroys the frustum, scene node, and removes the decal pass. */
    ~DecalCursor();

    // Non-copyable / non-movable — owns raw Ogre resources.
    DecalCursor( const DecalCursor & ) = delete;
    DecalCursor &operator=( const DecalCursor & ) = delete;
    DecalCursor( DecalCursor && ) = delete;
    DecalCursor &operator=( DecalCursor && ) = delete;

    /** @return The last position set via setPosition(). */
    [[nodiscard]] Ogre::Vector3 getPosition() const noexcept;

    /** @brief Makes the decal visible and positions the projection node. */
    void show();

    /** @brief Hides the decal and removes the texture unit state from the pass. */
    void hide();

    /** @return @c true if the decal is currently visible. */
    [[nodiscard]] bool isVisible() const noexcept;

    /**
     * @brief Moves the projection node to the given world position.
     *
     * The frustum node is automatically offset upward by a fixed amount so
     * the orthographic volume covers the terrain beneath it.
     */
    void setPosition( const Ogre::Vector3 &pos );

    /**
     * @brief Changes the world-space footprint of the decal.
     * @param size New width (x) and height (y). Both components must be > 0.
     *             A zero or negative value is silently ignored.
     */
    void setSize( const Ogre::Vector2 &size );

    /** @return The current decal texture name. */
    [[nodiscard]] Ogre::String getTextureName() const;

    /** @brief Replaces the decal texture used on the next @c show(). */
    void setTextureName( const Ogre::String &textureName );

private:
    /**
     * @brief One-time setup: creates the frustum, scene node, and decal pass.
     * @param size Initial projected area size.
     * @param tex  Decal texture name.
     */
    void init( const Ogre::Vector2 &size, const std::string &tex );

    /** @brief Lazily creates and configures the projective texture unit state. */
    void showTerrainDecal();

    /** @brief Removes the projective texture unit state from the pass. */
    void hideTerrainDecal();

    Ogre::Vector2 m_size;           ///< Projected area size in world units.
    Ogre::Vector3 m_pos;            ///< Last requested world position.

    Ogre::String m_sTextureName;    ///< Decal texture resource name.

    Ogre::SceneNode        *m_nodeProj;   ///< Scene node that carries the frustum.
    Ogre::Frustum          *m_frustProj;  ///< Orthographic projection frustum.
    Ogre::TextureUnitState *m_texState;   ///< Projective TUS; null when hidden.
    Ogre::Pass             *m_pass;       ///< The "Decal" pass in the terrain material.
    Ogre::MaterialPtr       m_terrainMat; ///< Terrain material that receives the pass.

    Ogre::SceneManager     *m_sceneMgr;   ///< Owning scene manager (not owned).

    bool m_bVisible; ///< Visibility state.
};

#endif  // SANGUIS_DECALCURSOR_H
