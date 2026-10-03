#ifndef IAerofoil_h__
#define IAerofoil_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/LookupCurve.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Interface for an aerofoil representation.
         *
         * @details
         * The `IAerofoil` interface exposes coefficient lookup curves and an associated
         * aircraft reference for aerofoil elements used by aerodynamic subsystems.
         * Coefficients are stored in `LookupCurve<real_Num>` instances and accessed
         * via raw pointer or `SmartPtr` accessors. Implementations provide storage,
         * lifetime and any interpolation/lookup semantics for the coefficient curves.
         *
         * @note All getters that return raw pointers (`get*Ptr`) return a non-owning
         *       pointer. Use the `SmartPtr` returning getters when ownership or
         *       reference-counted access is required.
         *
         * @see LookupCurve
         */
        class WPCore_API IAerofoil : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * @details Ensures derived classes are properly destroyed through the
             *          interface pointer.
             */
            ~IAerofoil() override;

            /**
             * @brief Get a non-owning pointer to the lift coefficient curve (CL).
             *
             * @return Pointer to the `LookupCurve<real_Num>` used for lift coefficient
             *         lookups. May be `nullptr` if not set.
             */
            virtual LookupCurve<real_Num> *getCLPtr() = 0;

            /**
             * @brief Get a reference-counted handle to the lift coefficient curve (CL).
             *
             * @return `SmartPtr` holding the curve. The `SmartPtr` may be empty if no
             *         curve is assigned.
             */
            virtual SmartPtr<LookupCurve<real_Num>> getCL() const = 0;

            /**
             * @brief Set the lift coefficient curve (CL).
             *
             * @param cl `SmartPtr` to the `LookupCurve<real_Num>` to use for lift lookups.
             *           Passing an empty `SmartPtr` clears the curve.
             */
            virtual void setCL( SmartPtr<LookupCurve<real_Num>> cl ) = 0;

            /**
             * @brief Get a non-owning pointer to the drag coefficient curve (CD).
             *
             * @return Pointer to the `LookupCurve<real_Num>` used for drag coefficient
             *         lookups. May be `nullptr` if not set.
             */
            virtual LookupCurve<real_Num> *getCDPtr() = 0;

            /**
             * @brief Get a reference-counted handle to the drag coefficient curve (CD).
             *
             * @return `SmartPtr` holding the drag curve. The `SmartPtr` may be empty if
             *         no curve is assigned.
             */
            virtual SmartPtr<LookupCurve<real_Num>> getCD() const = 0;

            /**
             * @brief Set the drag coefficient curve (CD).
             *
             * @param cd `SmartPtr` to the `LookupCurve<real_Num>` to use for drag lookups.
             *           Passing an empty `SmartPtr` clears the curve.
             */
            virtual void setCD( SmartPtr<LookupCurve<real_Num>> cd ) = 0;

            /**
             * @brief Get a non-owning pointer to the moment coefficient curve (CM).
             *
             * @return Pointer to the `LookupCurve<real_Num>` used for moment coefficient
             *         lookups. May be `nullptr` if not set.
             */
            virtual LookupCurve<real_Num> *getCMPtr() = 0;

            /**
             * @brief Get a reference-counted handle to the moment coefficient curve (CM).
             *
             * @return `SmartPtr` holding the moment curve. The `SmartPtr` may be empty if
             *         no curve is assigned.
             */
            virtual SmartPtr<LookupCurve<real_Num>> getCM() const = 0;

            /**
             * @brief Set the moment coefficient curve (CM).
             *
             * @param cm `SmartPtr` to the `LookupCurve<real_Num>` to use for moment lookups.
             *           Passing an empty `SmartPtr` clears the curve.
             */
            virtual void setCM( SmartPtr<LookupCurve<real_Num>> cm ) = 0;

            /**
             * @brief Get a raw (non-owning) pointer to the associated aircraft.
             *
             * @return Pointer to the `IAircraft` instance this aerofoil belongs to.
             *         May be `nullptr` if no aircraft has been associated.
             */
            virtual IAircraft *getAircraftPtr() const = 0;

            /**
             * @brief Get a reference-counted handle to the associated aircraft.
             *
             * @return `SmartPtr` holding the `IAircraft`. May be empty if no aircraft
             *         has been associated.
             */
            virtual SmartPtr<IAircraft> getAircraft() const = 0;

            /**
             * @brief Associate this aerofoil with an `IAircraft`.
             *
             * @param aircraft `SmartPtr` to the aircraft. Passing an empty `SmartPtr`
             *                 disassociates any previously set aircraft.
             */
            virtual void setAircraft( SmartPtr<IAircraft> aircraft ) = 0;

            /**
             * @brief Query whether coefficient values are reversed.
             *
             * @details Some aerofoil instances may store coefficients in a reversed
             *          convention (for example due to mounting orientation). This flag
             *          allows callers to query that behaviour.
             *
             * @return `true` if values are considered reversed; otherwise `false`.
             */
            virtual bool getReverseValues() const = 0;

            /**
             * @brief Set whether coefficient values should be interpreted as reversed.
             *
             * @param reverse `true` to treat coefficient values as reversed; `false`
             *                for normal interpretation.
             */
            virtual void setReverseValues( bool reverse ) = 0;

            /** Macro used by the runtime/type system to register the class. */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IAerofoil_h__
