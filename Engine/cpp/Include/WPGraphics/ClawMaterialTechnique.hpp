#ifndef ClawMaterialTechnique_h__
#define ClawMaterialTechnique_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/MaterialTechnique.hpp>

namespace workphone::render
{
    /**
     * @class ClawMaterialTechnique
     * @brief Production-ready implementation of IMaterialTechnique for the Claw graphics system.
     *
     * This class implements the logic for a material technique, which is a collection of
     * rendering passes used to render an object under a specific scheme (e.g., High Quality,
     * Low Quality, Depth Only). It is inspired by the Ogre3D material system, allowing
     * materials to have multiple techniques that can be swapped based on hardware capabilities
     * or rendering requirements.
     *
     * @see IMaterialTechnique
     * @see ClawMaterialPass
     */
    class WPGraphics_API ClawMaterialTechnique : public MaterialTechnique
    {
    public:
        /** @brief Default constructor. */
        ClawMaterialTechnique();

        /** @brief Destructor. */
        ~ClawMaterialTechnique() override;

        ClawMaterialTechnique( const ClawMaterialTechnique & ) = delete;
        ClawMaterialTechnique &operator=( const ClawMaterialTechnique & ) = delete;

        /** @copydoc IMaterialTechnique::getScheme */
        hash32 getScheme() const override;

        /** @copydoc IMaterialTechnique::setScheme */
        void setScheme( hash32 scheme ) override;

        /** @copydoc IMaterialTechnique::getNumPasses */
        u32 getNumPasses() const override;

        /** @copydoc IMaterialTechnique::createPass */
        SmartPtr<IMaterialPass> createPass() override;

        /** @copydoc IMaterialTechnique::addPass */
        void addPass( SmartPtr<IMaterialPass> pass ) override;

        /** @copydoc IMaterialTechnique::removePass */
        void removePass( SmartPtr<IMaterialPass> pass ) override;

        /** @copydoc IMaterialTechnique::removePasses */
        void removePasses() override;

        /** @copydoc IMaterialTechnique::getPasses */
        Array<SmartPtr<IMaterialPass>> getPasses() const override;

        /** @copydoc IMaterialTechnique::setPasses */
        void setPasses( Array<SmartPtr<IMaterialPass>> passes ) override;

        /** @copydoc IMaterialTechnique::getPass */
        SmartPtr<IMaterialPass> getPass( u32 index ) const override;

        /**
         * @brief Retrieves properties for the game editor/inspector.
         * @return Smart pointer to a Properties object describing this technique.
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Applies editor-supplied properties to the technique.
         * @param properties Smart pointer to a Properties object.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @copydoc IMaterialNode::getChildObjects */
        Array<SmartPtr<ISharedObject>> getChildObjects() const override;

        /** @copydoc IMaterialNode::handleStateMessage */
        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

        /** @copydoc IMaterialNode::handleStateChanged */
        bool handleStateChanged( SmartPtr<IState> &state ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        hash32 m_scheme = 0;                      ///< Scheme hash identifier for this technique.
        Array<SmartPtr<IMaterialPass>> m_passes;  ///< Collection of passes for this technique.
    };
}  // namespace workphone::render

#endif  // ClawMaterialTechnique_h__
