#ifndef __WP_CDirector_h__
#define __WP_CDirector_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>

namespace workphone
{
    /**
     * @class Director
     * @brief Implements a scene graph director for managing hierarchical scene objects and their
     * properties.
     *
     * The Director class is responsible for managing a hierarchy of scene directors, supporting
     * parent-child relationships, property management, and resource serialization. It extends
     * Resource<IBuildDirector>, providing loading, saving, and property serialization capabilities.
     * Directors are used to organize and control scene objects, their configuration, and their
     * relationships in a thread-safe manner.
     *
     * @see IBuildDirector, Resource, Properties, SmartPtr, ConcurrentArray
     */
    class WPCore_API Director : public Resource<IBuildDirector>
    {
    public:
        static const String versionStr;
        static const String objectTypeStr;

        /**
         * @brief Constructs a new Director instance.
         *
         * Initializes the director with no parent and an empty set of children and properties.
         */
        Director();

        /**
         * @brief Destructor. Cleans up resources and child relationships.
         */
        ~Director() override;

        /**
         * @copydoc IBuildDirector::load
         * @brief Loads the director from the provided shared object data.
         * @param data Shared object containing data to load.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IBuildDirector::unload
         * @brief Unloads the director, releasing resources and clearing children.
         * @param data Shared object containing data to unload.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Saves the director's state to a file.
         * @param filePath Path to the file where the director will be saved.
         */
        void saveToFile( const String &filePath ) override;

        /**
         * @brief Loads the director's state from a file.
         * @param filePath Path to the file from which to load the director.
         */
        void loadFromFile( const String &filePath ) override;

        /**
         * @copydoc IBuildDirector::save
         * @brief Saves the director's state to persistent storage or resource system.
         */
        void save() override;

        /**
         * @copydoc IBuildDirector::toData
         * @brief Serializes the director to a shared object for storage or transfer.
         * @return SmartPtr<ISharedObject> Serialized data representing the director.
         */
        SmartPtr<ISharedObject> toData() const override;

        /**
         * @copydoc IBuildDirector::fromData
         * @brief Deserializes the director from a shared object.
         * @param data Shared object containing serialized director data.
         */
        void fromData( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IBuildDirector::getProperties
         * @brief Gets the hierarchical property bag for this director.
         * @return SmartPtr<Properties> The properties associated with this director.
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @copydoc IBuildDirector::setProperties
         * @brief Sets the hierarchical property bag for this director.
         * @param properties The properties to associate with this director.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        /**
         * @copydoc IBuildDirector::setupHeaderProperties
         * @brief Configures the initial header properties for this director (e.g., metadata, type
         * info).
         * @param properties The properties object to populate with header information.
         */
        void setupHeaderProperties( SmartPtr<Properties> properties ) const override;

        /**
         * @copydoc IBuildDirector::getParent
         * @brief Gets the parent director in the scene hierarchy.
         * @return SmartPtr<IBuildDirector> The parent director, or nullptr if this is a root
         * director.
         */
        SmartPtr<IBuildDirector> getParent() const override;

        /**
         * @copydoc IBuildDirector::setParent
         * @brief Sets the parent director in the scene hierarchy.
         * @param parent The parent director to set.
         */
        void setParent( SmartPtr<IBuildDirector> parent ) override;

        /**
         * @copydoc IBuildDirector::addChild
         * @brief Adds a child director to this director's children collection.
         * @param child The child director to add.
         */
        void addChild( SmartPtr<IBuildDirector> child ) override;

        /**
         * @copydoc IBuildDirector::removeChild
         * @brief Removes a specific child director from this director.
         * @param child The child director to remove.
         */
        void removeChild( SmartPtr<IBuildDirector> child ) override;

        /**
         * @copydoc IBuildDirector::removeChildren
         * @brief Removes all child directors from this director.
         */
        void removeChildren() override;

        /**
         * @copydoc IBuildDirector::findChild
         * @brief Finds a child director by name.
         * @param name The name of the child director to find.
         * @return SmartPtr<IBuildDirector> The found child director, or nullptr if not found.
         */
        SmartPtr<IBuildDirector> findChild( const String &name ) override;

        /**
         * @copydoc IBuildDirector::getChildren
         * @brief Gets an array of all child directors.
         * @return Array<SmartPtr<IBuildDirector>> Array containing all child directors.
         */
        Array<SmartPtr<IBuildDirector>> getChildren() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Populates the properties object with button-related properties for this director.
         * @param properties The properties object to populate.
         */
        void getButtons( SmartPtr<Properties> properties ) const;

        /**
         * @brief Sets button-related properties for this director from the given properties object.
         * @param properties The properties object containing button data.
         */
        void setButtons( SmartPtr<Properties> properties );

        /// The parent director in the scene hierarchy.
        SmartPtr<IBuildDirector> m_parent;

        /// The thread-safe collection of child directors.
        ConcurrentArray<SmartPtr<IBuildDirector>> m_children;
    };

}  // namespace workphone

#endif  // __WP_CDirector_h__
