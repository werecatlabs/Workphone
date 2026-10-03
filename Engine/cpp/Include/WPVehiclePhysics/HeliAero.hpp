#ifndef FBHeliAero25H
#define FBHeliAero25H

/**
 * @file HeliAero.hpp
 * @brief Aerodynamics simulation for flybar-equipped single rotor helicopters
 *
 * This module handles comprehensive aerodynamic modeling for RC helicopters including:
 * - Main and tail rotor dynamics
 * - Flybar and flybarless systems
 * - Ground effect calculations
 * - Induced flow modeling
 * - Engine/motor and transmission systems
 * - Control surface linkages and servos
 * - Electronic speed controllers (ESC) and governors
 * - Gyroscopic stabilization
 *
 * @version 23
 * @date 2011-03-11
 *
 * @section version_history Version History
 * - v23: Added facilities to adjust heli parameters as needed by the core sim
 *        - Inhibit 'wobble' at low headspeed (DONE)
 *        - Control works via TX or direct collective/cyclic angles (DONE)
 *        - Gyro separated to distinct DLL
 *        - Support for external engine unit
 *        - Distinct shaft RPM and aerodynamic RPM
 * - v18: Introduced adjustment of visual rotor servo constants from Python
 * - v17: Introduced different flybar model based on low-aspect ratio dynamics
 *
 * @section flow_convention Flow Sign Convention
 * Hub flow points in direction of the flow. If the helicopter is flying in +Z direction
 * and pointing in +Z direction, then hub flow is in -Z direction. Blade element velocities
 * are in the direction of motion (along chord line). For sector 2, element velocity is in
 * +Z direction. Local flow = hub flow - element velocity.
 *
 * @note RotationalEnergy < 0 condition in EnergyBudget sets flywheel speed to 1e-3
 */

/*Modifications
 3/11/11        The RotateFrame args changed to (PitchAng,YawAng,RollAng) to be consistant with the X,Y,Z
 axis order Vectorised the Induced flow of the rotor and introduced the WInduced vector to hold value in
 the world frame
*/

/*Jobs
Note: current task is >>>

Airflow at blade elements (DONE)

Calculate induced flow at all arcs based on lift in the arc (DONE)

Calc angles of attack for all elements (DONE)

Lookup for CL and CD  (DONE)

Sum forces to give total thust and cyclic moments  (DONE)

solve head-linkage to get cyclic angles for flybar and main rotor (sort out Records to hold Frame of ref
for the swash and body) (DONE)

Get callback code into this dll with Tx read routines etc  (DONE)

convert Tx data to collective and cyclic angles (DONE)

calculate precession rates  and calculate new teeter forces (DONE)

Sort out forces to govern the visual rotor  (DONE)

Arrange for yaw channel to go to the gyro routine (DONE)

Add tail rotor pitch control from Tx/gyro system  (DONE)

Generally bring TR into play by:-
  Forcing its Frame as required by Body Frame  (DONE)
  Calc its flow including body rotation component    (DONE)

do a basic gyro, say conventional + 401 style part-time HL to keep it simple (in another dll??) (DONE)

Model the engine/transmission/governor system by reflecting rotor inertias through the gearing to give
effective flywheel at engine (DONE)

make the inflow for the flybar use the main-rotor inflow data for the corresponding radius range
(slightly tricky!) (Crude approach - use mid-span main rotor inflow values for the whole of the flybar!)
(DONE)


Allow for teeter force 'kickback' into rotor (do this in CalcTeeterForces ???)   (DONE)
Combine the DoPrecession and calcTeeterForces into one procedure that does the Main & FB precession and
calcs the teeter and does kickback as it is easier to deal with the differences in the way the rotors are
treated if we combine them under a single routine. (DONE)


put body drag into dll from Python (long overdue!!)


Improve induced flow calculations:-
  Fully vectorise the induced flow field   (DONE)
  Apply decay timeconstant.                 (DONE)
  Perhaps make the inflow array shared between the main and FB
  Decide if correction for free-stream flow velocity by lookup will use gross Vh value or local Vh for
each arc element (DONE)


*Make Induced flow calculation more efficient by using only local FoR and use counter rotation of the
induced vectors (oposite to heli rotation in world frame) (DONE)

*Check RotateFrame for accuracy! (DONE)

Add pitch and roll rates to the local flow of the main and flybar rotors

Ground effect (preferred method - ground 'rebound' method) possibly added in via the 'Wind' component as
an updraft?? (DONE)

Gearing losses (DONE)

Body - main-rotor interference

Modify the MainArcFlows code so that the GndEffect does not add into the tail-rotor!!!

Look for Cl/Cd curves for more representitive reynolds numbers (probably lower Cl max, higher Cd min and
earlier stall)

routine to calculate the gyroscopic and load related torque effects of the tail-rotor on the body

Generate Smoke flow field using main rotor induced flow

Generate sound data record using Engine RPM, Rotor RPM, Engine Load, Max and min blade pressures, Inflow
via boom etc

think about flybar losses from drag of the bar

**GET TO THE BOTTOM OF THE TRUC ERRORS AND SORT OUT THE LOOKUPS IN THE LIGHT OF IT**

**HAVE A GENERAL TIDY UP!!!!**

Tests etc:-

** Check that LiftVec and the the lift sign are OK in flow reversal situations! (see CalcMainArcFlows)

*/

