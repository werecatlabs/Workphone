#ifndef CResource_h__
#define CResource_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/UI/IUIManager.hpp>
#include <Workphone/System/Prototype.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{

    /** Resource class. A template class to provide the basic functionality of a resource. */
    template <class T>
    class Resource : public core::Prototype<T>
    {
    public:
        /** Constructor. */
        Resource();

        Resource( u32 poolTypeId );

        /** Destructor. */
        ~Resource() override;

        /** Unloads the resource.
         * @copydoc IResource::unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** Saves the resource to a file. */
        void saveToFile( const String &filePath ) override;

        /** Loads the resource from a file. */
        void loadFromFile( const String &filePath ) override;

        /** Saves the resource.
         * @copydoc IResource::save
         */
        void save() override;

        /** Imports the resource.
         * @copydoc IResource::import
         */
        void import() override;

        /** Reimport the resource. */
        void reimport() override;

        /** Gets the id of the resource in the system. */
        UUID getFileSystemId() const override;

        /** Sets the id of the resource stored in the system. */
        void setFileSystemId( UUID id ) override;

        /** Gets the file path.
         * @return The file path.
         */
        String getFilePath() const override;

        /** Sets the file path.
         * @param filePath The file path.
         */
        void setFilePath( const String &filePath ) override;

        /** Gets the id of the resource settings in the system. */
        UUID getSettingsFileSystemId() const override;

        /** Sets the id of the resource settings stored in the system.
         * @param id The id of the resource settings.
         */
        void setSettingsFileSystemId( UUID id ) override;

        /** Converts the resource to data. */
        SmartPtr<ISharedObject> toData() const override;

        /** Converts the resource from data. */
        void fromData( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IResource::getProperties */
        SmartPtr<Properties> getProperties() const override;

        /** @copydoc IResource::setProperties */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @brief Gets the object. Return nullptr.
         * @copydoc IResource::_getObject
         */
        void _getObject( void **ppObject ) const override;

        /** @copydoc IResource::getDependencies */
        Array<SmartPtr<IResource>> getDependencies() const override;

        IResourceManager *getResourceManagerPtr() const override;
        SmartPtr<IResourceManager> getResourceManager() const override;
        void setResourceManager( SmartPtr<IResourceManager> resourceManager ) override;

        IStateContext *getStateContextPtr() const override;
        SmartPtr<IStateContext> getStateContext() const override;

        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
        bool handleStateChanged( SmartPtr<IState> &state ) override;

        WP_CLASS_REGISTER_TEMPLATE_DECL( Resource, T );

    protected:
        AtomicSmartPtr<IResourceManager> m_resourceManager;
        AtomicObject<UUID> m_fileSystemId;
        AtomicObject<UUID> m_settingsFileSystemId;
        AtomicObject<FixedString<1024>> m_filePath;
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, Resource, T, core::Prototype<T> );

    template <class T>
    Resource<T>::Resource() : core::Prototype<T>( T::typeInfo() )
    {
    }

    template <class T>
    Resource<T>::Resource( u32 poolTypeId ) : core::Prototype<T>( poolTypeId )
    {
    }

    template <class T>
    Resource<T>::~Resource() = default;

    template <class T>
    void Resource<T>::unload( SmartPtr<ISharedObject> data )
    {
    }

    template <class T>
    void Resource<T>::saveToFile( const String &filePath )
    {
    }

    template <class T>
    void Resource<T>::loadFromFile( const String &filePath )
    {
    }

    template <class T>
    void Resource<T>::save()
    {
    }

    template <class T>
    void Resource<T>::import()
    {
    }

    template <class T>
    void Resource<T>::reimport()
    {
    }

    template <class T>
    UUID Resource<T>::getFileSystemId() const
    {
        return m_fileSystemId;
    }

    template <class T>
    void Resource<T>::setFileSystemId( UUID id )
    {
        m_fileSystemId = id;
    }

    template <class T>
    String Resource<T>::getFilePath() const
    {
        auto s = m_filePath.load();
        return s.str();
    }

    template <class T>
    void Resource<T>::setFilePath( const String &filePath )
    {
        m_filePath = FixedString<1024>( filePath.begin(), filePath.end() );
    }

    template <class T>
    UUID Resource<T>::getSettingsFileSystemId() const
    {
        return m_settingsFileSystemId;
    }

    template <class T>
    void Resource<T>::setSettingsFileSystemId( UUID id )
    {
        m_settingsFileSystemId = id;
    }

    template <class T>
    SmartPtr<ISharedObject> Resource<T>::toData() const
    {
        auto properties = getProperties();
        if( auto p = workphone::dynamic_pointer_cast<ISharedObject>( properties ) )
        {
            return p;
        }

        return nullptr;
    }

    template <class T>
    void Resource<T>::fromData( SmartPtr<ISharedObject> data )
    {
        if( auto properties = workphone::dynamic_pointer_cast<Properties>( data ) )
        {
            setProperties( properties );
        }
    }

    template <class T>
    SmartPtr<Properties> Resource<T>::getProperties() const
    {
        auto properties = core::Prototype<T>::getProperties();

        auto name = Resource<T>::getName();
        properties->setProperty( IResource::nameStr, name );

        return properties;
    }

    template <class T>
    void Resource<T>::setProperties( SmartPtr<Properties> properties )
    {
        core::Prototype<T>::setProperties( properties );

        auto name = String();
        properties->getPropertyValue( IResource::nameStr, name );

        Resource<T>::setName( name );
    }

    template <class T>
    void Resource<T>::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    template <class T>
    Array<SmartPtr<IResource>> Resource<T>::getDependencies() const
    {
        return Array<SmartPtr<IResource>>();
    }

    template <class T>
    IResourceManager *Resource<T>::getResourceManagerPtr() const
    {
        return m_resourceManager.get();
    }

    template <class T>
    SmartPtr<IResourceManager> Resource<T>::getResourceManager() const
    {
        return m_resourceManager.load();
    }

    template <class T>
    void Resource<T>::setResourceManager( SmartPtr<IResourceManager> resourceManager )
    {
        m_resourceManager = resourceManager;
    }

    template <class T>
    IStateContext *Resource<T>::getStateContextPtr() const
    {
        if( auto resourceManager = getResourceManagerPtr() )
        {
            return resourceManager->getStateContextPtr();
        }

        return nullptr;
    }

    template <class T>
    SmartPtr<IStateContext> Resource<T>::getStateContext() const
    {
        if( auto resourceManager = getResourceManagerPtr() )
        {
            return resourceManager->getStateContext();
        }

        return nullptr;
    }

    template <class T>
    bool Resource<T>::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    template <class T>
    bool Resource<T>::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

}  // namespace workphone

#endif  // CResource_h__
