#ifndef IOverlayElement_h__
#define IOverlayElement_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IOverlayElement
         * @brief Interface for a renderable element inside an overlay.
         *
         * Overlay elements represent the individual pieces of screen-space UI,
         * such as panels, images, labels, and element containers. Implementations
         * expose layout, visibility, material, colour, and hierarchy state while
         * hiding the renderer-specific object behind @c _getObject.
         */
        class WPCore_API IOverlayElement : public ISharedObject
        {
        public:
            /// Property key for the material name used by getProperties/setProperties.
            static const String materialNameStr;
            /// Property key for the colour tint used by getProperties/setProperties.
            static const String colourStr;
            /// Property key for the element position used by getProperties/setProperties.
            static const String positionStr;
            /// Property key for the element size used by getProperties/setProperties.
            static const String sizeStr;
            /// Property key for the element z-order used by getProperties/setProperties.
            static const String zorderStr;
            /// Property key for the element visibility used by getProperties/setProperties.
            static const String visibleStr;

            /**
             * @brief Units used when interpreting element position and size.
             */
            enum GuiMetricsMode
            {
                /// Position and size are normalized from 0.0 to 1.0.
                GMM_RELATIVE,
                /// Position and size are expressed in absolute pixels.
                GMM_PIXELS,
                /// Position and size are relative and adjusted for aspect ratio.
                GMM_RELATIVE_ASPECT_ADJUSTED
            };

            /**
             * @brief Horizontal origin used when interpreting the element's left coordinate.
             */
            enum GuiHorizontalAlignment
            {
                /// The left coordinate is measured from the parent's left edge.
                GHA_LEFT,
                /// The left coordinate is measured from the parent's horizontal center.
                GHA_CENTER,
                /// The left coordinate is measured from the parent's right edge.
                GHA_RIGHT
            };

            /**
             * @brief Vertical origin used when interpreting the element's top coordinate.
             */
            enum GuiVerticalAlignment
            {
                /// The top coordinate is measured from the parent's top edge.
                GVA_TOP,
                /// The top coordinate is measured from the parent's vertical center.
                GVA_CENTER,
                /// The top coordinate is measured from the parent's bottom edge.
                GVA_BOTTOM
            };

            /**
             * @brief Strongly typed horizontal alignment values.
             */
            enum class HorizontalAlignment
            {
                LEFT,
                CENTER,
                RIGHT,
                COUNT
            };

            /**
             * @brief Strongly typed vertical alignment values.
             */
            enum class VerticalAlignment
            {
                TOP,
                CENTER,
                BOTTOM,
                COUNT
            };

            /**
             * @brief Hash for the left property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_LEFT;

            /**
             * @brief Hash for the top property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_TOP;

            /**
             * @brief Hash for the width property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_WIDTH;

            /**
             * @brief Hash for the height property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_HEIGHT;

            /**
             * @brief Hash for the metrics mode property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_METRICSMODE;

            /**
             * @brief Hash for the horizontal alignment property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_ALIGN_HORIZONTAL;

            /**
             * @brief Hash for the vertical alignment property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_ALIGN_VERTICAL;

            /**
             * @brief Hash for the text property of an overlay element.
             */
            static const hash_type STATE_MESSAGE_TEXT;

            /**
             * @brief Hash for adding a child overlay element to a parent.
             */
            static const hash_type STATE_MESSAGE_ADDCHILD;

            /**
             * @brief Hash for removing a child overlay element from a parent.
             */
            static const hash_type STATE_MESSAGE_REMOVECHILD;

            /**
             * @brief Hash for attaching an object to an overlay element.
             */
            static const hash_type STATE_MESSAGE_ATTACH_OBJECT;

            /**
             * @brief Hash for detaching an object from an overlay element.
             */
            static const hash_type STATE_MESSAGE_DETACH_OBJECT;

            /**
             * @brief Hash for detaching all objects from an overlay element.
             */
            static const hash_type STATE_MESSAGE_DETACH_ALL_OBJECTS;

            /**
             * @brief Virtual destructor.
             */
            ~IOverlayElement() override;

            /**
             * @brief Apply a material to this element.
             * @param material Material used by the renderer when drawing the element.
             */
            virtual void setMaterial( SmartPtr<IMaterial> material ) = 0;

            /**
             * @brief Get the material currently applied to this element.
             * @return Current material, or nullptr when no material is assigned.
             */
            virtual SmartPtr<IMaterial> getMaterial() const = 0;

            /**
             * @brief Set the caption or text displayed by this element.
             * @param text Caption text to store on the element.
             */
            virtual void setCaption( const String &text ) = 0;

            /**
             * @brief Get the caption or text displayed by this element.
             * @return Current caption text, or an empty string if none is set.
             */
            virtual String getCaption() const = 0;

            /**
             * @brief Set whether this element should be visible.
             * @param visible true to show the element; false to hide it.
             */
            virtual void setVisible( bool visible ) = 0;

            /**
             * @brief Query whether this element is visible.
             * @return true if the element is visible; false otherwise.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Get the element position in the current metrics mode.
             * @return Position as a 2D vector.
             */
            virtual Vector2<real_Num> getPosition() const = 0;

            /**
             * @brief Set the element position in the current metrics mode.
             * @param position New 2D position.
             */
            virtual void setPosition( const Vector2<real_Num> &position ) = 0;

            /**
             * @brief Get the element size in the current metrics mode.
             * @return Size as width and height.
             */
            virtual Vector2<real_Num> getSize() const = 0;

            /**
             * @brief Set the element size in the current metrics mode.
             * @param size New width and height.
             */
            virtual void setSize( const Vector2<real_Num> &size ) = 0;

            /**
             * @brief Get the element z-order relative to sibling elements.
             * @return Current z-order value.
             */
            virtual u32 getZOrder() const = 0;

            /**
             * @brief Set the element z-order relative to sibling elements.
             * @param zOrder New z-order value.
             */
            virtual void setZOrder( u32 zOrder ) = 0;

            /**
             * @brief Set the colour tint applied to this element.
             * @param colour Colour tint to apply.
             */
            virtual void setColour( const ColourF &colour ) = 0;

            /**
             * @brief Get the colour tint applied to this element.
             * @return Current colour tint.
             */
            virtual ColourF getColour() const = 0;

            /**
             * @brief Set how position and size values are interpreted.
             * @param metricsMode One of the GuiMetricsMode values.
             */
            virtual void setMetricsMode( u8 metricsMode ) = 0;

            /**
             * @brief Get how position and size values are interpreted.
             * @return Current GuiMetricsMode value.
             */
            virtual u8 getMetricsMode() const = 0;

            /**
             * @brief Set the horizontal origin used for layout.
             * @param gha One of the GuiHorizontalAlignment values.
             */
            virtual void setHorizontalAlignment( u8 gha ) = 0;

            /**
             * @brief Get the horizontal origin used for layout.
             * @return Current GuiHorizontalAlignment value.
             */
            virtual u8 getHorizontalAlignment() const = 0;

            /**
             * @brief Set the vertical origin used for layout.
             * @param gva One of the GuiVerticalAlignment values.
             */
            virtual void setVerticalAlignment( u8 gva ) = 0;

            /**
             * @brief Get the vertical origin used for layout.
             * @return Current GuiVerticalAlignment value.
             */
            virtual u8 getVerticalAlignment() const = 0;

            /**
             * @brief Query whether this element can contain children.
             * @return true if this element is a container; false otherwise.
             */
            virtual bool isContainer() const = 0;

            /**
             * @brief Get the overlay that owns this element.
             * @return Owning overlay, or nullptr when detached.
             */
            virtual SmartPtr<IOverlay> getOverlay() const = 0;

            /**
             * @brief Set the overlay that owns this element.
             * @param overlay Overlay to associate with this element.
             */
            virtual void setOverlay( SmartPtr<IOverlay> overlay ) = 0;

            /**
             * @brief Get this element's parent in the overlay hierarchy.
             * @return Parent element, or nullptr when this is a root element.
             */
            virtual SmartPtr<IOverlayElement> getParent() const = 0;

            /**
             * @brief Set this element's parent in the overlay hierarchy.
             * @param element Parent element to associate with this element.
             */
            virtual void setParent( SmartPtr<IOverlayElement> element ) = 0;

            /**
             * @brief Add a child element to this element.
             * @param element Child element to add.
             */
            virtual void addChild( SmartPtr<IOverlayElement> element ) = 0;

            /**
             * @brief Remove a child element from this element.
             * @param element Child element to remove.
             */
            virtual void removeChild( SmartPtr<IOverlayElement> element ) = 0;

            /**
             * @brief Get this element's children.
             * @return Snapshot of child elements.
             */
            virtual Array<SmartPtr<IOverlayElement>> getChildren() const = 0;

            /**
             * @brief Retrieve the renderer-specific object behind this element.
             * @param ppObject Output pointer that receives the backend object.
             *
             * The returned pointer type depends on the active graphics backend.
             * Ownership is not transferred to the caller.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IOverlayElement_h__
