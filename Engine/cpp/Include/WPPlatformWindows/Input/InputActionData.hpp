#ifndef InputActionData_h__
#define InputActionData_h__

#include <Workphone/Interface/Input/IInputAction.hpp>

/**
 * @file InputActionData.hpp
 * @brief Simple container for input action identifiers.
 */

namespace workphone
{

    /**
     * @brief Implementation of IInputAction that stores primary/secondary
     *        actions and a mapped action id.
     *
     * This class is used to represent a mapping from one or two input
     * identifiers (primary, secondary) to a single action identifier used by
     * the game or application.
     */
    class InputActionData : public IInputAction
    {
    public:
        /**
         * @brief Default-construct an empty InputActionData.
         */
        InputActionData();

        /**
         * @brief Construct with initial primary, secondary and action ids.
         *
         * @param first Primary action id
         * @param second Secondary action id
         * @param actionId Mapped action id
         */
        InputActionData( u32 first, u32 second, u32 actionId );

        /**
         * @brief Get the primary action identifier.
         *
         * @return hash_type Primary action id
         */
        hash_type getPrimaryAction() const override;

        /**
         * @brief Set the primary action identifier.
         *
         * @param primary Primary action id
         */
        void setPrimaryAction( hash_type primary ) override;

        /**
         * @brief Get the secondary action identifier.
         *
         * @return hash_type Secondary action id
         */
        hash_type getSecondaryAction() const override;

        /**
         * @brief Set the secondary action identifier.
         *
         * @param secondary Secondary action id
         */
        void setSecondaryAction( hash_type secondary ) override;

        /**
         * @brief Get the mapped action id.
         *
         * @return hash_type Action id
         */
        hash_type getActionId() const override;

        /**
         * @brief Set the mapped action id.
         *
         * @param actionId Action id to set
         */
        void setActionId( hash_type actionId ) override;

        /**
         * @brief Get the stored primary action name (keyboard key text).
         * @return Primary action name.
         */
        String getPrimaryName() const;

        /**
         * @brief Set the stored primary action name.
         * @param name Name to store.
         */
        void setPrimaryName( const String &name );

        /**
         * @brief Get the stored secondary action name (keyboard key text).
         * @return Secondary action name.
         */
        String getSecondaryName() const;

        /**
         * @brief Set the stored secondary action name.
         * @param name Name to store.
         */
        void setSecondaryName( const String &name );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Primary input identifier (hash).
        hash_type m_primary = 0;
        /// Secondary input identifier (hash).
        hash_type m_secondary = 0;
        /// Mapped action identifier (hash).
        hash_type m_actionId = 0;
        /// Optional human-readable primary name (used for keyboard keys).
        String m_primaryName;
        /// Optional human-readable secondary name (used for keyboard keys).
        String m_secondaryName;
    };
}  // namespace workphone

#endif  // InputActionData_h__
