#ifndef GyroUnit1H
#define GyroUnit1H

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include "WPVehiclePhysics/HeliVars.hpp"

namespace workphone::vehicle
{
    /**
         * @brief Gyroscopic control unit for yaw (tail) control.
         *
         * CGyroUnit encapsulates state and tuning parameters used by the
         * yaw-rate feedback and stop-control (SC) algorithms for helicopter
         * tail stabilization. Values are expressed in SI units where applicable:
         * - yaw rate in radians/second
         * - acceleration in radians/second^2
         *
         * This class is a plain data holder (POD-like) used by higher-level
         * control code; it intentionally deletes the copy constructor to avoid
         * accidental copying of control state.
         */
    class WPVehiclePhysics_API CGyroUnit
    {
    public:
        /**
             * @brief Default constructor — initializes members to safe defaults.
             *
             * Implementation should set sensible defaults for all tuning and
             * state variables so the unit is in a known, disabled or neutral
             * state after construction.
             */
        CGyroUnit();

        /** @brief Deleted copy constructor to prevent copying runtime state. */
        CGyroUnit(const CGyroUnit &other) = delete;

        /* Input / output */
        /** @brief Raw control input from pilot or autopilot (-1 .. 1). */
        physics_Num m_input; // in range -1 to 1

        /** @brief Computed servo output (-1 .. 1). */
        physics_Num m_output; // in range -1 to 1

        /* Stick handling */
        /**
             * @brief Fraction of stick deflection treated as deadband (0 .. 1).
             *
             * Inputs with absolute magnitude below this fraction are treated as zero
             * to avoid reacting to small stick noise.
             */
        physics_Num m_stickDeadBand;

        /**
             * @brief Stick sensitivity: yaw rate (rad/s) commanded at unit input = 1.
             *
             * Defines the mapping from normalized input to desired yaw rate.
             */
        physics_Num m_stickSensitivity; // yaw rate in Radians/s at unit input

        /**
             * @brief Exponential (expo) fraction applied to stick response (0 .. 1).
             *
             * Fraction of the sensitivity that scales with the square of the input
             * to provide softer center and stronger end-stick sensitivity.
             */
        physics_Num m_stickExpo;

        /** @brief Direct in-to-out coupling gain (unitless). */
        physics_Num m_directGain;

        /* Yaw loop state and gains */
        /** @brief Current yaw demand (target) in normalized units or as set by flight logic. */
        physics_Num m_yawDemand;

        /** @brief Measured yaw rate in radians/second. */
        physics_Num m_yawRate;

        /** @brief Current yaw rate error (demand - measured) in rad/s. */
        physics_Num m_yawError;

        /**
             * @brief Limit applied to yaw error for computing corrective terms.
             *
             * Used to saturate the influence of large transient errors.
             */
        physics_Num m_yawErrorLimit;

        /** @brief Proportional gain applied to yaw error (servo deflection per rad/s). */
        physics_Num m_yawErrorGain;

        /* Heading lock (HL) / hold logic */
        /** @brief Heading-lock error (angular) used for HL term calculations (radians). */
        physics_Num m_hlError;

        /** @brief Angular range used to normalise HL term (radians). */
        physics_Num m_hlRange;

        /**
             * @brief Angular range that produces a full HL term of 1 (radians).
             *
             * This value is limited to a maximum of m_hlRange by higher-level logic.
             */
        physics_Num m_hlLimit;

        /** @brief Gain applied to the heading-lock term (servo deflection per radian). */
        physics_Num m_hlGain;

        /** @brief First throw limit for servo travel (unitless fraction of full travel). */
        physics_Num m_throwLimit1;

        /** @brief Second throw limit for servo travel (unitless fraction of full travel). */
        physics_Num m_throwLimit2;

        /** @brief Master gain of the gyro supplied from the Mode/Gain channel (unitless). */
        physics_Num m_gain;

        /** @brief Trim offset applied to the servo output (unitless). */
        physics_Num m_servoOffset;

        /** @brief Time (seconds) that HL has been off during a stop event. */
        physics_Num m_hlOffTimer;

        /**
             * @brief Maximum time (seconds) HL is forced off during a stop.
             *
             * Prevents HL from being re-enabled too quickly after a stop.
             */
        physics_Num m_hlKillTime;

        /** @brief Decay factor for heading lock accumulation (unitless). */
        physics_Num m_hlDecay;

        /* Acceleration feed-forward */
        /** @brief Yaw acceleration in radians/second^2 (computed). */
        physics_Num m_acceleration;

        /** @brief Last sampled yaw rate (rad/s) used for acceleration calculation. */
        physics_Num m_lastYawRate;

        /** @brief Delta time (seconds) used by the acceleration calculator. */
        physics_Num m_deltaT;

        /** @brief Time constant (seconds) over which acceleration is computed. */
        physics_Num m_accTC;

        /** @brief Acceleration gain — servo fraction per rad/s^2 acceleration. */
        physics_Num m_accGain;

        /** @brief Acceleration term contributing to servo deflection (unitless). */
        physics_Num m_accTerm;

        /**
             * @brief Upper limit of the acceleration term as a fraction of full servo travel (0..1).
             *
             * Prevents excessive contribution from transient accelerations.
             */
        physics_Num m_accTermLimit;

        /* Stop control (SC) and stop gains */
        /** @brief Left-stop servo gain (unitless fraction of travel). */
        physics_Num m_leftStopGain;

        /** @brief Right-stop servo gain (unitless fraction of travel). */
        physics_Num m_rightStopGain;

        /**
             * @brief Currently active stop gain (equals 1 for normal flight, or one of the stop gains).
             *
             * Value is:
             *  - 1.0 for normal flight
             *  - m_leftStopGain during a left stop
             *  - m_rightStopGain during a right stop
             */
        physics_Num m_currentStopGain;

        /** @brief Yaw demand threshold above which Stop Control (SC) will arm. */
        physics_Num m_scArmDemand;

        /** @brief Minimum yaw rate (rad/s) required before arming SC. */
        physics_Num m_scArmRate;

        /** @brief Yaw demand threshold below which SC can be triggered. */
        physics_Num m_scTrigDemand;

        /** @brief Yaw rate error threshold (rad/s) above which stop control is triggered. */
        physics_Num m_scTrigRateError;

        /**
             * @brief Yaw demand below which SC can disarm if the yaw error is also below a threshold.
             *
             * Used to allow SC to leave armed state when the pilot reduces demand.
             */
        physics_Num m_scDisarmDemand;

        /** @brief Yaw rate error below which stop control can be disarmed (rad/s). */
        physics_Num m_scDisarmRateError;

        /**
             * @brief Yaw rate (rad/s) at which an active stop is considered complete.
             *
             * When the current yaw rate falls below this value the stop is deemed done and HL can be
             * re-enabled.
             */
        physics_Num m_scStopDoneYaw;

        /** @brief Demand threshold at which an active stop control is aborted (unitless). */
        physics_Num m_scStopAbortDemand;

        /**
             * @brief Stop-Control (SC) mode.
             *
             * Valid values:
             *  - 0 = disarmed
             *  - 1 = armed with positive yaw
             *  - 3 = armed with negative yaw
             *  - 4 = triggered (active stop)
             */
        s32 m_scMode;

        /** @brief True when Mode/Gain channel is configured for Heading-Lock mode. */
        bool m_hlMode;

        /** @brief True when Heading-Lock is currently inhibited by stop control. */
        bool m_hlOn;

        /** @brief If true, invert the sense of the gyro input / correction. */
        bool m_senseReverse;
    }; // class CGyroUnit
}

#endif //  GyroUnit1H
