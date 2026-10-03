#ifndef CMaterialTechnique_h__
#define CMaterialTechnique_h__

#include <Workphone/Graphics/MaterialNode.hpp>
#include <Workphone/Interface/Graphics/IMaterialTechnique.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Concrete implementation of the IMaterialTechnique interface.
         *
         * A MaterialTechnique represents a rendering technique for a material.
         * It acts as a container for an ordered set of material passes
         * (IMaterialPass) and an associated rendering scheme identifier. A
         * technique groups together one or more passes that collectively
         * describe how a material should be rendered for a particular
         * rendering scheme (for example "forward" or "deferred").
         *
         * This class derives from MaterialNode<IMaterialTechnique> which
         * provides state management, notification hooks and (de)serialization
         * support.
         *
         * @note Instances typically own their pass list and manage pass
         *       lifetimes via SmartPtr.
         * @see IMaterialTechnique, IMaterialPass, MaterialNode
         */
        class WPCore_API MaterialTechnique : public MaterialNode<IMaterialTechnique>
        {
        public:
            static const String nameStr;
            static const String schemeStr;
            static const String passStr;

            /**
             * @brief Listener for technique state changes and messages.
             *
             * The MaterialTechniqueStateListener receives state messages and
             * change notifications emitted by the MaterialNode base class and
             * either forwards them or performs technique-specific handling.
             */
            class MaterialTechniqueStateListener : public MaterialNodeStateListener
            {
            public:
                MaterialTechniqueStateListener();
                ~MaterialTechniqueStateListener() override;

                /**
                 * @brief Process an incoming state message.
                 *
                 * Implementors should inspect the provided @p message and
                 * perform any technique-specific handling. Returning true
                 * indicates the message was handled and no further
                 * processing is required.
                 *
                 * @param message The state message to process.
                 * @return true if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Notify that a state object has changed.
                 *
                 * Called when a state object associated with this technique
                 * has been modified. Implementations may update cached
                 * derived data or trigger downstream notifications.
                 *
                 * @param state The state object that changed.
                 * @return true if the change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            /**
             * @brief Construct a new MaterialTechnique.
             *
             * Initializes the technique with an empty pass list and the
             * default scheme identifier (0).
             */
            MaterialTechnique();

            /**
             * @brief Destroy the MaterialTechnique.
             *
             * Releases owned pass containers and performs any necessary
             * cleanup.
             */
            ~MaterialTechnique() override;

            /**
             * @brief Load technique state from a serialized object.
             *
             * Called when constructing a technique from previously saved
             * data. The provided @p data object is expected to contain the
             * full technique state (scheme id, pass definitions, properties,
             * etc.). This will replace the current technique state.
             *
             * @param data Serialized representation containing technique
             *             state.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reload technique state from @p data.
             *
             * Performs a refresh of the technique using the supplied
             * serialized representation. This is intended for runtime updates
             * (for example when material files are edited and reloaded).
             *
             * @param data Serialized representation to reload from.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload technique resources.
             *
             * Releases runtime resources associated with this technique and
             * its passes. After calling unload the technique will be in a
             * clean state and may be reinitialized by calling @c load or
             * @c reload.
             *
             * @param data Optional context or serialized object to guide
             *             unloading behaviour.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the rendering scheme identifier for this technique.
             *
             * The scheme identifier is a 32-bit hash used to select rendering
             * code paths or variants that this technique targets.
             *
             * @return hash32 Scheme identifier.
             */
            hash32 getScheme() const override;

            /**
             * @brief Set the rendering scheme identifier for this technique.
             *
             * @param scheme 32-bit hash value representing the scheme.
             */
            void setScheme( hash32 scheme ) override;

            /**
             * @brief Return the number of passes in this technique.
             *
             * @return u32 Number of contained passes.
             */
            u32 getNumPasses() const override;

            /**
             * @brief Create and return a new material pass instance.
             *
             * The caller receives ownership via SmartPtr and should add the
             * pass to this technique using @c addPass or replace the entire
             * pass list with @c setPasses.
             *
             * @return SmartPtr<IMaterialPass> Newly created material pass.
             */
            SmartPtr<IMaterialPass> createPass() override;

            /**
             * @brief Add an existing material pass to this technique.
             *
             * If the same pass instance is already present, behaviour is
             * implementation-defined (typically it will be ignored or a new
             * reference will be stored).
             *
             * @param pass Pass to add.
             */
            void addPass( SmartPtr<IMaterialPass> pass ) override;

            /**
             * @brief Remove the specified pass from this technique.
             *
             * If @p pass is not present in the technique this call has no
             * effect.
             *
             * @param pass Pass to remove.
             */
            void removePass( SmartPtr<IMaterialPass> pass ) override;

            /**
             * @brief Remove and release all passes from the technique.
             *
             * After calling this method the technique will contain no passes.
             */
            void removePasses() override;

            /**
             * @brief Retrieve a copy of the technique's pass list.
             *
             * The returned array contains SmartPtr references to the passes
             * owned by the technique. The array itself is a copy and modifying
             * it will not affect the technique's internal storage.
             *
             * @return Array<SmartPtr<IMaterialPass>> Copy of the pass list.
             */
            Array<SmartPtr<IMaterialPass>> getPasses() const override;

            /**
             * @brief Replace the technique's pass list.
             *
             * The @p passes array is copied into the technique's internal
             * storage; ownership of each pass is managed via SmartPtr.
             *
             * @param passes New pass list to assign.
             */
            void setPasses( Array<SmartPtr<IMaterialPass>> passes ) override;

            /**
             * @brief Get the pass at the specified index.
             *
             * No bounds checking is performed; the caller must ensure @p index
             * is valid (0 <= index < getNumPasses()).
             *
             * @param index Index of the pass to retrieve.
             * @return SmartPtr<IMaterialPass> Pass at the specified index.
             */
            SmartPtr<IMaterialPass> getPass( u32 index ) const;

            /** @copydoc MaterialNode<IMaterialTechnique>::toData */
            SmartPtr<ISharedObject> toData() const override;

            /** @copydoc MaterialNode<IMaterialTechnique>::fromData */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /** @copydoc MaterialNode<IMaterialTechnique>::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc MaterialNode<IMaterialTechnique>::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc MaterialNode<IMaterialTechnique>::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Forward a state message to this technique instance.
             *
             * These methods provide a public mechanism (in addition to the
             * listener) for invoking technique-specific state handling.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @copydoc MaterialNode<IMaterialTechnique>::handleStateChanged
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Scheme identifier (32-bit hash) for this technique. Default = 0.
            hash32 m_scheme = 0;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // CMaterialTechnique_h__