/*Notes:
We need a routine to take the aircraft velocity (with wind added later) and the aircraft orientation
together with the angles between the model and the airframe to calculate the flow in the rotor frame of
reference In the first instance we can perhaps ignore the teeter angles and use the airframe relative
flow or use the orientation if the 'rotor body' to calculate the relative flow.

FLOW SIGN CONVENTION:
Hub flow points in direction of the flow so if the heli is flying in +z direction and pointing in the +z
direction then the hub flow is in the -z direction The blade element velocities are in the direction of
motion of the element (basically along the chord line) so in sector 2 the element velocity is in the +z
direction. Thus the difference between the element velocity and the hub flow gives the local flow for the
element In the example above the sector 2 local flows will be in the -z direction. So, local flow = hub
flow - element velocity


We also need to deal with the fact that the TPP of the rotor, flybar etc is dictated by the aerodynamic
forces on these rotors (except for the tail which is forced to orient with the heli) but that the control
axes for the cyclic commands change as the heading of the heli changes. We therefore need to generate the
InplaneX and InplaneZ vectors to match with the heli orientation before the local flow at the four
sectors can be calculated. InplaneX corresponds to the 90 degree azimuth sector[2] position and InplaneZ
to the sector[3] position. ModelEarVector <x> TPP is perpendicular to both the TPP and the models
transverse axis and I believe this is InPlaneZ InplaneX must be at rightangles to both TPP and InplaneZ
so we have:- Rotor.InPlaneZ:= VUnit(VCross(ModelEarVector, Rotor.TPP)) Rotor.InPlaneX:=
VUnit(VCross(Rotor.TPP, InPlaneZ) I HOPE!!

The hub flow can then be calculated by taking components in the InPlaneX, TPP, and InPlaneZ directions as
the x, y, and z components in the Rotor frame. For the individual blade elements (Surfaces) it only
remains to add the blade speed to the hub flow.

The sum of the rotor forces must then be translated back into either the airframe (EarVec, ShaftVec,
NoseVec frame) or the ground frame. Its probably best to use Total groundframe force = Force.x * InplaneX
+ Force.y * TPP + Force.z*InplaneZ we can then take components in the aircraft fram if we like. A direct
transformation to aircraft frame is probably a bit more of a fiddle but may be eased by it being only a
'small angle' job.


*/

