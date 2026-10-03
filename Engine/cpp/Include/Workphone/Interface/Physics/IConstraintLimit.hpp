#ifndef IConstraintLimit_h__
#define IConstraintLimit_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for constraint limits used in physics simulation.
         */
        class WPCore_API IConstraintLimit : public ISharedObject
        {
        public:
            /**
             * @brief Destructor for IConstraintLimit.
             */
            ~IConstraintLimit() override;

            /**
             * @brief Get the coefficient of restitution.
             * @return The coefficient of restitution.
             */
            virtual real_Num getRestitution() const = 0;

            /**
             * @brief Set the coefficient of restitution.
             * @param restitution The new coefficient of restitution.
             */
            virtual void setRestitution( real_Num restitution ) = 0;

            /**
             * @brief Get the bounce threshold.
             * @return The bounce threshold.
             */
            virtual real_Num getBounceThreshold() const = 0;

            /**
             * @brief Set the bounce threshold.
             * @param bounceThreshold The new bounce threshold.
             */
            virtual void setBounceThreshold( real_Num bounceThreshold ) = 0;

            /**
             * @brief Get the stiffness of the limit.
             * @return The stiffness of the limit.
             */
            virtual real_Num getStiffness() const = 0;

            /**
             * @brief Set the stiffness of the limit.
             * @param stiffness The new stiffness of the limit.
             */
            virtual void setStiffness( real_Num stiffness ) = 0;

            /**
             * @brief Get the damping of the limit.
             * @return The damping of the limit.
             */
            virtual real_Num getDamping() const = 0;

            /**
             * @brief Set the damping of the limit.
             * @param damping The new damping of the limit.
             */
            virtual void setDamping( real_Num damping ) = 0;

            /**
             * @brief Get the contact distance of the limit.
             * @return The contact distance of the limit.
             */
            virtual real_Num getContactDistance() const = 0;

            /**
             * @brief Set the contact distance of the limit.
             * @param contactDistance The new contact distance of the limit.
             */
            virtual void setContactDistance( real_Num contactDistance ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace physics
}  // namespace workphone

#endif  // IConstraintLimit_h__
