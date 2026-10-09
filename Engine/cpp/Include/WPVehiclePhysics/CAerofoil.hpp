#ifndef __CAerofoil_h__
#define __CAerofoil_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IAerofoil.hpp>

namespace workphone::vehicle
{
    /**
         * \brief Concrete implementation of an aerofoil (airfoil) model.
         *
         * CAerofoil stores lookup curves for lift (CL), drag (CD) and moment (CM)
         * coefficients and a reference to the parent aircraft.  The class provides
         * loading functionality and accessors used by the flight model.
         */
    class WPVehiclePhysics_API CAerofoil : public IAerofoil
    {
    public:
        /**
             * \brief Construct an empty aerofoil object.
             *
             * Creates an aerofoil with no lookup curves assigned and with
             * reverse-value behavior disabled by default.
             */
        CAerofoil();

        /**
             * \brief Virtual destructor.
             *
             * Ensures proper cleanup of derived resources.
             */
        ~CAerofoil() override;

        /**
             * \brief Load aerofoil data from a file.
             *
             * The file is expected to contain coefficient curves (CL, CD, CM)
             * in the project's aerodynamic data format. Implementations should
             * set the internal lookup curves on successful parsing.
             *
             * \param fileName Path to the aerofoil data file.
             */
        void load(const std::string &fileName);

        /**
             * \brief Get raw pointer to the lift coefficient lookup curve.
             *
             * \return Pointer to the CL lookup curve, or nullptr if not set.
             */
        LookupCurve<real_Num> *getCLPtr() override;

        /**
             * \brief Get smart pointer to the lift coefficient lookup curve.
             *
             * \return SmartPtr owning or referencing the CL lookup curve.
             */
        SmartPtr<LookupCurve<real_Num>> getCL() const override;

        /**
             * \brief Set the lift coefficient lookup curve.
             *
             * \param cl SmartPtr to the CL lookup curve to use.
             */
        void setCL(SmartPtr<LookupCurve<real_Num>> cl) override;

        /**
             * \brief Get raw pointer to the drag coefficient lookup curve.
             *
             * \return Pointer to the CD lookup curve, or nullptr if not set.
             */
        LookupCurve<real_Num> *getCDPtr() override;

        /**
             * \brief Get smart pointer to the drag coefficient lookup curve.
             *
             * \return SmartPtr owning or referencing the CD lookup curve.
             */
        SmartPtr<LookupCurve<real_Num>> getCD() const override;

        /**
             * \brief Set the drag coefficient lookup curve.
             *
             * \param cd SmartPtr to the CD lookup curve to use.
             */
        void setCD(SmartPtr<LookupCurve<real_Num>> cd) override;

        /**
             * \brief Get raw pointer to the pitching moment coefficient lookup curve.
             *
             * \return Pointer to the CM lookup curve, or nullptr if not set.
             */
        LookupCurve<real_Num> *getCMPtr() override;

        /**
             * \brief Get smart pointer to the pitching moment lookup curve.
             *
             * \return SmartPtr owning or referencing the CM lookup curve.
             */
        SmartPtr<LookupCurve<real_Num>> getCM() const override;

        /**
             * \brief Set the pitching moment lookup curve.
             *
             * \param cm SmartPtr to the CM lookup curve to use.
             */
        void setCM(SmartPtr<LookupCurve<real_Num>> cm) override;

        /**
             * \brief Get raw pointer to the associated aircraft interface.
             *
             * \return Pointer to the IAircraft instance, or nullptr if none set.
             */
        IAircraft *getAircraftPtr() const override;

        /**
             * \brief Get smart pointer to the associated aircraft interface.
             *
             * \return SmartPtr owning or referencing the IAircraft instance.
             */
        SmartPtr<IAircraft> getAircraft() const override;

        /**
             * \brief Associate this aerofoil with an aircraft instance.
             *
             * \param aircraft SmartPtr to the aircraft that owns or uses this aerofoil.
             */
        void setAircraft(SmartPtr<IAircraft> aircraft) override;

        /**
             * \brief Query whether coefficient values should be interpreted reversed.
             *
             * Some data sets may provide values in an orientation opposite to the
             * simulation convention; when true the aerofoil will invert or remap
             * values as needed.
             *
             * \return True if values are reversed, false otherwise.
             */
        bool getReverseValues() const override;

        /**
             * \brief Set whether coefficient values should be interpreted reversed.
             *
             * \param reverse True to enable reverse-value behavior, false to disable.
             */
        void setReverseValues(bool reverse) override;

    private:
        // Helper to convert raw curve data into a fixed-size points array.
        // std::array<std::pair<real_Num, real_Num>, 360> getPointsArray(
        //     Array<data::aircraft_curve_value> values );

        /// Lift coefficient lookup curve (CL).
        SmartPtr<LookupCurve<real_Num>> m_cl;

        /// Drag coefficient lookup curve (CD).
        SmartPtr<LookupCurve<real_Num>> m_cd;

        /// Pitching moment coefficient lookup curve (CM).
        SmartPtr<LookupCurve<real_Num>> m_cm;

        /// Owning or referencing aircraft instance for context-dependent queries.
        SmartPtr<IAircraft> m_aircraft;

        /// When true, input coefficient values should be interpreted in reverse.
        bool m_reverseValues = false;
    };
}

#endif // Aerofoil_h__