/* Important note about DLL memory management: ShareMem must be the
  first unit in your library's USES clause AND your project's (select
  Project-View Source) USES clause if your DLL exports any procedures or
  functions that pass strings as parameters or function results. This
  applies to all strings passed to and from your DLL--even those that
  are nested in records and classes. ShareMem is the interface unit to
  the BORLNDMM.DLL shared memory manager, which must be deployed along
  with your DLL. To avoid using BORLNDMM.DLL, pass string information
  using PChar or ShortString parameters. */

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <WPVehiclePhysics/CAerodymanicsWind.hpp>
#include <WPVehiclePhysics/VecMath.hpp>
#include <WPVehiclePhysics/HeliVars.hpp>
#include <WPVehiclePhysics/EngineClutchUnit.hpp>
#include <WPVehiclePhysics/CFlybarlessUnit.hpp>
#include <WPVehiclePhysics/CGyroUnit.hpp>
#include <WPVehiclePhysics/CGovernorUnit.hpp>
#include <WPVehiclePhysics/CBatteryPack.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @class HeliAero
         * @brief Comprehensive aerodynamics simulation for RC helicopters
         *
         * HeliAero provides a complete aerodynamic model for single main rotor helicopters
         * with optional flybar systems. It handles:
         * - Blade element theory for main and tail rotors
         * - Induced flow calculations with ground effect
         * - Rotor precession and teeter dynamics
         * - Control linkage simulation (cyclic, collective, tail)
         * - Power system modeling (ICE or electric)
         * - Flybarless stabilization systems
         * - Gyroscopic tail control
         *
         * The simulation uses a multi-sector approach to model airflow at different
         * azimuth positions around the rotor disc, accounting for:
         * - Forward flight asymmetry
         * - Climb/descent induced velocity changes
         * - Ground effect modifications
         * - Blade flapping and lead-lag
         *
         * @note This class uses the Singleton pattern for global access
         */
        class WPVehiclePhysics_API HeliAero : public ISharedObject
        {
        public:
            /// Constructor - initializes default helicopter parameters
            HeliAero();

            /// Destructor - cleans up resources
            ~HeliAero() override;

            /// Prints debug information to console
            void debugPrint();

            /**
             * @brief Constrains rotor tilt angles to prevent excessive deflection
             * @param Rotor Reference to the rotor to constrain
             * @param ShaftFrame The shaft's frame of reference
             * @param AngleLimit Maximum allowed tilt angle in radians
             */
            void cageRotor( TRotor &Rotor, FrameOfRef &ShaftFrame, physics_Num AngleLimit );

            /// Defines and initializes all flight parameters from configuration
            void defineFlightParameters();

            /**
             * @brief Parses XML configuration string to extract parameter values
             * @param XMLString XML formatted string containing parameters
             */
            void findAllParameterValuesIn( const String &XMLString );

            /// Loads default parameter values for the helicopter
            void readDefaultParameterValues();

            /// Reads current helicopter parameters from configuration (exported to Python)
            void readCurrentHeliParameters() /* export */;

            /**
             * @brief Sets callback function pointer for external communication
             * @param FuncPtr Pointer to callback function
             */
            void setFunc( void *FuncPtr );

            /// Calls the registered callback function
            void callFunc();

            /**
             * @brief Configures teeter hinge parameters
             * @param TeeterSpring Spring constant for teeter restraint
             * @param TeeterDamping Damping coefficient for teeter motion
             * @param IndS Induced smoothing factor
             */
            void setTeeter( f32 TeeterSpring, f32 TeeterDamping, f32 IndS );

            /**
             * @brief Configures flybar physical properties
             * @param R1 Inner radius of flybar paddles
             * @param R2 Outer radius of flybar paddles
             * @param C1 Root chord of flybar paddles
             * @param C2 Tip chord of flybar paddles
             * @param Weight Total flybar weight
             * @param IntFac Interference factor between main rotor and flybar
             */
            void setFlybar( f32 R1, f32 R2, f32 C1, f32 C2, f32 Weight, f32 IntFac );

            /**
             * @brief Configures ground effect parameters
             * @param Max Maximum ground effect multiplier
             * @param Decay Height decay rate for ground effect
             */
            void setGroundEffect( f32 Max, f32 Decay );

            /**
             * @brief Enables or disables linkage control mode
             * @param LC True to enable linkage control, false for direct control
             */
            void setLinkageControl( bool LC );

            /**
             * @brief Controls which forces are applied to the simulation
             * @param BF Body forces flag (0=off, 1=on)
             * @param RF Rotor forces flag (0=off, 1=on)
             */
            void gateForces( int BF, int RF );

            /**
             * @brief Sets drag coefficient multiplier for tuning
             * @param CDFactor Multiplier for drag coefficients
             * @param Gash Reserved parameter (legacy)
             */
            void setCDMultiplier( f32 CDFactor, f32 Gash );

            /// Retrieves yaw channel input from transmitter
            void getYawChannel();

            /// Retrieves all transmitter channel data
            void getAllTxData();

            /**
             * @brief Commands tail servo based on gyro deviation
             * @param Deviation Angular deviation to correct (radians)
             */
            void driveTailServo( physics_Num Deviation );

            /**
             * @brief Sends flybarless controller outputs to swashplate mixer
             * @param AilDeflection Aileron deflection command
             * @param EleDeflection Elevator deflection command
             */
            void sendFlybarlessControlsToMixer( physics_Num AilDeflection, physics_Num EleDeflection );

            /// Retrieves control information from input sources
            void getControlInfo();

            /// Reads all transmitter channel values
            void getTxChannels();

            /// Retrieves current engine/motor output power
            void getEngineOutput();

            /// Retrieves model angular velocity from physics system
            void getModelAngularVelocity();

            /// Retrieves model linear velocity from physics system
            void getModelLinearVelocity();

            /**
             * @brief Adds a force to a body at a specific location
             * @param Bdy Body ID to apply force to
             * @param Force Force vector in local coordinates
             * @param Loc Location of force application in local coordinates
             * @param ForceID Identifier for debugging/tracking
             */
            void addLocalForce( int Bdy, const physics_Vec &Force, const physics_Vec &Loc, f32 ForceID );

            /**
             * @brief Adds a torque to a body
             * @param Bdy Body ID to apply torque to
             * @param Torque Torque vector in local coordinates
             */
            void addLocalTorque( int Bdy, const physics_Vec &Torque );

            /**
             * @brief Displays a vector for debugging visualization
             * @param Bdy Body ID for coordinate reference
             * @param V Vector to display
             * @param Org Origin point for vector display
             */
            void displayLocalVector( int Bdy, const physics_Vec &V, const physics_Vec &Org );

            /**
             * @brief Callback function for external system communication
             * @param N1 First callback parameter
             * @param N2 Second callback parameter
             * @param TP Pointer to pass data array
             */
            void callback( int N1, int N2, TPassAPtr TP );

            /// Retrieves model position from physics system
            void getModelPosition();

            /// Retrieves model orientation frame from physics system
            void getModelFrame();

            /// Retrieves visual rotor orientation frame
            void getVisualRotorFrame();

            /// Retrieves visual rotor angular velocity for rendering
            void getVisualRotorAngularVelocity();

            /**
             * @brief Calculates distance to ground in specified direction
             * @param Dir Direction vector to check
             * @param Loc Starting location for ray cast
             * @return Distance to ground intersection
             */
            f32 getGroundDistance( const physics_Vec &Dir, const physics_Vec &Loc );

            /// Reads model data from configuration files
            void readModelData();

            /**
             * @brief Rotates induced velocity vectors for rotor orientation changes
             * @param Rotor Reference to rotor being rotated
             * @param PitchAng Pitch angle in radians
             * @param YawAng Yaw angle in radians
             * @param RollAng Roll angle in radians
             */
            void rotateInduced( TRotor &Rotor, double PitchAng, double YawAng, double RollAng );

            /// Initializes all helicopter systems and components
            void initialize();

            /**
             * @brief Initializes blade surface elements for aerodynamic calculations
             * @param Rotor Reference to rotor to initialize
             */
            void initSurfaces( TRotor &Rotor );
            /// Calculates airflow in rotor reference frames for all rotors
            void calcAllRotorFrameFlows();

            /**
             * @brief Calculates airflow at arc sectors around rotor disc
             * @param Rotor Reference to rotor for flow calculations
             */
            void calcMainArcFlows( TRotor &Rotor );

            /**
             * @brief Applies downwash interference from main rotor to flybar
             * @param IFac Interference factor (0.0 to 1.0)
             */
            void mainToFlyBarInterference( physics_Num IFac );

            /**
             * @brief Updates blade surface element states
             * @param Rotor Reference to rotor to update
             */
            void updateSurfaces( TRotor &Rotor );

            /**
             * @brief Calculates induced velocity correction factor for climb/descent
             * @param ClimbRate Vertical climb rate
             * @return Correction factor for induced velocity
             */
            physics_Num climbFactor( physics_Num ClimbRate );

            /**
             * @brief Calculates induced velocity correction for forward flight
             * @param TransRate Forward translation rate
             * @return Correction factor for induced velocity
             */
            physics_Num translationFactor( physics_Num TransRate );

            /**
             * @brief Calculates ground effect multiplier based on height
             * @param Ht Height above ground
             * @return Ground effect multiplier (1.0 = no effect, >1.0 = ground effect)
             */
            physics_Num groundFactor( physics_Num Ht );

            /// Calculates induced velocity at all blade elements
            void calcInduced();

            /**
             * @brief Applies smoothing filter to induced velocities
             * @param Rotor Reference to rotor
             * @param S Smoothing factor (time constant)
             */
            void smoothInduced( TRotor &Rotor, physics_Num S );

            /**
             * @brief Calculates angle of attack for a blade surface element
             * @param S Reference to surface element
             */
            void calcAttack( TSurface &S );

            /**
             * @brief Finds maximum angle of attack across all blades
             * @param Max Output: maximum angle of attack found
             * @param Rotor Reference to rotor to analyze
             */
            void calcAlphaLMax( physics_Num &Max, TRotor &Rotor );

            /**
             * @brief Loads lookup table for climb/translation corrections
             * @param FName Filename of lookup table
             * @param Table Output: loaded lookup table
             */
            void getClimbTransLookup( String FName, LookupArray &Table );

            /**
             * @brief Loads airfoil lift/drag characteristics from file
             * @param IFileName Input filename
             * @param AFoilLookup Output: airfoil lookup table
             */
            void getFoilLookup( const String &IFileName, TFoilTable &AFoilLookup );

            /**
             * @brief Reads CL and CD values from lookup table for surface
             * @param S Reference to surface element
             * @param AFoilLookup Airfoil lookup table
             */
            void readCLandD( TSurface &S, TFoilTable &AFoilLookup );

            /**
             * @brief Calculates lift and drag forces for a surface element
             * @param S Reference to surface element
             */
            void calcLandD( TSurface &S );

            /**
             * @brief Integrates forces from all blade elements
             * @param Rotor Reference to rotor
             */
            void getForcesFromFlow( TRotor &Rotor );

            /**
             * @brief Zeros accumulated forces and moments for rotor
             * @param Rotor Reference to rotor to zero
             */
            void zeroForcesAndMoments( TRotor &Rotor );

            /**
             * @brief Sums forces and moments from all blade elements
             * @param Rotor Reference to rotor
             */
            void sumForcesandMoments( TRotor &Rotor );

            /// Aligns all coordinate frames to current model orientation
            void alignFrames();

            /// Sorts and updates frame relationships and linkage geometry
            void sortFramesAndLinks();

            /**
             * @brief Adjusts teeter factors based on RPM and dynamics
             * @param FC Force coefficient to correct
             * @param DC Damping coefficient to correct
             * @param FrameTime Simulation timestep
             * @param RPM Current rotor RPM
             * @param MaxRPM Maximum rated rotor RPM
             * @param AngMom Angular momentum vector
             * @param MoI Moment of inertia vector
             */
            void correctTeeterFactors( physics_Num &FC, physics_Num &DC, physics_Num FrameTime,
                                       physics_Num RPM, physics_Num MaxRPM, physics_Num AngMom,
                                       const physics_Vec &MoI );

            /// Calculates rotor precession and teeter motion
            void precessAndTeeter();

            /// Experimental alternative precession and teeter calculation
            void experimentalPrecessAndTeeter();

            /**
             * @brief Calculates gyroscopic precession for rotor
             * @param dt Simulation timestep
             * @param Rotor Reference to rotor
             */
            void doPrecession( physics_Num dt, TRotor &Rotor );

            /**
             * @brief Calculates forces in teeter hinge
             * @param Head Reference to rotor head
             */
            void calcTeeterForces( TRotorHead &Head );

            /**
             * @brief Initializes tail rotor control linkage
             * @param TailLinkage Reference to tail linkage structure
             */
            void initTailLinkage( TailLinkage &TailLinkage );

            /**
             * @brief Processes tail rotor linkage kinematics
             * @param InSig Input signal to linkage
             * @param TL Reference to tail linkage
             * @return Output signal from linkage
             */
            physics_Num doTailLinkage( physics_Num InSig, TailLinkage &TL );

            /**
             * @brief Initializes servo parameters
             * @param Servo Reference to servo structure
             * @param SecPer60 Time in seconds for 60 degrees of travel
             */
            void initServo( Servo &Servo, physics_Num SecPer60 );

            /**
             * @brief Simulates servo response with rate limiting
             * @param InSig Input signal
             * @param dt Timestep
             * @param ThisServo Reference to servo
             * @return Current servo position
             */
            physics_Num servo( physics_Num InSig, physics_Num dt, Servo &ThisServo );

            /**
             * @brief Initializes gear train ratios and inertias
             * @param Gears Reference to gear train structure
             */
            void initGearTrain( GearTrain &Gears );

            /// Calculates energy budget for internal combustion engine
            void doICEnergyBudget();

            /// Calculates energy budget for electric motor system
            void doElecEnergyBudget();
            /**
             * @brief Exports current rotor head RPM to Python
             * @return Main rotor RPM
             */
            f32 headRPMToPy() /* export */;

            /**
             * @brief Exports battery pack state to Python
             * @return Pack state (0.0-1.0, where 1.0 is fully charged)
             */
            f32 packStateToPy() /* export */;

            /**
             * @brief Exports battery pack voltage to Python
             * @return Pack voltage in volts
             */
            f32 packVoltsToPy() /* export */;

            /**
             * @brief Exports motor power consumption to Python
             * @return Motor power in watts
             */
            f32 motorPowerToPy() /* export */;

            /**
             * @brief Exports engine RPM to Python
             * @return Engine RPM
             */
            f32 engineRPMToPy() /* export */;

            /**
             * @brief Exports effective RPM for sound synthesis to Python
             * @return Sound synthesis RPM
             */
            f32 soundRPMToPy() /* export */;

            /**
             * @brief Returns maximum angle of attack on main rotor
             * @return Maximum alpha in radians
             */
            f32 maxMainAttackToPy();

            /**
             * @brief Returns maximum angle of attack on tail rotor
             * @return Maximum alpha in radians
             */
            f32 maxTailAttackToPy();

            /**
             * @brief Calculates smoke flow vectors for visual effects
             * @param RootSmokeFlowX Root flow X component output
             * @param RootSmokeFlowY Root flow Y component output
             * @param RootSmokeFlowZ Root flow Z component output
             * @param TipSmokeFlowX Tip flow X component output
             * @param TipSmokeFlowY Tip flow Y component output
             * @param TipSmokeFlowZ Tip flow Z component output
             */
            void smokeFlow( physics_Num &RootSmokeFlowX, physics_Num &RootSmokeFlowY,
                            physics_Num &RootSmokeFlowZ, physics_Num &TipSmokeFlowX,
                            physics_Num &TipSmokeFlowY, physics_Num &TipSmokeFlowZ );

            /**
             * @brief Retrieves blade tip pressure and drag data for sound synthesis
             * @param TipPress1 Blade 1 tip pressure output
             * @param TipPress2 Blade 2 tip pressure output
             * @param TipPress3 Blade 3 tip pressure output
             * @param TipPress4 Blade 4 tip pressure output
             * @param TipDrg1 Blade 1 tip drag output
             * @param TipDrg2 Blade 2 tip drag output
             * @param TipDrg3 Blade 3 tip drag output
             * @param TipDrg4 Blade 4 tip drag output
             */
            void soundData( physics_Num &TipPress1, physics_Num &TipPress2, physics_Num &TipPress3,
                            physics_Num &TipPress4, physics_Num &TipDrg1, physics_Num &TipDrg2,
                            physics_Num &TipDrg3, physics_Num &TipDrg4 );

            /// Resets all simulation state to initial conditions
            void resetAll();

            /// Outputs visual rotor forces for debugging/display
            void outputVisualRotorForces();

            /// Outputs visual rotor torques for debugging/display
            void outputVisualRotorTorques();

            /// Outputs all forces to physics system
            void outputForces();

            /// Tests force calculations (debugging)
            void testForces();

            /**
             * @brief Special test frame for debugging specific scenarios
             * @param t Current simulation time
             * @param dt Timestep
             */
            void specialTestFrame( f32 t, f32 dt );

            /// Draws VBar controller state for visualization
            void drawVBar();

            /**
             * @brief Workbench mode frame for parameter tuning
             * @param t Current simulation time
             * @param dt Timestep
             * @param ParamID Parameter ID to vary
             * @param ParamValue Parameter value to test
             */
            void workBenchFrame( f32 t, f32 dt, int ParamID, f32 ParamValue ) /* export */;

            /**
             * @brief Main simulation frame - called each physics step
             * @param t Current simulation time
             * @param dt Timestep duration
             */
            void mainFrame( real_dNum t, real_dNum dt );

            /// Displays test information (exported to Python)
            void testDisplay() /* export */;

            /**
             * @brief Levitates model for balance testing
             * @param Time Total levitation time
             * @param SpinTime Time to reach desired spin
             * @param SpinSpeed Target spin speed in RPM
             */
            void levitate( f32 Time, f32 SpinTime, f32 SpinSpeed ); /* export */

            /**
             * @brief Retrieves model mass properties (exported to Python)
             * @param ModelMass Output: total model mass
             * @param PMoI Output: pitch moment of inertia
             * @param RMoI Output: roll moment of inertia
             * @param YMoI Output: yaw moment of inertia
             * @param HeadMass Output: rotor head mass
             */
            void getModelMassProps( f32 &ModelMass, f32 &PMoI, f32 &RMoI, f32 &YMoI,
                                    f32 &HeadMass ); /* export */

            /**
             * @brief Sets path to data directory for configuration files
             * @param param Wide string path to data directory
             */
            void setDataPath( const StringW &param );

            /**
             * @brief Gets current data directory path
             * @return Wide string path to data directory
             */
            StringW getDataPath() const;

            /**
             * @brief Sets initial rotor RPM for startup
             * @param rpm Target RPM value
             */
            void setCurrentRPM( f32 rpm );

            /**
             * @brief Configures bailout/crash detection parameters
             * @param bailout First bailout parameter
             * @param bailout2 Second bailout parameter
             * @param bailout3 Third bailout parameter (integer)
             */
            void setBailout( f32 bailout, f32 bailout2, s32 bailout3 );

            /// Loads default gyro parameters from configuration
            void readDefaultGyroParameters();

            /// Defines gyro parameter structure
            void defineGyroParameters();

            /**
             * @brief Parses user gyro parameters from XML
             * @param XMLString XML string containing gyro parameters
             */
            void gyroUserFindAllParameterValuesIn( const String &XMLString );

            /**
             * @brief Parses gyro parameters from XML
             * @param XMLString XML string containing gyro parameters
             */
            void gyroFindAllParameterValuesIn( const String &XMLString );

            /// Reads current gyro parameter values
            void readCurrentGyroParameters();

            /**
             * @brief Parses governor parameters from XML
             * @param XMLString XML string containing governor parameters
             */
            void govFindAllParameterValuesIn( const String &XMLString );

            /// Defines governor parameter structure
            void defineGovernorParameters();

            /// Reads current governor parameter values
            void readCurrentGovernorParameters();

            /// Loads default governor parameters from configuration
            void readDefaultGovernorParameters();

            /**
             * @brief Gets singleton instance of HeliAero
             * @return Shared pointer to singleton instance
             */
            static std::shared_ptr<HeliAero> &getSingleton();

            // ===== Rotor Head Parameters =====
            VehicleParam
                m_rotorHeadHubPosition;     ///< Position of rotor hub relative to model origin (meters)
            VehicleParam m_modelWeight;     ///< Total model weight (kg)
            VehicleParam m_modelMoI;        ///< Model moments of inertia (kg*m�)
            VehicleParam m_mainRotorWeight; ///< Main rotor assembly weight (kg)
            VehicleParam m_rotorHeadBoltRadius;     ///< Radius from hub center to blade bolt (meters)
            VehicleParam m_rotorHeadNumBlades;      ///< Number of main rotor blades (typically 2)
            VehicleParam m_rotorHeadSwashToMainMix; ///< Mixing ratio from swashplate to main rotor
            VehicleParam m_rotorHeadSwashToFBMix;   ///< Mixing ratio from swashplate to flybar
            VehicleParam m_rotorHeadFBtoMainMix;    ///< Mixing ratio from flybar to main rotor
            VehicleParam m_rotorHeadMaxSwashEle;    ///< Maximum swashplate elevator deflection (degrees)
            VehicleParam m_rotorHeadMaxSwashAil;    ///< Maximum swashplate aileron deflection (degrees)
            VehicleParam m_rotorHeadCollectivePerMM; ///< Collective pitch change per mm of servo travel
                                                     ///< (degrees/mm)
            VehicleParam m_rotorHeadShaftRake;       ///< Shaft rake angle - forward tilt (degrees)
            VehicleParam m_rotorHeadShaftTilt;       ///< Shaft tilt angle - lateral tilt (degrees)
            VehicleParam m_rotorHeadTeeterFC;        ///< Teeter force coefficient (spring constant)
            VehicleParam m_rotorHeadTeeterDC;        ///< Teeter damping coefficient
            // ===== Main Rotor Blade Parameters =====
            VehicleParam m_mainBladeLength;     ///< Main blade length from hub to tip (meters)
            VehicleParam m_mainBladeRootRadius; ///< Radius where aerodynamic blade starts (meters)
            VehicleParam m_mainBladeTipChord;   ///< Blade chord at tip (meters)
            VehicleParam m_mainBladeRootChord;  ///< Blade chord at root (meters)
            VehicleParam m_mainBladeTwist;      ///< Blade twist from root to tip (degrees)
            VehicleParam m_mainBladeWeight;     ///< Weight of single main blade (kg)
            VehicleParam m_mainBladeRadOfGyr;   ///< Radius of gyration for blade inertia (meters)
            // ===== Tail Rotor Parameters =====
            VehicleParam
                m_tailHubPosition; ///< Position of tail rotor hub relative to model origin (meters)
            VehicleParam m_tailHubBoltRadius; ///< Radius from tail hub center to blade bolt (meters)
            VehicleParam m_tailNumBlades;     ///< Number of tail rotor blades (typically 2)
            VehicleParam m_tailMaxPitch;      ///< Maximum tail blade pitch angle (degrees)
            VehicleParam m_tailPitchTrim;   ///< Tail pitch trim offset for torque compensation (degrees)
            VehicleParam m_tailBladeLength; ///< Tail blade length from hub to tip (meters)
            VehicleParam m_tailBladeRootRadius; ///< Radius where aerodynamic tail blade starts (meters)
            VehicleParam m_tailBladeTipChord;   ///< Tail blade chord at tip (meters)
            VehicleParam m_tailBladeRootChord;  ///< Tail blade chord at root (meters)
            VehicleParam m_tailBladeTwist;      ///< Tail blade twist from root to tip (degrees)
            VehicleParam m_tailBladeWeight;     ///< Weight of single tail blade (kg)
            VehicleParam m_tailBladeRadOfGyr;   ///< Radius of gyration for tail blade inertia (meters)
            // ===== Flybar Parameters =====
            VehicleParam m_flybarless;      ///< True if helicopter uses flybarless system
            VehicleParam m_paddleWeight;    ///< Weight of flybar paddles (kg)
            VehicleParam m_paddleSpan;      ///< Span of flybar paddles (meters)
            VehicleParam m_paddleRootChord; ///< Paddle chord at root (meters)
            VehicleParam m_paddleTipChord;  ///< Paddle chord at tip (meters)
            VehicleParam
                m_paddleThreadOnLength; ///< Length of threaded portion for paddle adjustment (meters)
            VehicleParam m_flybarRodDiameter; ///< Diameter of flybar rod (meters)
            VehicleParam m_flybarRodDensity;  ///< Material density of flybar rod (kg/m�)
            VehicleParam m_flybarRodLength;   ///< Total length of flybar rod (meters)
            VehicleParam
                m_mainToFBInterference; ///< Downwash interference factor from main to flybar (0.0-1.0)

            // ===== Ground Effect Parameters =====
            VehicleParam m_groundEffectMax;   ///< Maximum ground effect multiplier (typically 1.1-1.3)
            VehicleParam m_groundEffectDecay; ///< Height decay rate for ground effect (meters)
            // ===== Tuning Parameters =====
            VehicleParam m_vrMultiplier; ///< Visual rotor speed multiplier for rendering
            VehicleParam m_cdMultiplier; ///< Drag coefficient multiplier for tuning

            // ===== Drive Train Parameters =====
            VehicleParam m_mainGearTeeth;   ///< Number of teeth on main gear
            VehicleParam m_pinionGearTeeth; ///< Number of teeth on pinion gear
            VehicleParam m_tailGearing;     ///< Tail rotor gear ratio relative to main rotor
            VehicleParam m_drivenTail;      ///< True if tail is belt/shaft driven, false if electric

            // ===== Body Drag Parameters =====
            VehicleParam m_bodyCdA;        ///< Body drag coefficient * area (m�)
            VehicleParam m_bodyDragCentre; ///< Location of drag center relative to CG (meters)
            // ===== Engine Parameters (ICE) =====
            VehicleParam m_enginePeakPower;    ///< Peak engine power output (watts)
            VehicleParam m_enginePeakPowerRPM; ///< RPM at which peak power occurs
            VehicleParam m_engineMoI;          ///< Engine rotating assembly moment of inertia (kg*m�)
            VehicleParam m_clutchMoI;          ///< Clutch moment of inertia (kg*m�)
            VehicleParam m_clutchConst;        ///< Clutch engagement rate constant
            VehicleParam m_clutchBiteRPM;      ///< RPM at which clutch begins to engage
            // ===== Flybarless Controller (VBar) Parameters =====
            VehicleParam m_vBarStickDeadBand;    ///< Stick deadband zone (0.0-1.0)
            VehicleParam m_vBarStickSensitivity; ///< Stick sensitivity multiplier
            VehicleParam m_vBarStickExponential; ///< Stick exponential factor (0.0-1.0)
            VehicleParam m_vBarAilGain;          ///< Aileron stabilization gain
            VehicleParam m_vBarEleGain;          ///< Elevator stabilization gain
            VehicleParam m_vBarAngleLimit;       ///< Maximum allowed attitude angle (degrees)
            VehicleParam m_vBarDecayTime;        ///< Time constant for attitude hold decay (seconds)
            VehicleParam m_vBarStabilize;        ///< Stabilization mode enable flag
            VehicleParam m_vBarStabGain;         ///< Overall stabilization gain
            VehicleParam m_vBarDirectMix;        ///< Direct stick-to-swash mixing ratio (0.0-1.0)
            VehicleParam m_vBarAilStickFilter;   ///< Aileron stick input filter time constant
            VehicleParam m_vBarEleStickFilter;   ///< Elevator stick input filter time constant
            // ===== Electric Power System Parameters =====
            VehicleParam m_electricPower;      ///< True if model uses electric power
            VehicleParam m_emulateBattery;     ///< True to simulate battery discharge
            VehicleParam m_cellsInPack;        ///< Number of cells in battery pack (e.g., 6S = 6)
            VehicleParam m_cellFullV;          ///< Fully charged cell voltage (volts)
            VehicleParam m_cellFlatV;          ///< Discharged/flat cell voltage (volts)
            VehicleParam m_cellR;              ///< Cell internal resistance (ohms)
            VehicleParam m_cellAHr;            ///< Cell capacity (amp-hours)
            VehicleParam m_motorMoI;           ///< Motor rotor moment of inertia (kg*m�)
            VehicleParam m_motorKV;            ///< Motor velocity constant (RPM/volt)
            VehicleParam m_motorEfficiency;    ///< Motor efficiency (0.0-1.0)
            VehicleParam m_motorNoLoadCurrent; ///< Motor no-load current draw (amps)
            VehicleParam m_motorR;             ///< Motor winding resistance (ohms)
            VehicleParam m_motorILimit;        ///< Motor current limit (amps)
            // ===== ESC (Electronic Speed Controller) Parameters =====
            VehicleParam m_escSlowRampTC;       ///< Slow ramp time constant for RPM changes (seconds)
            VehicleParam m_escFastRampTC;       ///< Fast ramp time constant for rapid throttle (seconds)
            VehicleParam m_escSoftStartDelay;   ///< Delay before soft start begins (seconds)
            VehicleParam m_escAccelerationGain; ///< PID acceleration term gain
            VehicleParam m_escRPMGain;          ///< PID proportional RPM gain
            VehicleParam m_escPhaseGain;        ///< PID derivative phase gain
            VehicleParam m_escAccelerationLimit; ///< Maximum acceleration term contribution
            VehicleParam m_escRPMErrorLimit;     ///< Maximum RPM error term contribution
            VehicleParam m_escPhaseErrorLimit;   ///< Maximum phase error term contribution
            VehicleParam m_escMinControlPoint;   ///< Minimum throttle control point (0.0-1.0)
            VehicleParam m_escILimit;            ///< ESC current limit (amps)
            VehicleParam m_escCutoffV;           ///< Low voltage cutoff threshold (volts)
            VehicleParam m_escResistance;        ///< ESC internal resistance (ohms)

            // ===== Miscellaneous Parameters =====
            VehicleParam m_visualTailReverse; ///< Reverse visual tail rotor direction
            VehicleParam m_directTailControl; ///< Use direct tail control (bypass gyro)
            VehicleParam m_bearingFriction;   ///< Bearing friction coefficient

            // ===== Gyro Parameters =====
            VehicleParam m_gyroServoOffset;      ///< Servo center offset for tail (degrees)
            VehicleParam m_gyroThrowLimit1;      ///< First throw limit angle (degrees)
            VehicleParam m_gyroThrowLimit2;      ///< Second throw limit angle (degrees)
            VehicleParam m_gyroStickDeadband;    ///< Rudder stick deadband (0.0-1.0)
            VehicleParam m_gyroStickSensitivity; ///< Rudder stick sensitivity multiplier
            VehicleParam m_gyroStickExponential; ///< Rudder stick exponential (0.0-1.0)
            VehicleParam m_gyroDirectCoupling;   ///< Direct coupling gain from stick
            VehicleParam m_gyroYawErrorLimit;    ///< Maximum yaw error for correction (radians)
            VehicleParam m_gyroConventionalGain; ///< Conventional mode gyro gain
            VehicleParam m_gyroHLRange;          ///< Heading-lock mode range (degrees)
            VehicleParam m_gyroHLGain;           ///< Heading-lock mode gain
            VehicleParam m_gyroSenseReverse;     ///< Reverse gyro sense direction
            VehicleParam m_gyroHLKillTime; ///< Time to disable heading lock after stick input (seconds)
            VehicleParam m_gyroHLDecay;    ///< Heading lock decay rate
            VehicleParam m_gyroAccTimeConst;  ///< Accelerometer time constant (seconds)
            VehicleParam m_gyroAccGain;       ///< Accelerometer feedback gain
            VehicleParam m_gyroAccTermLimit;  ///< Maximum accelerometer term contribution
            VehicleParam m_gyroLeftStopGain;  ///< Left mechanical stop compensation gain
            VehicleParam m_gyroRightStopGain; ///< Right mechanical stop compensation gain
            CGyroUnit    m_tailGyro;          ///< Tail gyro controller instance

            // ===== Governor Parameters =====
            VehicleParam m_govAccelerationGain;  ///< PID acceleration term gain for governor
            VehicleParam m_govRPMGain;           ///< PID proportional RPM gain for governor
            VehicleParam m_govPhaseGain;         ///< PID derivative phase gain for governor
            VehicleParam m_govAccelerationLimit; ///< Maximum acceleration term for governor
            VehicleParam m_govRPMErrorLimit;     ///< Maximum RPM error term for governor
            VehicleParam m_govPhaseErrorLimit;   ///< Maximum phase error term for governor
            VehicleParam m_govMinControlPoint;   ///< Minimum governor control point (0.0-1.0)
            VehicleParam m_govReqHeadRPM;        ///< Requested/target head speed RPM
            VehicleParam m_govRampRate;          ///< RPM ramp rate (RPM/second)
            VehicleParam m_govMode;              ///< Governor mode (0=off, 1=standard, 2=advanced)
            VehicleParam m_govMinHeadRPM;        ///< Minimum allowed head speed RPM
            VehicleParam m_govMaxHeadRPM;        ///< Maximum allowed head speed RPM

            // ===== Coordinate Frames and Structures =====
            FrameOfRef  m_modelFrame;       ///< Model body coordinate frame
            FrameOfRef  m_visualRotorFrame; ///< Visual rotor coordinate frame
            GearTrain   m_theGears;         ///< Gear train configuration
            LookupArray m_climbLookup;      ///< Induced velocity lookup for climb
            LookupArray m_transLookup;      ///< Induced velocity lookup for translation

            HelicopterBody m_body;        ///< Body aerodynamics structure
            Servo          m_tailServo;   ///< Tail rotor servo
            TailLinkage    m_tailLinkage; ///< Tail control linkage
            Servo          m_eleServo;    ///< Elevator servo
            Servo          m_ailServo;    ///< Aileron servo
            Servo          m_colServo;    ///< Collective servo

            // ===== State Vectors =====
            physics_Vec m_wind = physics_Vec::zero();          ///< Wind velocity vector (m/s)
            physics_Vec m_modelVelocity = physics_Vec::zero(); ///< Model velocity in world frame (m/s)
            physics_Vec m_modelAngularVelocity = physics_Vec::zero(); ///< Model angular velocity (rad/s)
            physics_Vec m_modelPosition =
                physics_Vec::zero(); ///< Model position in world frame (meters)
            physics_Vec m_visualRotorAngularVelocity =
                physics_Vec::zero(); ///< Visual rotor angular velocity (rad/s)

            physics_Vec m_hopTestForce = physics_Vec::zero(); ///< Test force for hop tests (Newtons)
            physics_Vec m_totalFlow = physics_Vec::zero();    ///< Total airflow vector (m/s)

            physics_Num m_roair; ///< Air density (kg/m�)

            // ===== Simulation Timing =====
            size_t    m_loops = 0; ///< Total number of simulation loops executed
            real_dNum m_maxDeltaT =
                static_cast<physics_Num>( 0.0 ); ///< Maximum timestep observed (seconds)
            real_dNum m_minDeltaT =
                static_cast<physics_Num>( 0.0 ); ///< Minimum timestep observed (seconds)
            real_dNum m_lastDeltaT =
                static_cast<physics_Num>( 0.0 ); ///< Previous frame timestep (seconds)
            real_dNum m_thisDeltaT =
                static_cast<physics_Num>( 0.0 ); ///< Current frame timestep (seconds)
            real_dNum m_nextDeltaT =
                static_cast<physics_Num>( 0.0 ); ///< Predicted next timestep (seconds)
            real_dNum m_meanDeltaT = static_cast<physics_Num>( 0.0 ); ///< Mean timestep (seconds)
            real_dNum m_simTime =
                static_cast<physics_Num>( 0.0 ); ///< Total simulation time elapsed (seconds)
            real_dNum m_sumOfDeltas =
                static_cast<physics_Num>( 0.0 ); ///< Sum of all timesteps (seconds)

            // ===== Aerodynamic State Variables =====
            physics_Num m_inducedSmoothing =
                static_cast<physics_Num>( 0.0 ); ///< Induced flow smoothing time constant
            physics_Num m_interferenceFactor =
                static_cast<physics_Num>( 0.0 ); ///< Main-to-flybar interference factor
            physics_Num m_geMax = static_cast<physics_Num>( 0.0 ); ///< Current ground effect maximum
            physics_Num m_geDecay =
                static_cast<physics_Num>( 0.0 ); ///< Current ground effect decay rate
            physics_Num m_alphaLMaxMain =
                static_cast<physics_Num>( 0.0 ); ///< Maximum angle of attack on main rotor (radians)
            physics_Num m_alphaLMaxTail =
                static_cast<physics_Num>( 0.0 ); ///< Maximum angle of attack on tail rotor (radians)

            // ===== Control System State =====
            physics_Num m_tailGyroInput = static_cast<physics_Num>( 0.0 );  ///< Input to tail gyro
            physics_Num m_tailGyroGain = static_cast<physics_Num>( 0.0 );   ///< Current tail gyro gain
            physics_Num m_tailGyroOutput = static_cast<physics_Num>( 0.0 ); ///< Output from tail gyro
            physics_Num m_rxThrottleSig =
                static_cast<physics_Num>( 0.0 ); ///< Receiver throttle signal (0.0-1.0)

            // ===== Power System State =====
            physics_Num m_engineRPM = static_cast<physics_Num>( 0.0 ); ///< Current engine RPM
            physics_Num m_governorOutput =
                static_cast<physics_Num>( 0.0 ); ///< Governor output signal (0.0-1.0)
            physics_Num m_engineThrottlePosition =
                static_cast<physics_Num>( 0.0 ); ///< Physical throttle position
            physics_Num m_engineOutput =
                static_cast<physics_Num>( 0.0 ); ///< Engine power output (watts)

            // ===== Tuning/Debugging Variables =====
            physics_Num m_clFiddle =
                static_cast<physics_Num>( 0.0 ); ///< Lift coefficient adjustment factor
            physics_Num m_cdFiddle =
                static_cast<physics_Num>( 0.0 ); ///< Drag coefficient adjustment factor
            physics_Num m_nitroFiddle =
                static_cast<physics_Num>( 0.0 ); ///< Nitro fuel adjustment factor

            physics_Num m_topDownTorque =
                static_cast<physics_Num>( 0.0 ); ///< Torque from rotor to engine (N*m)
            physics_Num m_bottomUpTorque =
                static_cast<physics_Num>( 0.0 ); ///< Torque from engine to rotor (N*m)
            physics_Num m_vbAilOut =
                static_cast<physics_Num>( 0.0 ); ///< VBar aileron output for debugging
            physics_Num m_vbEleOut =
                static_cast<physics_Num>( 0.0 ); ///< VBar elevator output for debugging

            // ===== Control Flags =====
            bool m_bodyForcesOn = false;    ///< Enable body drag forces
            bool m_rotorForcesOn = false;   ///< Enable rotor aerodynamic forces
            bool m_linkageControl = false;  ///< Enable linkage-based control (vs direct)
            bool m_modelIsElectric = false; ///< True if model uses electric power

            // ===== Communication and Callback =====
            TPassArray         m_va;         ///< Variable argument array for callbacks
            std::array<f32, 8> m_txChannel;  ///< Transmitter channel values [0-7]
            TProcedurePtr      m_fnCallback; ///< Function pointer for callbacks
            TPassAPtr          m_tp;         ///< Pointer for passing data arrays

            s32 m_cbFun = 0; ///< Callback function ID
            s32 m_cbSub = 0; ///< Callback sub-function ID

            // ===== Power System Components =====
            std::shared_ptr<CBatteryPack>  m_pack;  ///< Battery pack instance (electric power)
            std::shared_ptr<CEMotor>       m_motor; ///< Electric motor instance
            std::shared_ptr<CESController> m_esc;   ///< Electronic speed controller instance

            // ===== Control and Rotor Components =====
            std::shared_ptr<CFlybarlessUnit>   m_theVBar;         ///< Flybarless controller instance
            std::shared_ptr<EngineClutchUnit> m_theEngineClutch; ///< Engine and clutch system
            std::shared_ptr<CGovernorUnit>     m_theGovernor;     ///< RPM governor instance
            std::shared_ptr<TRotorHead>        m_rotorHead;       ///< Main rotor head assembly
            std::shared_ptr<TRotor>            m_tailRotor;       ///< Tail rotor assembly

            IAerodymanicsWind *m_windInterface = nullptr; ///< Wind system interface

            String  m_engineFileName; ///< Filename for engine power curve data
            String  m_pCurveFileName; ///< Filename for power curve lookup table
            StringW m_dataPath;       ///< Path to data files directory
        };

        extern std::shared_ptr<HeliAero> gHeliAero;

        inline std::shared_ptr<HeliAero> &HeliAero::getSingleton()
        {
            return gHeliAero;
        }
    } // namespace vehicle
} // namespace workphone

#endif //  FBHeliAero25H
