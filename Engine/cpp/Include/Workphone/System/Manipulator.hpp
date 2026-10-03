#ifndef Manipulator_h__
#define Manipulator_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    /** Manipulator base class. */
    class WPCore_API Manipulator : public ISharedObject
    {
    public:
        /** Listens to the selection manager. */
        class SelectionManagerListener : public IEventListener
        {
        public:
            /** Constructor. */
            SelectionManagerListener();

            /** Destructor. */
            ~SelectionManagerListener() override;

            /** Handles the event. */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /** Adds the selected object. */
            void addSelectedObject();

            /** Adds the selected objects. */
            void addSelectedObjects();

            /** Sets the selected object. */
            void setSelectedObjects();

            /** Deselects the selected object. */
            void deselectObjects();

            /** Deselects all the selected objects. */
            void deselectAll();

            /** Gets the owner of the listener. */
            SmartPtr<Manipulator> getOwner() const;

            /** Sets the owner of the listener. */
            void setOwner( SmartPtr<Manipulator> owner );

            WP_CLASS_REGISTER_DECL;

        private:
            /** The owner of the listener. */
            SmartPtr<Manipulator> m_owner;
        };

        /** Manipulator axis type. */
        enum AxisType
        {
            AT_X = 0,
            AT_Y = 1,
            AT_Z = 2,
            AT_XY = 3,
            AT_XZ = 4,
            AT_YZ = 5,
            AT_LAST = 6
        };

        /** Constructor. */
        Manipulator();

        /** Destructor. */
        ~Manipulator() override;

        /** Used to load object.
        @param data The data to load. Expected to be null.
        */
        void load( SmartPtr<ISharedObject> data ) override;

        /** Used to unload object.
        @param data The data to unload. Expected to be null.
        */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** Used to update object. */
        void update() override;

        /** Handles the input event. */
        virtual bool handleEvent( const SmartPtr<IInputEvent> &event );

        /** Enables or disables the manipulator. */
        void setEnabled( bool bEnabled );

        /** Returns true if the manipulator is enabled. */
        bool isEnabled();

        /** Returns true if the manipulator is visible. */
        bool isVisible() const;

        /** Sets whether or not the manipulator is visible. */
        void setVisible( bool visible );

        /** Gets the manipulator's position. */
        Vector3<real_Num> getPosition() const;

        /** Sets the manipulator's position. */
        void setPosition( const Vector3<real_Num> &position );

        /** Gets rotation of the manipulator. */
        Vector3<real_Num> getRotation() const;

        /** Sets rotation of the manipulator. */
        void setRotation( const Vector3<real_Num> &rotation );

        /** Gets the manipulator's scale. */
        Vector3<real_Num> getScale() const;

        /** Sets the manipulator's scale. */
        void setScale( const Vector3<real_Num> &scale );

        /** Updates the manipulator position. */
        virtual void updateManipulatorPosition();

        WP_CLASS_REGISTER_DECL;

    protected:
        // The selection manager listener.
        SmartPtr<SelectionManagerListener> m_selectionManagerListener;

        // Relative translation of the scene node.
        Vector3<real_Num> m_relativeTranslation;

        // Relative rotation of the scene node.
        Vector3<real_Num> m_relativeRotation;

        // Relative scale of the scene node.
        Vector3<real_Num> m_relativeScale;

        // Store the gizmo's position
        Vector3<real_Num> m_vPosition;

        // The scale of the manipulator components.
        real_Num m_fScale = real_Num( 1.0 );

        // Boolean to know whether or not the gizmo should be shown.
        atomic_bool m_enabled = false;

        // Boolean to know whether or not the gizmo should be shown.
        bool m_visible = false;
    };

}  // namespace workphone

#endif  // Manipulator_h__
