#ifndef _TranslateManipulator_H
#define _TranslateManipulator_H

#include <Workphone/System/Manipulator.hpp>
#include <Workphone/Math/Cylinder3.hpp>
#include <Workphone/Math/Line3.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /** The TranslateManipulator class is used to translate objects in the scene. */
    class WPCore_API TranslateManipulator : public Manipulator
    {
    public:
        /**
         * Constructor.
         * @param drawDebugLines Whether to use the engine-native debug-line gizmo.
         */
        explicit TranslateManipulator( bool drawDebugLines = false );

        /** Destructor. */
        ~TranslateManipulator() override;

        /** Used to load object.
        @param data The data to load. Expected to be null.
        */
        void load( SmartPtr<ISharedObject> data ) override;

        /** Used to unload object.
        @param data The data to unload. Expected to be null.
        */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** Used to update object. */
        void preUpdate() override;

        /** Used to update object. */
        void update() override;

        /** Used to update object. */
        void postUpdate() override;

        /** Handles the input event. */
        bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

        /** Get the line representing the manipulator's handles. */
        Array<Line3<real_Num>> getLines() const;

        /** Get the cylinder representing the manipulator's handles. */
        Array<Cylinder3<real_Num>> getHandles() const;

        /** Returns whether the engine-native debug-line gizmo is enabled. */
        bool getDrawDebugLines() const;

        /** Enables or disables the engine-native debug-line gizmo. */
        void setDrawDebugLines( bool drawDebugLines );

        WP_CLASS_REGISTER_DECL;

    private:
        /** Shows or hides all engine-native debug lines owned by this manipulator. */
        void setDebugLinesVisible( bool visible );

        // The axis around which the translation takes place.
        Vector3<real_Num> m_vAxis;

        // The start position of the gizmo's movement.
        Vector3<real_Num> m_vStart;

        // The previous mouse position.
        Vector2<real_Num> m_previousCursorPosition;

        // The current mouse position.
        Vector2<real_Num> m_currentCursorPosition;

        // The radius of the gizmo's handles.
        f32 m_handleRadius = 0.1f;

        // The height of the gizmo's handles.
        f32 m_handleHeight = 3.0f;

        // Scales the distance the camera moves when the gizmo is moved.
        f32 m_translateReduction = 0.02f;

        // Scales the distance the camera moves when the gizmo is moved.
        f32 m_cameraDistanceScale = 0.2f;

        // Bool to indicate whether or not the mouse button is down.
        bool m_bMouseButtonDown = false;

        // Whether to render and interact with the engine-native debug-line gizmo.
        bool m_drawDebugLines = false;

        // The lines representing the manipulator's axis.
        Array<SmartPtr<render::IDebugLine>> m_lines;

        // Booleans to indicate whether or not the axis is selected.
        Array<bool> m_selected;
    };
}  // namespace workphone

#endif
