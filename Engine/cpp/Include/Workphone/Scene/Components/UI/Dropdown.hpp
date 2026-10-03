#ifndef Dropdown_h__
#define Dropdown_h__

#include <Workphone/Scene/Components/UI/UIComponent.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class Dropdown
         * @brief UI component that implements a dropdown (select) control.
         *
         * The Dropdown component composes a trigger button and an options panel (content).
         * The trigger button toggles the visibility of the options panel. Options are
         * represented by the nested Option class and are typically created from an
         * option prefab. The component exposes properties for layout, styling and
         * behaviour such as panel colour, content height, z-ordering and initial
         * option capacity.
         */
        class WPCore_API Dropdown : public UIComponent
        {
        public:
            static const String dropdownButtonStr;
            static const String dropdownPanelStr;
            static const String isOpenStr;
            static const String optionStr;
            static const String imageStr;
            static const String textStr;

            static const String panelColourStr;
            static const String contentOffsetStr;
            static const String contentHeightStr;
            static const String panelSizeStr;
            static const String contentZOrderStr;
            static const String optionButtonColourStr;
            static const String contentNameStr;
            static const String panelNameStr;
            static const String scrollbarNameStr;
            static const String scrollbarBackgroundNameStr;
            static const String scrollbarZOrderOffsetStr;
            static const String initialOptionCapacityStr;

            /**
             * @brief Represents a single dropdown option.
             *
             * Each Option contains optional image data and label text. Option instances
             * are lightweight shared objects used by the Dropdown to populate the
             * options panel.
             */
            class WPCore_API Option : public ISharedObject
            {
            public:
                Option();
                Option( const String &text );
                ~Option() override;

                /// Optional icon/thumbnail for the option.
                SmartPtr<render::ITexture> imageTexture;

                /// Display text for the option.
                FixedString<256> text;

                WP_CLASS_REGISTER_DECL;
            };

            /**
             * @brief Default constructor.
             */
            Dropdown();

            /**
             * @brief Virtual destructor.
             */
            ~Dropdown() override;

            /** @copydoc UIComponent::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc UIComponent::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc UIComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc UIComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc UIComponent::isValid */
            bool isValid() const override;

            /**
             * @brief Get the trigger button used to open/close the dropdown.
             * @return Smart pointer to the Button used as trigger.
             */
            SmartPtr<Button> getButton() const;

            /**
             * @brief Set the trigger button for this dropdown.
             * @param button Button instance to use as the trigger control.
             */
            void setButton( SmartPtr<Button> button );

            /**
             * @brief Get the panel actor that displays the currently selected option.
             * @return Smart pointer to the panel actor.
             */
            SmartPtr<IGameActor> getPanel() const;

            /**
             * @brief Set the panel actor that displays the selected option.
             * @param panel Panel actor to use for displaying selection.
             */
            void setPanel( SmartPtr<IGameActor> panel );

            /**
             * @brief Get the content actor that holds the options list.
             * @return Smart pointer to the content actor.
             */
            SmartPtr<IGameActor> getContent() const;

            /**
             * @brief Set the content actor used to host the options UI.
             * @param content Content actor to host generated option items.
             */
            void setContent( SmartPtr<IGameActor> content );

            /**
             * @brief Get the prefab actor used to instantiate option items.
             * @return Smart pointer to the option prefab actor.
             */
            SmartPtr<IGameActor> getOptionPrefab() const;

            /**
             * @brief Set the prefab used to create option UI elements.
             * @param optionPrefab Prefab actor to clone for each option.
             */
            void setOptionPrefab( SmartPtr<IGameActor> optionPrefab );

            /**
             * @brief Query whether the dropdown is currently open.
             * @return True if the options panel is visible/open.
             */
            bool isOpen() const;

            /**
             * @brief Open or close the dropdown.
             * @param open True to open (show) the options panel; false to close it.
             */
            void setOpen( bool open );

            /**
             * @brief Get the current array of options.
             * @return Array of shared pointers to Option objects.
             */
            Array<SmartPtr<Option>> getOptions() const;

            /**
             * @brief Replace the dropdown's options with a new set.
             * @param options Array of Option shared pointers to use.
             */
            void setOptions( const Array<SmartPtr<Option>> &options );

            /**
             * @brief Add a single option to the dropdown.
             * @param option Option object to append.
             */
            void addOption( SmartPtr<Option> option );

            /**
             * @brief Remove an option from the dropdown.
             * @param option Option object to remove if present.
             */
            void removeOption( SmartPtr<Option> option );

            /**
             * @brief Remove all options from the dropdown.
             */
            void removeOptions();

            /** @copydoc Renderer::handleComponentEvent */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /** @copydoc UIComponent::handleEvent */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Get the background colour applied to the options panel.
             * @return Background ColourF used for the panel.
             */
            ColourF getPanelColour() const;

            /**
             * @brief Set the background colour applied to the options panel.
             * @param bgPanelColour Colour to use for the panel background.
             */
            void setPanelColour( const ColourF &bgPanelColour );

            /**
             * @brief Get the colour applied to generated option buttons in their normal state.
             * @return ColourF used for option buttons.
             */
            ColourF getOptionButtonColour() const;

            /**
             * @brief Set the normal-state colour applied to generated option buttons.
             * @param colour Colour to apply to option buttons in normal state.
             */
            void setOptionButtonColour( const ColourF &colour );

            /**
             * @brief Get the vertical gap in pixels between the trigger button and content area.
             * @return Vertical gap in pixels.
             */
            f32 getContentOffset() const;

            /**
             * @brief Set the vertical gap in pixels between the trigger button and content area.
             * @param offset Gap in pixels.
             */
            void setContentOffset( f32 offset );

            /**
             * @brief Get the visible height (pixels) of the dropdown content area.
             * @return Visible height in pixels.
             */
            f32 getContentHeight() const;

            /**
             * @brief Set the visible height (pixels) of the dropdown content area.
             * @param height Height in pixels.
             */
            void setContentHeight( f32 height );

            /**
             * @brief Get the inner options panel size in pixels.
             * @return Panel size as a Vector2 of pixel dimensions.
             */
            Vector2<real_Num> getPanelSize() const;

            /**
             * @brief Set the inner options panel size in pixels.
             * @param size Panel size as a Vector2 of pixel dimensions.
             */
            void setPanelSize( const Vector2<real_Num> &size );

            /**
             * @brief Get the base Z-order value for the content layer and its children.
             * @return Integer Z-order base.
             */
            s32 getContentZOrder() const;

            /**
             * @brief Set the base Z-order value for the content layer and its children.
             * @param zOrder Base Z-order to apply.
             */
            void setContentZOrder( s32 zOrder );

            /** Get/set helper names used to locate child actors. */
            String getContentName() const;
            void setContentName( const String &name );

            String getPanelName() const;
            void setPanelName( const String &name );

            String getScrollbarName() const;
            void setScrollbarName( const String &name );

            String getScrollbarBackgroundName() const;
            void setScrollbarBackgroundName( const String &name );

            /** Get/set scrollbar z-order offset relative to content z-order. */
            s32 getScrollbarZOrderOffset() const;
            void setScrollbarZOrderOffset( s32 offset );

            /** Get/set initial reserve capacity for the options array. */
            u32 getInitialOptionCapacity() const;
            void setInitialOptionCapacity( u32 capacity );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Construct the options panel UI elements based on current properties and options.
             */
            void buildOptionsPanel();

            /**
             * @brief Update visibility and transform of the options panel depending on m_isOpen.
             */
            void updateOptionsPanelVisibility();

            /**
             * @brief Create or find child UI actors used by this component.
             */
            void createUI() override;

            /**
             * @brief Update visual state (styles, enabled/disabled state) of the element.
             */
            void updateElementState() override;

            /**
             * @brief Return raw pointer to the content actor, may be nullptr.
             * @return Raw IGameActor pointer for internal usage.
             */
            IGameActor *getContentPtr() const;

            /// Scroll view used to display options (non-owning raw pointer).
            ScrollView *m_scrollView = nullptr;

            /// Trigger button (non-owning raw pointer cached for performance).
            Button *m_button = nullptr;

            /// Actor used to display the currently selected option.
            SmartPtr<IGameActor> m_panel;

            /// Actor that hosts the generated option items.
            SmartPtr<IGameActor> m_content;

            /// Prefab actor cloned to create each option item.
            SmartPtr<IGameActor> m_optionPrefab;

            /// Background colour applied to the options panel.
            ColourF m_bgPanelColour = ColourF( 0.5f, 0.5f, 0.5f, 1.0f );

            /// Colour applied to generated option buttons in their normal state.
            ColourF m_optionButtonColour = ColourF( 0.3f, 0.3f, 0.3f, 0.0f );

            /// Vertical gap (pixels) between the trigger button and the content area.
            f32 m_contentOffset = 20.0f;

            /// Visible height (pixels) of the dropdown content area.
            f32 m_contentHeight = 300.0f;

            /// Size (pixels) of the inner options panel.
            Vector2<real_Num> m_panelSize = Vector2<real_Num>( 300.0f, 300.0f );

            /// Base Z-order value for the content layer and its children.
            s32 m_contentZOrder = 1000;

            /// Helper names used to find child actors inside prefabs/actors.
            FixedString<128> m_contentName = "Content";
            FixedString<128> m_panelName = "Panel";
            FixedString<128> m_scrollbarName = "Scrollbar";
            FixedString<128> m_scrollbarBackgroundName = "Background";

            /// Z-order offset applied to the scrollbar relative to the content z-order.
            s32 m_scrollbarZOrderOffset = 1000;

            /// Reserve capacity used when creating the options array to reduce allocations.
            u32 m_initialOptionCapacity = 12u;

            /// True when the dropdown's options panel is currently open/visible.
            bool m_isOpen = false;

            /// Array of option objects representing the dropdown choices.
            Array<SmartPtr<Option>> m_options;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Dropdown_h__
