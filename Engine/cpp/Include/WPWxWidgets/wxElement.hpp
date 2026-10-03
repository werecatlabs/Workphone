#ifndef _FB_WxElement_H
#define _FB_WxElement_H

#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace ui
    {

        //-------------------------------------------------
        /** A base gui element.
         */
        template <class T>
        class wxElement : public T
        {
        public:
            /** Default constructor. */
            CUIElement();

            /** Virtual destructor. */
            virtual ~CUIElement();

            /** Used to initialise the gui element with default values. */
            virtual void initialise( SmartPtr<IUIElement> &parent );

            /** Used to initialise the gui element. */
            virtual void initialise( SmartPtr<IUIElement> &parent, const Properties &properties );

            /** Called the update the gui item. */
            virtual void update();

            /** Handles a gui element message. */
            virtual void handleMessage( SmartPtr<IMessage> message );

            /** Called when an input event has occurred. */
            virtual bool onEvent( const SmartPtr<IInputEvent> &event );

            /** Sets the id of the gui item. */
            void setName( const String &name );

            /** Gets the id of the gui item. */
            const String &getName() const;

            /** Gets the hash id of the gui element. */
            u32 getHashId() const;

            /** Sets the component id. */
            void setComponentID( const String &componentId );

            /** Gets the component id. */
            const String &getComponentID() const;

            /** Gets the type of the gui item. */
            const String &getType() const;

            /** Sets the position of the gui item. */
            virtual void setPosition( const Vector2F &position );

            /** Returns the position of the gui item. */
            Vector2F getPosition() const;

            /** Gets the absolute position of the gui item. */
            Vector2F getAbsolutePosition() const;

            /** Sets the size of the gui item. */
            virtual void setSize( const Vector2F &size );

            /** Gets the size of the gui item. */
            Vector2F getSize() const;

            /** Sets whether this gui item is enabled. */
            void setEnabled( bool enabled, bool cascade = true );

            /** Sets whether this gui item is enabled. */
            bool isEnabled() const;

            /** Sets whether this gui item is selected. */
            void setSelected( bool selected );

            /** Checks if this gui item is selected. */
            bool isSelected() const;

            /** Sets whether the gui item is visible or not. */
            virtual void setVisible( bool isVisible, bool cascade = true );

            /** Returns a boolean value indicating whether or not the ui item is visible. */
            bool isVisible() const;

            /** Sets whether this gui item is in focus. */
            virtual void setFocus( bool hasFocus );

            /** Checks if the this gui item is in focus. */
            bool isInFocus() const;

            /** Sets this gui item as being highlighted. */
            void setHighlighted( bool isHighlighted, bool cascade = true );

            /** Check if this gui item is highlighted. */
            bool isHighlighted() const;

            SmartPtr<IUIElement> getParent() const
            {
                return m_parent;
            }
            void setParent( SmartPtr<IUIElement> parent )
            {
                m_parent = parent;
            }

            /** Gets the children of the gui item. */
            const Array<SmartPtr<IUIElement>> &getChildren() const;

            /** Adds a child to this gui item. */
            void addChild( SmartPtr<IUIElement> pGUIItem ) override;

            /** Removes a child of the gui item. */
            bool removeChild( SmartPtr<IUIElement> pGUIItem ) override;

            /** Removes this element. */
            void remove();

            /** Removes a the children of this gui item. */
            void removeAllChildren();

            /** Check if the check item has a child. */
            virtual bool hasChildById( const String &id ) const;

            /** Find a child by id. */
            virtual SmartPtr<IUIElement> findChildById( const String &id ) const;

            /** Finds a component by id. */
            SmartPtr<IUIElement> findChildByComponentId( const String &componentId ) const;

            SmartPtr<IUIElement> getLayout() const;
            void setLayout( SmartPtr<IUIElement> layout );

            /** Sets the attached user data.*/
            void setUserData( void *userData );

            /** Gets the user data attached. */
            void *getUserData() const;

            /** Adds a gui item listener. */
            void addGUIItemListener( IUIElementListener *listener );

            /** Removes a gui item listener. */
            bool removeGUIItemListener( IUIElementListener *listener );

            /** Finds a gui item listener. */
            SmartPtr<IUIElementListener> findGUIItemListener( const String &id ) const;

            /** Adds an input listener. */
            void addInputListener( IUIItemInputListener *listener );

            /** Removes an input listener. */
            bool removeInputListener( IUIItemInputListener *listener );

            /** Finds an input listener. */
            IUIItemInputListener *findInputListener( const String &id );

            /** Adds an animator to the container. */
            void addAnimator( SmartPtr<IAnimator> &animator );

            /** Removes an animator from the container. */
            bool removeAnimator( SmartPtr<IAnimator> &animator );

            SmartPtr<IUIContainer> &getContainer();
            const SmartPtr<IUIContainer> &getContainer() const;
            void setContainer( SmartPtr<IUIContainer> container );

            void _getObject( void **ppObject ) const;

            //
            //IScriptObjects
            //

            /** Gets an object call script functions. */
            virtual SmartPtr<IScriptInvoker> &getInvoker();

            /** Gets an object call script functions. */
            virtual const SmartPtr<IScriptInvoker> &getInvoker() const;

            /** Sets an object call script functions. */
            virtual void setInvoker( SmartPtr<IScriptInvoker> invoker );

            /** Gets an object to receive script calls. */
            virtual SmartPtr<IScriptReceiver> &getReceiver();

            /** Gets an object to receive script calls. */
            virtual const SmartPtr<IScriptReceiver> &getReceiver() const;

            /** Sets an object to receive script calls. */
            virtual void setReceiver( SmartPtr<IScriptReceiver> receiver );

            /** Internal function used by the script system. */
            virtual void _setData( SmartPtr<IScriptData> data );

            /** Internal function used by the script system. */
            virtual SmartPtr<IScriptData> _getData() const;

            /** Sets a property. */
            virtual s32 setProperty( hash32 hash, const String &value );

            /** Gets a property. */
            virtual s32 getProperty( hash32 hash, String &value ) const;

            /** Sets a property. */
            virtual s32 setProperty( hash32 hash, const Parameter &param );

            /** Sets a property. */
            virtual s32 setProperty( hash32 hash, const Parameters &params );

            /** Sets a property. */
            virtual s32 setProperty( hash32 hash, void *param );

            /** Gets a property. */
            virtual s32 getProperty( hash32 hash, Parameter &param );

            /** Gets a property. */
            virtual s32 getProperty( hash32 hash, Parameters &params ) const;

            /** Gets a property. */
            virtual s32 getProperty( hash32 hash, void *param ) const;

            /** Overridden from IScriptObject*/
            virtual s32 getObject( u32 hash, SmartPtr<ISharedObject> &object ) const;

            /** Calls a function. */
            virtual s32 callFunction( u32 hash, const Parameters &params, Parameters &results );

            /** Calls a function. */
            virtual s32 callFunction( u32 hash, SmartPtr<ISharedObject> object, Parameters &results );

            /** Internal function. */
            virtual void _onInitialiseStart();

            /** Internal function. */
            virtual void _onInitialiseEnd();

            //
            //Events
            //

            virtual void onAddChild( IUIElement *child );
            virtual void onRemoveChild( IUIElement *child );
            virtual void onChangedState();
            virtual void onChildChangedState( IUIElement *child );
            virtual void onToggleEnabled();
            virtual void onToggleVisibility();
            virtual void onToggleHighlight();
            virtual void onActivate( SmartPtr<IUIElement> element );
            virtual void onDeactivate();
            virtual void onSelect();
            virtual void onDeselect();
            virtual void onGainFocus();
            virtual void onLostFocus();

            virtual void handleEvent( const SmartPtr<IEvent> &event );

        protected:
            //general event
            virtual void onEvent( const String &eventType );

            void setType( const String &type );

            SmartPtr<IUIContainer> m_container;

            SmartPtr<IScriptInvoker> m_scriptInvoker;

            SmartPtr<IScriptReceiver> m_scriptReceiver;

            /// The data used by the script system.
            SmartPtr<IScriptData> m_scriptData;

            /// The position of the gui element.
            Vector2F m_position;

            /// The size of the gui element.
            Vector2F m_size;

            /// The parent gui element.
            SmartPtr<IUIElement> m_parent;

            /// The attached user data.
            void *m_userData;

            SmartPtr<IUIElement> m_layout;

            /// Used to store user properties.
            mutable SmartPtr<ISharedObject> m_userProperties;

            /// Used to know if the element is enabled.
            bool m_isEnabled;

            /// To know if the element is visible.
            bool m_isVisible;

            /// To know if the element has focus.
            bool m_hasFocus;

            /// To know if the element is highlighted.
            bool m_isHighlighted;

            /// To know if the element is selected.
            bool m_isSelected;

            /// The id of the element.
            String m_name;

            /// The hashed id of the element.
            u32 m_hashId;

            /// The id of the component.
            String m_componentId;

            /// The type of gui element.
            String m_type;

            /// The children of the gui element.
            Array<SmartPtr<IUIElement>> m_children;

            /// The gui element listeners.
            Array<IUIElementListener *> m_listeners;

            /// The gui element input listeners.
            Array<IUIItemInputListener *> m_inputListeners;

            /// The number of the next name extension.
            static u32 m_nextGeneratedNameExt;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // _FB_WxElement_H
