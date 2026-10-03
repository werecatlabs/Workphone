using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json;
using fb.json;


namespace fb
{
    namespace json
    {



        public class pos
        {
            public int current = 0;
            public int max = 0;
        }

        public class lap
        {
            public int current = 0;
            public int max = 0;
        }

        public class times
        {
            public string current = "";
            public string last = "";
            public string best = "";
            public string total = "";
        }

        public class race
        {
            public double startTime = 0.0;
            public json.times times = new json.times();
            public json.lap lap = new json.lap();
            public json.pos pos = new json.pos();
            public string countdownText = "";
        }

        public class game
        {
            public string type = "";
            public json.race race = new json.race();
        }

        [System.Serializable]
        public class players
        {
            public int count = 0;
            public int ready = 0;
        }

        [System.Serializable]
        public class spectators
        {
            public int count = 0;
        }

        [System.Serializable]
        public class user
        {
            public bool ready = false;
            public string type = "";
        }

        [System.Serializable]
        public class session
        {
            public json.players players = new json.players();
            public json.spectators spectators = new json.spectators();
            public json.user user = new json.user();

            public string id = "";
            public string type = "";
            public string name = "";
            public string creator = "";
            public string scenery = "";
            public string modelClass = "";
            public string modelType = "";
            public string status = "";

            public int numCurrentPlayers = 0;
            public int numMaxPlayers = 4;
            public int numCurrentSpectators = 0;
            public int numMaxSpectators = 0;
            public int latency = 0;
            public bool hasPassword = false;
            public bool canJoin = true;
            public bool isNight = false;
            public bool isRace = false;

            public int sceneryId = 0;
            public string sceneryResource = "";
        }


        public class multiplayer
        {
            public bool refreshSessionList = false;
            public List<json.session> session = new List<json.session>();
        }



        public struct dialog_data
        {
            public string factoryType;
            public string name;
            public string path;
            public string anchor_position;
            public float position_x;
            public float position_y;
            public float px_width;
            public float px_height;

            public int position_abs_x;
            public int position_abs_y;
            public int width_abs;
            public int height_abs;

            public int visible;
            public int offscreen;
            public int zorder;
        };



        public struct fsm_data
        {
            public string name;
            public string eventName;
            public int newState;
            public int previousState;
            public int currentState;
        };

        public struct attribute
        {
            public string name;
            public string value;
        };

        public class property
        {
            public string name;
            public string value;
            public List<json.attribute> attributes = new List<json.attribute>();
        };

        [System.Serializable]
        public class properties
        {
            public List<json.property> propertiesList = new List<json.property>();
        };

        [System.Serializable]
        public struct camera_data
        {
            public bool active;
            public float yFOV;
            public float nearDistance;
            public float farDistance;
        };

        [System.Serializable]
        public struct vec4
        {
            public vec4(float _x, float _y, float _z)
            {
                x = _x;
                y = _y;
                z = _z;
                w = 0.0f;
            }

            public vec4(float _x, float _y, float _z, float _w)
            {
                x = _x;
                y = _y;
                z = _z;
                w = _w;
            }

            public float x;
            public float y;
            public float z;
            public float w;

            static vec4 m_Zero = new vec4(0.0f, 0.0f, 0.0f, 0.0f);

            static public json.vec4 zero
            {
                get { return m_Zero; }
            }
        };

        [System.Serializable]
        public class transform_data
        {
            public vec4 position = new vec4();
            public vec4 orientation = new vec4(0.0f, 0.0f, 0.0f, 1.0f);
            public vec4 rotation = new vec4();
            public vec4 scale = new vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        [System.Serializable]
        public class rotor_data
        {
            public List<string> staticNodes = new List<string>();
            public List<string> dynamicNodes = new List<string>();
            public List<string> fadedStaticNodes = new List<string>();
            public List<string> fadedDynamicNodes = new List<string>();
            public List<string> fadedStaticEntities = new List<string>();
            public List<string> fadedDynamicEntities = new List<string>();
        }

        [System.Serializable]
        public class glow_rope_data
        {
            public List<string> materialNames = new List<string>();
            public List<string> currentMaterialNames = new List<string>();
            public List<string> originalMaterialNames = new List<string>();
        }

        public class pid_data
        {
            public float p;
            public float i;
            public float d;
            public float iLim;
            public float iAccumulator;
            public float lastErr;
            public float lastDer;
        }

        public class pid_scaler_data
        {
            public float p = 1.0f;
            public float i = 1.0f;
            public float d = 1.0f;
        }

        [System.Serializable]
        public class stab_settings_data
        {
            public enum InnerLoopOptions
            {
                STABILIZATIONSTATUS_INNERLOOP_DIRECT = 0,
                STABILIZATIONSTATUS_INNERLOOP_VIRTUALFLYBAR = 1,
                STABILIZATIONSTATUS_INNERLOOP_ACRO = 2,
                STABILIZATIONSTATUS_INNERLOOP_AXISLOCK = 3,
                STABILIZATIONSTATUS_INNERLOOP_RATE = 4,
                STABILIZATIONSTATUS_INNERLOOP_CRUISECONTROL = 5
            };

            public enum OuterLoopOptions
            {
                STABILIZATIONSTATUS_OUTERLOOP_DIRECT = 0,
                STABILIZATIONSTATUS_OUTERLOOP_ATTITUDE = 1,
                STABILIZATIONSTATUS_OUTERLOOP_RATTITUDE = 2,
                STABILIZATIONSTATUS_OUTERLOOP_WEAKLEVELING = 3,
                STABILIZATIONSTATUS_OUTERLOOP_ALTITUDE = 4,
                STABILIZATIONSTATUS_OUTERLOOP_ALTITUDEVARIO = 5
            };

            public stab_settings_data()
            {
                for (int i = 0; i < NUM_AXES; ++i)
                {
                    MaximumRate[i] = 0.0f;
                }

                MaxAxisLock = 0.0f;
                MaxAxisLockRate = 0.0f;
                AxisLockKp = 0.0f;

                VbarMaxAngle = 0.0f;
                VbarGyroSuppress = 0.0f;
                VbarRollPI_Kp = 0.0f;
                VbarRollPI_Ki = 0.0f;
                VbarPitchPI_Kp = 0.0f;
                VbarPitchPI_Ki = 0.0f;
                VbarYawPI_Kp = 0.0f;
                VbarYawPI_Ki = 0.0f;
                VbarDecay = 0.0f;

                for (int i = 0; i < NUM_AXES; ++i)
                {
                    VbarSensitivity[i] = 0.0f;
                }

                VbarPiroComp = 0;


                RollMax = 0.0f;
                PitchMax = 0.0f;
                YawMax = 0.0f;
                RattitudeTransition = 0.0f;
                WeakLevelingKp = 0.0f;
                MaxWeakLevelingRate = 0.0f;

                for (int i = 0; i < NUM_AXES; ++i)
                {
                    ManualRate[i] = 0.0f;
                }

                for (int i = 0; i < NUM_AXES; ++i)
                {
                    innerPids[i] = new pid_data();
                }

                for (int i = 0; i < NUM_AXES; ++i)
                {
                    outerPids[i] = new pid_data();
                }

                for (int i = 0; i < NUM_AXES; ++i)
                {
                    InnerLoopEnabled[i] = (int)InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_DIRECT;
                }

                for (int i = 0; i < NUM_AXES; ++i)
                {
                    OuterLoopEnabled[i] = (int)OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
                }

                for (int i = 0; i < NUM_AXES; ++i)
                {
                    Expo[i] = 60.0f;
                }

                Expo[0] = 100.0f;
                Expo[1] = 100.0f;
                Expo[2] = 100.0f;
                Expo[3] = 0.0f;
            }

            //-----------------------------------------------------------------------
            public string GetValue(string name)
            {
                if (name == "Expo/Pitch")
                    return Expo[0].ToString();
                if (name == "Expo/Roll")
                    return Expo[1].ToString();
                if (name == "Expo/Yaw")
                    return Expo[2].ToString();
                if (name == "Expo/Throttle")
                    return Expo[3].ToString();
                if (name == "MaximumRate[0]" || name == "Rates/Pitch")
                    return MaximumRate[0].ToString();
                if (name == "MaximumRate[1]" || name == "Rates/Roll")
                    return MaximumRate[1].ToString();
                if (name == "MaximumRate[2]" || name == "Rates/Yaw")
                    return MaximumRate[2].ToString();
                if (name == "MaximumRate[3]")
                    return MaximumRate[3].ToString();
                if (name == "MaxAxisLock")
                    return MaxAxisLock.ToString();
                if (name == "MaxAxisLockRate")
                    return MaxAxisLockRate.ToString();
                if (name == "AxisLockKp")
                    return AxisLockKp.ToString();
                if (name == "VbarRollPI_Kp")
                    return VbarRollPI_Kp.ToString();
                if (name == "VbarRollPI_Ki")
                    return VbarRollPI_Ki.ToString();
                if (name == "VbarPitchPI_Kp")
                    return VbarPitchPI_Kp.ToString();
                if (name == "VbarPitchPI_Ki")
                    return VbarPitchPI_Ki.ToString();
                if (name == "VbarYawPI_Kp")
                    return VbarYawPI_Kp.ToString();
                if (name == "VbarYawPI_Ki")
                    return VbarYawPI_Ki.ToString();
                if (name == "VbarSensitivity[0]")
                    return VbarSensitivity[0].ToString();
                if (name == "VbarSensitivity[1]")
                    return VbarSensitivity[1].ToString();
                if (name == "VbarSensitivity[2]")
                    return VbarSensitivity[2].ToString();
                if (name == "vbar_decay")
                    return VbarDecay.ToString();
                if (name == "VbarMaxAngle")
                    return VbarMaxAngle.ToString();
                if (name == "VbarGyroSuppress")
                    return VbarGyroSuppress.ToString();
                if (name == "VbarPiroComp")
                    return VbarPiroComp.ToString();
                if (name == "RollMax")
                    return RollMax.ToString();
                if (name == "PitchMax")
                    return PitchMax.ToString();
                if (name == "YawMax")
                    return YawMax.ToString();
                if (name == "rattitude_transition")
                    return RattitudeTransition.ToString();
                if (name == "WeakLevelingKp")
                    return WeakLevelingKp.ToString();
                if (name == "MaxWeakLevelingRate")
                    return MaxWeakLevelingRate.ToString();
                if (name == "ManualRate[0]")
                    return ManualRate[0].ToString();
                if (name == "ManualRate[1]")
                    return ManualRate[1].ToString();
                if (name == "ManualRate[2]")
                    return ManualRate[2].ToString();
                if (name == "ManualRate[3]")
                    return ManualRate[3].ToString();
                if (name == "innerPids[0].p" || name == "Pid/Pitch/P")
                    return innerPids[0].p.ToString();
                if (name == "innerPids[0].i" || name == "Pid/Pitch/I")
                    return innerPids[0].i.ToString();
                if (name == "innerPids[0].d" || name == "Pid/Pitch/D")
                    return innerPids[0].d.ToString();
                if (name == "innerPids[0].iLim")
                    return innerPids[0].iLim.ToString();
                if (name == "innerPids[0].iAccumulator")
                    return innerPids[0].iAccumulator.ToString();
                if (name == "innerPids[0].lastErr")
                    return innerPids[0].lastErr.ToString();
                if (name == "innerPids[0].lastDer")
                    return innerPids[0].lastDer.ToString();
                if (name == "outerPids[0].p")
                    return outerPids[0].p.ToString();
                if (name == "outerPids[0].i")
                    return outerPids[0].i.ToString();
                if (name == "outerPids[0].d")
                    return outerPids[0].d.ToString();
                if (name == "outerPids[0].iAccumulator")
                    return outerPids[0].iAccumulator.ToString();
                if (name == "outerPids[0].iLim")
                    return outerPids[0].iLim.ToString();
                if (name == "outerPids[0].lastDer")
                    return outerPids[0].lastDer.ToString();
                if (name == "outerPids[0].lastErr")
                    return outerPids[0].lastErr.ToString();
                if (name == "innerPids[1].p" || name == "Pid/Roll/P")
                    return innerPids[1].p.ToString();
                if (name == "innerPids[1].i" || name == "Pid/Roll/I")
                    return innerPids[1].i.ToString();
                if (name == "innerPids[1].d" || name == "Pid/Roll/D")
                    return innerPids[1].d.ToString();
                if (name == "innerPids[1].iLim")
                    return innerPids[1].iLim.ToString();
                if (name == "innerPids[1].iAccumulator")
                    return innerPids[1].iAccumulator.ToString();
                if (name == "innerPids[1].lastErr")
                    return innerPids[1].lastErr.ToString();
                if (name == "innerPids[1].lastDer")
                    return innerPids[1].lastDer.ToString();
                if (name == "outerPids[1].p")
                    return outerPids[1].p.ToString();
                if (name == "outerPids[1].i")
                    return outerPids[1].i.ToString();
                if (name == "outerPids[1].d")
                    return outerPids[1].d.ToString();
                if (name == "outerPids[1].iAccumulator")
                    return outerPids[1].iAccumulator.ToString();
                if (name == "outerPids[1].iLim")
                    return outerPids[1].iLim.ToString();
                if (name == "outerPids[1].lastDer")
                    return outerPids[1].lastDer.ToString();
                if (name == "outerPids[1].lastErr")
                    return outerPids[1].lastErr.ToString();
                if (name == "innerPids[2].p" || name == "Pid/Yaw/P")
                    return innerPids[2].p.ToString();
                if (name == "innerPids[2].i" || name == "Pid/Yaw/I")
                    return innerPids[2].i.ToString();
                if (name == "innerPids[2].d" || name == "Pid/Yaw/D")
                    return innerPids[2].d.ToString();
                if (name == "innerPids[2].iLim")
                    return innerPids[2].iLim.ToString();
                if (name == "innerPids[2].iAccumulator")
                    return innerPids[2].iAccumulator.ToString();
                if (name == "innerPids[2].lastErr")
                    return innerPids[2].lastErr.ToString();
                if (name == "innerPids[2].lastDer")
                    return innerPids[2].lastDer.ToString();
                if (name == "outerPids[2].p")
                    return outerPids[2].p.ToString();
                if (name == "outerPids[2].i")
                    return outerPids[2].i.ToString();
                if (name == "outerPids[2].d")
                    return outerPids[2].d.ToString();
                if (name == "outerPids[2].iAccumulator")
                    return outerPids[2].iAccumulator.ToString();
                if (name == "outerPids[2].iLim")
                    return outerPids[2].iLim.ToString();
                if (name == "outerPids[2].lastDer")
                    return outerPids[2].lastDer.ToString();
                if (name == "outerPids[2].lastErr")
                    return outerPids[2].lastErr.ToString();
                if (name == "InnerLoopEnabled[0]")
                    return InnerLoopEnabled[0].ToString();
                if (name == "InnerLoopEnabled[1]")
                    return InnerLoopEnabled[1].ToString();
                if (name == "InnerLoopEnabled[2]")
                    return InnerLoopEnabled[2].ToString();
                if (name == "InnerLoopEnabled[3]")
                    return InnerLoopEnabled[3].ToString();
                if (name == "OuterLoopEnabled[0]")
                    return OuterLoopEnabled[0].ToString();
                if (name == "OuterLoopEnabled[1]")
                    return OuterLoopEnabled[1].ToString();
                if (name == "OuterLoopEnabled[2]")
                    return OuterLoopEnabled[2].ToString();
                if (name == "OuterLoopEnabled[3]")
                    return OuterLoopEnabled[3].ToString();

                return "";
            }

            //-----------------------------------------------------------------------
            public void SetValue(string name, string value)
            {
                if (name == "Expo/Pitch")
                    Expo[0] = float.Parse(value);
                if (name == "Expo/Roll")
                    Expo[1] = float.Parse(value);
                if (name == "Expo/Yaw")
                    Expo[2] = float.Parse(value);
                if (name == "Expo/Throttle")
                    Expo[3] = float.Parse(value);
                if (name == "MaximumRate[0]" || name == "Rates/Pitch")
                    MaximumRate[0] = float.Parse(value);
                if (name == "MaximumRate[1]" || name == "Rates/Roll")
                    MaximumRate[1] = float.Parse(value);
                if (name == "MaximumRate[2]" || name == "Rates/Yaw")
                    MaximumRate[2] = float.Parse(value);
                if (name == "MaximumRate[3]")
                    MaximumRate[3] = float.Parse(value);
                if (name == "MaxAxisLock")
                    MaxAxisLock = float.Parse(value);
                if (name == "MaxAxisLockRate")
                    MaxAxisLockRate = float.Parse(value);
                if (name == "AxisLockKp")
                    AxisLockKp = float.Parse(value);
                if (name == "VbarRollPI_Kp")
                    VbarRollPI_Kp = float.Parse(value);
                if (name == "VbarRollPI_Ki")
                    VbarRollPI_Ki = float.Parse(value);
                if (name == "VbarPitchPI_Kp")
                    VbarPitchPI_Kp = float.Parse(value);
                if (name == "VbarPitchPI_Ki")
                    VbarPitchPI_Ki = float.Parse(value);
                if (name == "VbarYawPI_Kp")
                    VbarYawPI_Kp = float.Parse(value);
                if (name == "VbarYawPI_Ki")
                    VbarYawPI_Ki = float.Parse(value);
                if (name == "VbarSensitivity[0]")
                    VbarSensitivity[0] = float.Parse(value);
                if (name == "VbarSensitivity[1]")
                    VbarSensitivity[1] = float.Parse(value);
                if (name == "VbarSensitivity[2]")
                    VbarSensitivity[2] = float.Parse(value);
                if (name == "vbar_decay")
                    VbarDecay = float.Parse(value);
                if (name == "VbarMaxAngle")
                    VbarMaxAngle = float.Parse(value);
                if (name == "VbarGyroSuppress")
                    VbarGyroSuppress = float.Parse(value);
                if (name == "VbarPiroComp")
                    VbarPiroComp = int.Parse(value);
                if (name == "RollMax" || name == "Rates/Roll")
                    RollMax = float.Parse(value);
                if (name == "PitchMax" || name == "Rates/Pitch")
                    PitchMax = float.Parse(value);
                if (name == "YawMax" || name == "Rates/Yaw")
                    YawMax = float.Parse(value);
                if (name == "rattitude_transition")
                    RattitudeTransition = float.Parse(value) / 100.0f;
                if (name == "WeakLevelingKp")
                    WeakLevelingKp = float.Parse(value);
                if (name == "MaxWeakLevelingRate")
                    MaxWeakLevelingRate = float.Parse(value);
                if (name == "ManualRate[0]")
                    ManualRate[0] = float.Parse(value);
                if (name == "ManualRate[1]")
                    ManualRate[1] = float.Parse(value);
                if (name == "ManualRate[2]")
                    ManualRate[2] = float.Parse(value);
                if (name == "ManualRate[3]")
                    ManualRate[3] = float.Parse(value);
                if (name == "innerPids[0].p" || name == "Pid/Pitch/P")
                    innerPids[0].p = float.Parse(value);
                if (name == "innerPids[0].i" || name == "Pid/Pitch/I")
                    innerPids[0].i = float.Parse(value);
                if (name == "innerPids[0].d" || name == "Pid/Pitch/D")
                    innerPids[0].d = float.Parse(value);
                if (name == "innerPids[0].iLim")
                    innerPids[0].iLim = float.Parse(value);
                if (name == "innerPids[0].iAccumulator")
                    innerPids[0].iAccumulator = float.Parse(value);
                if (name == "innerPids[0].lastErr")
                    innerPids[0].lastErr = float.Parse(value);
                if (name == "innerPids[0].lastDer")
                    innerPids[0].lastDer = float.Parse(value);
                if (name == "outerPids[0].p")
                    outerPids[0].p = float.Parse(value);
                if (name == "outerPids[0].i")
                    outerPids[0].i = float.Parse(value);
                if (name == "outerPids[0].d")
                    outerPids[0].d = float.Parse(value);
                if (name == "outerPids[0].iAccumulator")
                    outerPids[0].iAccumulator = float.Parse(value);
                if (name == "outerPids[0].iLim")
                    outerPids[0].iLim = float.Parse(value);
                if (name == "outerPids[0].lastDer")
                    outerPids[0].lastDer = float.Parse(value);
                if (name == "outerPids[0].lastErr")
                    outerPids[0].lastErr = float.Parse(value);
                if (name == "innerPids[1].p" || name == "Pid/Roll/P")
                    innerPids[1].p = float.Parse(value);
                if (name == "innerPids[1].i" || name == "Pid/Roll/I")
                    innerPids[1].i = float.Parse(value);
                if (name == "innerPids[1].d" || name == "Pid/Roll/D")
                    innerPids[1].d = float.Parse(value);
                if (name == "innerPids[1].iLim")
                    innerPids[1].iLim = float.Parse(value);
                if (name == "innerPids[1].iAccumulator")
                    innerPids[1].iAccumulator = float.Parse(value);
                if (name == "innerPids[1].lastErr")
                    innerPids[1].lastErr = float.Parse(value);
                if (name == "innerPids[1].lastDer")
                    innerPids[1].lastDer = float.Parse(value);
                if (name == "outerPids[1].p")
                    outerPids[1].p = float.Parse(value);
                if (name == "outerPids[1].i")
                    outerPids[1].i = float.Parse(value);
                if (name == "outerPids[1].d")
                    outerPids[1].d = float.Parse(value);
                if (name == "outerPids[1].iAccumulator")
                    outerPids[1].iAccumulator = float.Parse(value);
                if (name == "outerPids[1].iLim")
                    outerPids[1].iLim = float.Parse(value);
                if (name == "outerPids[1].lastDer")
                    outerPids[1].lastDer = float.Parse(value);
                if (name == "outerPids[1].lastErr")
                    outerPids[1].lastErr = float.Parse(value);
                if (name == "innerPids[2].p" || name == "Pid/Yaw/P")
                    innerPids[2].p = float.Parse(value);
                if (name == "innerPids[2].i" || name == "Pid/Yaw/D")
                    innerPids[2].i = float.Parse(value);
                if (name == "innerPids[2].d" || name == "Pid/Yaw/I")
                    innerPids[2].d = float.Parse(value);
                if (name == "innerPids[2].iLim")
                    innerPids[2].iLim = float.Parse(value);
                if (name == "innerPids[2].iAccumulator")
                    innerPids[2].iAccumulator = float.Parse(value);
                if (name == "innerPids[2].lastErr")
                    innerPids[2].lastErr = float.Parse(value);
                if (name == "innerPids[2].lastDer")
                    innerPids[2].lastDer = float.Parse(value);
                if (name == "outerPids[2].p")
                    outerPids[2].p = float.Parse(value);
                if (name == "outerPids[2].i")
                    outerPids[2].i = float.Parse(value);
                if (name == "outerPids[2].d")
                    outerPids[2].d = float.Parse(value);
                if (name == "outerPids[2].iAccumulator")
                    outerPids[2].iAccumulator = float.Parse(value);
                if (name == "outerPids[2].iLim")
                    outerPids[2].iLim = float.Parse(value);
                if (name == "outerPids[2].lastDer")
                    outerPids[2].lastDer = float.Parse(value);
                if (name == "outerPids[2].lastErr")
                    outerPids[2].lastErr = float.Parse(value);
                if (name == "InnerLoopEnabled[0]")
                    InnerLoopEnabled[0] = (int)getInnerOption(value);
                if (name == "InnerLoopEnabled[1]")
                    InnerLoopEnabled[1] = (int)getInnerOption(value);
                if (name == "InnerLoopEnabled[2]")
                    InnerLoopEnabled[2] = (int)getInnerOption(value);
                if (name == "InnerLoopEnabled[3]")
                    InnerLoopEnabled[3] = (int)getInnerOption(value);
                if (name == "OuterLoopEnabled[0]")
                    OuterLoopEnabled[0] = (int)getOuterLoopOption(value);
                if (name == "OuterLoopEnabled[1]")
                    OuterLoopEnabled[1] = (int)getOuterLoopOption(value);
                if (name == "OuterLoopEnabled[2]")
                    OuterLoopEnabled[2] = (int)getOuterLoopOption(value);
                if (name == "OuterLoopEnabled[3]")
                    OuterLoopEnabled[3] = (int)getOuterLoopOption(value);
            }

            //-----------------------------------------------------------------------
            public InnerLoopOptions getInnerOption(string value)
            {
                InnerLoopOptions result = InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_DIRECT;

                if (value == "DIRECT")
                    result = InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_DIRECT;
                if (value == "FLYBAR")
                    result = InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_VIRTUALFLYBAR;
                if (value == "ACRO")
                    result = InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_ACRO;
                if (value == "AXISLOCK")
                    result = InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_AXISLOCK;
                if (value == "RATE")
                    result = InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_RATE;
                if (value == "CRUISECONTROL")
                    result = InnerLoopOptions.STABILIZATIONSTATUS_INNERLOOP_CRUISECONTROL;

                return result;
            }

            //-----------------------------------------------------------------------
            public OuterLoopOptions getOuterLoopOption(string value)
            {
                OuterLoopOptions result = OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_DIRECT;

                if (value == "DIRECT")
                    result = OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_DIRECT;
                if (value == "ATTITUDE")
                    result = OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_ATTITUDE;
                if (value == "RATTITUDE")
                    result = OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_RATTITUDE;
                if (value == "AXISLOCK")
                    result = OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_WEAKLEVELING;
                if (value == "RATE")
                    result = OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_ALTITUDE;
                if (value == "ALTITUDEVARIO")
                    result = OuterLoopOptions.STABILIZATIONSTATUS_OUTERLOOP_ALTITUDEVARIO;

                return result;
            }

            public int OptionId = 0;

            public float MaxAxisLock = 0.0f;
            public float MaxAxisLockRate = 0.0f;
            public float AxisLockKp = 0.0f;
            public float VbarMaxAngle = 0.0f;
            public float VbarGyroSuppress = 0.0f;
            public float VbarRollPI_Kp = 0.0f;
            public float VbarRollPI_Ki = 0.0f;
            public float VbarPitchPI_Kp = 0.0f;
            public float VbarPitchPI_Ki = 0.0f;
            public float VbarYawPI_Kp = 0.0f;
            public float VbarYawPI_Ki = 0.0f;
            public float VbarDecay = 0.0f;
            public float RollMax = 0.0f;
            public float PitchMax = 0.0f;
            public float YawMax = 0.0f;
            public float RattitudeTransition = 0.0f;
            public float WeakLevelingKp = 0.0f;
            public float MaxWeakLevelingRate = 0.0f;

            public int VbarPiroComp = 0;

            const int NUM_AXES = 4;

            public List<float> ManualRate = new List<float>(new float[NUM_AXES]);
            public List<pid_data> innerPids = new List<pid_data>(new pid_data[NUM_AXES]);
            public List<pid_data> outerPids = new List<pid_data>(new pid_data[NUM_AXES]);
            public List<float> Expo = new List<float>(new float[NUM_AXES]);
            public List<int> InnerLoopEnabled = new List<int>(new int[NUM_AXES]);
            public List<int> OuterLoopEnabled = new List<int>(new int[NUM_AXES]);
            public List<float> MaximumRate = new List<float>(new float[NUM_AXES]);
            public List<float> VbarSensitivity = new List<float>(new float[NUM_AXES]);
        }


        [System.Serializable]
        public class drone_stab_settings
        {
            public int currentOptionId = 0;
            public List<stab_settings_data> settingsData = new List<stab_settings_data>();
        }


        [System.Serializable]
        public class multi_rotor_prop_data
        {
            public transform_data localTransform = new transform_data();
            public transform_data worldTransform = new transform_data();
            public float diameter = 6.0f;
            public float pitch = 4.0f;
        }


        [System.Serializable]
        public class multi_rotor_motor_data
        {
            public transform_data localTransform = new transform_data();
            public transform_data worldTransform = new transform_data();
            public float maxTorque = 1.0f;
            public float maxRPM = 15000.0f;
            public float slewRateUp = 30.0f;
            public float slewRateDown = 10.0f;
        }


        [System.Serializable]
        public class multi_rotor_ctrl_data
        {
            public stab_settings_data stabSettingsData = new stab_settings_data();
            public List<multi_rotor_prop_data> propData = new List<multi_rotor_prop_data>();
            public List<multi_rotor_motor_data> motorData = new List<multi_rotor_motor_data>();
            public List<collider> colliders = new List<collider>();
            public float maxRPM = 15000.0f;
            public float maxTorque = 1.0f;
            public float angularDamping = 0.0f;
            public float propPitch = 0.0f;
            public float propDiameter = 0.0f;
            public float voMultiplier = 0.0f;
            public float groundEffectMultiplier = 0.0f;
            public float drag = 0.0f;
            public float moiX = 0.0f;
            public float moiY = 0.0f;
            public float moiZ = 0.0f;
            public float maxThrust = 0.0f;
            public float thrustMultiplier = 0.0f;
            public float mass = 0.0f;
            public int type = 0;
        }

        [System.Serializable]
        public class four_wheeled_vehicle_wheel_data
        {
            public transform_data localTransform;
            public transform_data worldTransform;
            public string wheelName = "";
        }

        [System.Serializable]
        public class four_wheeled_vehicle_data
        {
            public float angularDamping = 0.0f;
            public float linearDamping = 0.0f;
            public float mass = 1.0f;
            public float wheelMass = 1.0f;
            public vec4 chassisDims = new vec4();
            public vec4 moi = new vec4();
            public transform_data cogLocalTransform;
            public transform_data cogWorldTransform;
            public float centreBias = 0.0f;
            public float frontBias = 0.0f;
            public float rearBias = 0.0f;
            public float frontRearSplit = 0.0f;
            public bool isNitro = false;
            public List<four_wheeled_vehicle_wheel_data> wheels = new List<four_wheeled_vehicle_wheel_data>();
        }

        public class water_ripples
        {
            public int displacementTexture = 0;
            public int displacementWidth = 0;
            public int displacementHeight = 0;
            public vec4 amplitude = new vec4();
            public vec4 frequency = new vec4();
            public vec4 steepness = new vec4();
            public vec4 speed = new vec4();
            public vec4 directionAB = new vec4();
            public vec4 directionCD = new vec4();
            public List<vec4> displacementTextureData = new List<vec4>();
        }


        [System.Serializable]
        public class component_data
        {
            public int hashName = 0;

            public string ident = "";
            public string objectReference = "";
            public string componentName = "";
            public string componentResource = "";
            public string prefab = "";
            public string parentName = "";
            public string sceneNodeName = "";
            public string configType = "";

            public transform_data localTransform;
            public transform_data worldTransform;

            public vec4 vec1;
            public vec4 vec2;
            public vec4 vec3;

            public float rpm = 0.0f;
            public int isControl = 0;
            public int isVisible = 0;

            public int viewId = 0;
            public int playerId = 0;

            public int rowId = 0;
            public int objectId = 0;

            public string componentType = "";
            public string particleSystemName = "";

            public json.properties properties;

            public rotor_data rotorData = new rotor_data();
            public glow_rope_data glowRopeData = new glow_rope_data();
            public multi_rotor_ctrl_data multiRotorCtrlData = new multi_rotor_ctrl_data();
            public four_wheeled_vehicle_data fourWheeledVehicleData = new four_wheeled_vehicle_data();
        };



        public class model_buoyancy_data
        {
            public int id = 0;
            public float density = 700;
            public int slicesPerAxis = 2;
            public bool isConcave = false;
            public int voxelsLimit = 16;
            public float waveVelocity = 0.05f;
        }


        [System.Serializable]
        public class model_data
        {
            public string name = "";
            public string resource = "";
            public int modelId = 0;
            public int modelReferenceId = 0;
            public string prefab = "";

            public int viewId = 0;
            public int playerId = 0;

            public vec4 position = new vec4();
            public vec4 orientation = new vec4();
            public vec4 scale = new vec4();

            public vec4 startPosition = new vec4();
            public vec4 startOrientation = new vec4();

            public int isVisible = 1;
            public List<json.component_data> components = new List<json.component_data>();
            public model_buoyancy_data modelBouyancyData = new model_buoyancy_data();
        };


        public struct model_load_data
        {
            public string name;
            public string resource;
            public int modelId;
            public int modelReferenceId;
            public string prefab;
        };


        public struct plugin_event
        {
            public string type;
            public string arg1;
            public string arg2;
            public string arg3;
            public string arg4;
        }


        public class plugin_events
        {
            public List<plugin_event> events = new List<plugin_event>();
        }



        public class scenery_data
        {
            public int id = 0;
            public string name = "";
            public string resource = "";
            public string sceneName = "";
            public string coneLayout = "";
            public bool isNight = false;
        }

        public class render_system_config
        {
            public vec4 resolution;
            public int fsaa;
            public bool vsync;
            public bool fullscreen;
        };


        public class chat_data
        {
            public string user_id = "";
            public string user_name = "";
            public string message = "";
            public bool is_host = false;
        };

        public class model_wind
        {
            public float speed;
            public float direction;
            public float turbulence;
            public float groundHeight;
            public float directionOffset;
            public float fieldRoughness;
            public float temperature;
            public float pressure;
            public float smallTurbulence;
        }

        public class collider_data
        {
            public string name = "";
            public string type = "";
            public string meshName = "";
            public vec4 size = new vec4();
            public vec4 minExtents = new vec4();
            public vec4 maxExtents = new vec4();
        }

        public class gameobject_component_data
        {
            public string name = "";
            public string type = "";
        }

        public class prefab_data
        {
            public string name = "";
            public List<json.collider_data> colliders = new List<json.collider_data>();
            public List<json.gameobject_component_data> components = new List<json.gameobject_component_data>();
            public List<json.prefab_data> children = new List<json.prefab_data>();
        }

        public class tree_prototype_data
        {
            public prefab_data prefabData = new prefab_data();
        }

        public class tree_instance_data
        {
            public vec4 position = new vec4();
            public vec4 orientation = new vec4();
            public vec4 scale = new vec4();
            public vec4 color = new vec4();
            public vec4 lightmapColor = new vec4();
            public float heightScale = 0.0f;
            public float rotation = 0.0f;
            public float widthScale = 0.0f;
            public int prototypeIndex = 0;
        }

        public class terrain_data
        {
            public List<json.tree_prototype_data> treePrototypeData = new List<json.tree_prototype_data>();
            public List<json.tree_instance_data> treeInstanceData = new List<json.tree_instance_data>();
        }

        public class resolution_entry
        {
            public string label = "1280 x 720";
            public int width = 1280;
            public int height = 720;
            public int option = 0;
        }

        public class fsaa_entry
        {
            public fsaa_entry() { }
            public fsaa_entry(string label_, int value_)
            {
                label = label_;
                value = value_;
            }

            public string label = "";
            public int value = 0;
        }

        public class resolution_data
        {
            public List<json.resolution_entry> resolutions = new List<json.resolution_entry>();
            public List<json.fsaa_entry> availableFSAA = new List<json.fsaa_entry>();
        }



        public class material
        {
            public string name;
            public string mainTexture;
            public string normalTexture;
            public string shader;

            public vec4 tiling;
            public vec4 offset;
            public vec4 tiling1;
            public vec4 offset1;

            public float shininess = 0.0f;
            public float smoothness = 0.5f;
            public int uvSet = 0;

            public List<json.property> properties = new List<json.property>();
        }

        public class material_data
        {
            public List<json.material> materials = new List<json.material>();
        }

        public class field
        {
            public string name;
            public string value;
        }

        public class rows
        {
            public List<json.field> fieldPairs = new List<json.field>();
        }

        public class query
        {
            public List<json.rows> rows = new List<json.rows>();
        }

        public class object_attributes
        {
            public int id;
            public int objectId;
            public query attributesQuery;
        }



        public class configured_model_component_data
        {
            public int id = 0;
            public int referenceId = 0;
            public int parentReferenceId = 0;
            public int hashName = 0;
            public string label = "";
            public string image = "";
            public string resource = "";
            public string className = "";
            public string configType = "";
            public string objectName = "";
            public int componentType = 0;
            public int channel = -1;
        }



        public class configured_model_data
        {
            public int id = 0;
            public int parentId = 0;

            public int refComponentId = 0;
            public int scale = 0;
            public int nitro = 0;
            public int standard = 0;
            public int refModelRowId = 0;

            public string configType = "";
            public string name = "";
            public string resourceName = "";
            public string modelImage = "";

            public json.query componentQuery;
            public List<json.query> componentQueries = new List<json.query>();
            public List<json.object_attributes> objectAttributes = new List<json.object_attributes>();
            public List<json.configured_model_component_data> configuredComponents = new List<json.configured_model_component_data>();
        }



        public class configured_actors_data
        {
            public List<json.configured_model_data> configuredModelData = new List<json.configured_model_data>();
        }



        public class debug_text_data
        {
            public int id;
            public float x;
            public float y;
            public string text;
            public string eventName;
            public bool visible;
        }


        public class dialog_manager_state
        {
            public bool fullScreenDialogDisplayed;
        }

        public class resource_map_entry
        {
            public int id = 0;
            public string resourceType = "";
            public string name = "";
            public string prefab = "";
        }

        public class resource_map
        {
            public List<resource_map_entry> resources = new List<resource_map_entry>();
        }

        public class layer_data
        {
            public string name = "";
            public int value = -1;
        }

        public class sphere_collider
        {
            public vec4 center = new vec4();
            public float radius = 0.0f;
        }

        public class plane_collider
        {
            public vec4 normal = new vec4();
            public float distance = 0.0f;
        }

        public class box_collider
        {
            public vec4 center = new vec4();
            public vec4 size = new vec4();
        }

        public class sub_mesh_collider
        {
            public List<json.vec4> vertices = new List<json.vec4>();
            public List<int> indices = new List<int>();
            public bool sharedVertices = false;
        }

        public class mesh_collider
        {
            public string filePath = "";
            public bool isConvex = false;
            public List<json.vec4> vertices = new List<json.vec4>();
            public List<int> indices = new List<int>();
            public List<sub_mesh_collider> subMeshes = new List<sub_mesh_collider>();
        }

        public class compond_collider
        {
            public List<collider> colliders = new List<collider>();
        }

        public class concave_collider
        {
            public List<mesh_collider> convexMeshes = new List<mesh_collider>();
        }

        public class terrain_collider
        {
            public string filePath = "";
            public vec4 heightScale = new vec4();
            public vec4 terrainSize = new vec4();
            public vec4 size = new vec4();
            public List<float> heightData = new List<float>();
        }

        public class physics_material
        {
            public float staticFriction = 1.0f;
            public float dynamicFriction = 1.0f;
            public float restitution = 0.1f;
            public float roughness = 0.0f;
        }

        public class collider
        {
            public sphere_collider sphereCollider = new sphere_collider();
            public plane_collider planeCollider = new plane_collider();
            public box_collider boxCollider = new box_collider();
            public mesh_collider meshCollider = new mesh_collider();
            public terrain_collider terrainCollider = new terrain_collider();
            public compond_collider compondCollider = new compond_collider();
            public string colliderType = "";
            public transform_data transform = new transform_data();
            public transform_data localTransform = new transform_data();
            public transform_data worldTransform = new transform_data();
            public physics_material physicsMaterial = new physics_material();
            public bool isTrigger = false;
        }

        public class bounds_data
        {
            public vec4 minExtents = new vec4();
            public vec4 maxExtents = new vec4();
        }

        public class constraint_data
        {
            public int type = 0;

            public int motionAxisX = 0;
            public int motionAxisY = 0;
            public int motionAxisZ = 0;

            public int motionTwist = 0;
            public int motionSwing1 = 0;
            public int motionSwing2 = 0;

            public int actorA = 0;
            public int actorB = 0;

            public int rigidBodyA = 0;
            public int rigidBodyB = 0;

            public vec4 localPositionA = new vec4();
            public vec4 localPositionB = new vec4();

            public vec4 localOrientationA = new vec4(0.0f, 0.0f, 0.0f, 1.0f);
            public vec4 localOrientationB = new vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        public class rigid_body
        {
            public List<json.collider> colliders = new List<json.collider>();
            public int numSolverIterations = 10;
            public float mass = 1.0f;
            public float drag = 0.0f;
            public float angularDrag = 0.05f;
            public bool useGravity = true;
            public bool isKinematic = false;
            public bool isStatic = true;
            public bool autoWind = true;
            public bool autoBouyancy = true;
            public layer_data layer = new layer_data();
            public List<layer_data> collidableLayers = new List<layer_data>();
            public bounds_data bounds = new bounds_data();
        }

        public class model_layer_data
        {
            public layer_data layer = new layer_data();
            public List<layer_data> collidableLayers = new List<layer_data>();
        }

        public class buoyancy_data
        {
            public int actorId = 0;
            public float density = 700.0f;
            public int slicesPerAxis = 2;
            public bool isConcave = false;
            public int voxelsLimit = 16;
            public float waveVelocity = 0.05f;
        }


        public class actor_data
        {
            public int actorId = 0;
            public string actorName = "";
            public transform_data transformData = new transform_data();
        }

        public class component_event_data
        {
            public int componentId = 0;
            public string componentType = "";
            public buoyancy_data buoyancyData = new buoyancy_data();
            public rigid_body rigidBody = new rigid_body();
            public constraint_data constraintData = new constraint_data();
            public water_ripples water = new water_ripples();
        }

        public class actor_event_data
        {
            public string eventName = "";
            public actor_data actorData = new actor_data();
            public component_event_data componentData = new component_event_data();
            public component_data modelComponentData = new component_data();
            public List<string> strData = new List<string>();
        }



        public class debug_draw
        {
            public int id = 0;
            public vec4 start = new vec4();
            public vec4 end = new vec4();
        }


        public class object_select_data
        {
            public int selectedId = 0;
            public string objectType = "";
            public string objectName = "";
        }

        [System.Serializable]
        public class movable_object_data
        {
            public string name = "";
            public string type = "";
            public vec4 minExtent = new vec4();
            public vec4 maxExtent = new vec4();
            public vec4 extent = new vec4();
            public vec4 center = new vec4();
        }

        [System.Serializable]
        public class scene_node_data
        {
            public string name = "";
            public string componentType = "";
            public int hashId = 0;
            public bool isVisible = false;
            public bool isComponentVisible = false;
            public bool isInHierarchy = false;
            public transform_data localTransform = new transform_data();
            public transform_data worldTransform = new transform_data();
            public List<scene_node_data> children = new List<scene_node_data>();
            public List<movable_object_data> objects = new List<movable_object_data>();
        }



        public class rotor_network_data
        {
            public int hashName = 0;
            public float dynamicAlpha = 0.0f;
            public float mediumAlpha = 0.0f;
            public float staticAlpha = 0.0f;
            public float rotation = 0.0f;
            public float rpm = 0.0f;
        }

        [System.Serializable]
        public class path_data
        {
            public string installationPath = "";
            public string mydocumentsPath = "";
            public string appdataPath = "";
            public string flightDataPath = "";
        }

        [System.Serializable]
        public class model_physics_setup_flight_data
        {
            public int modelType = 0;

            public bool isNitro = false;
            public bool isFlybar = false;

            public string model = "Model";
            public string rotorHead = "RotorHead";
            public string headTeeter = "HeadTeeter";
            public string mainRotor = "MainRotor";
            public string tailRotor = "TailRotor";
            public string flybar = "Flybar";
            public string propRotor = "PropRotor";
            public string engine = "Engine";
            public string swashplate = "Swashplate";
            public string receiver = "accurc";
            public string flybarController = "flybarctrl";

            public float groundEffectMax = 1.2f;
            public float groundEffectDecay = 1.2f;

            public float calculationRateVal1 = 1.5f;
            public float calculationRateVal2 = 1.5f;
        }

        [System.Serializable]
        public class model_physics_setup_data
        {
            public string name = "";
            public model_physics_setup_flight_data flightData = new model_physics_setup_flight_data();
        }



        [System.Serializable]
        public class models_physics_setup_data
        {
            public model_physics_setup_data[] data;
        }



        [System.Serializable]
        public class option
        {
            public string value = "";
            public string text = "";
            public bool enabled = true;
        }



        [System.Serializable]
        public class setup_wizard_data
        {
            public string language = "";
            public string languageCode = "";
            public string flybarlessSetting = "";
            public string txMake = "";
            public string txMode = "";
            public string adapter = "";

            public option txMakeOption = new option();
            public option txModeOption = new option();
            public option adapterOption = new option();
        }



        [System.Serializable]
        public class system_settings_data
        {
            public resolution_data resolutionData = new resolution_data();
            public string renderQuality = "";
            public int screenWidth = 0;
            public int screenHeight = 0;
            public bool isFullscreen = false;
            public bool isStartWorkbench = true;
            public bool isRaceMode = false;
            public int startSceneId = -1;
            public models_physics_setup_data modelsPhysicsSetupData = new models_physics_setup_data();
            public setup_wizard_data setupWizardData = new setup_wizard_data();
        }


        public class switch_indicators_data
        {
            public int rpm1 = 0;
            public int rpm2 = 0;
            public int rpm3 = 0;

            public int tx = 0;
            public int adapter = 0;
            public int norm = 0;
            public int idle = 0;
            public int hold = 0;
            public int dualRate = 0;
            public int gearDown = 0;
            public int flapDown = 0;
            public int bail = 0;
            public int bailMode = 0;
            public float batteryState = 0;
            public float batteryVolts = 0;
        }


        public class model_component_setup_data
        {
            public string setupName = "";
            public string setupPath = "";
            public string setupResourcePath = "";
        }

        public class model_component_config_data
        {
            public string id = "";
            public model_component_setup_data componentSetupData = new model_component_setup_data();
        }

        public class model_setup_data
        {
            public int modelType = 0;
            public string modelName = "Model";
            public string[] tags;
            public bool isNitro = false;
            public bool hasCanopy = true;
            public bool hasBodyShell = true;
            public string resourceName = "";
            public string imageName = "";
            public int referenceId = 0;
            public int modelConfigId = 0;

            public List<model_component_config_data> componentConfigData = new List<model_component_config_data>();
        }

        [System.Serializable]
        public class hover_data
        {
            public string label = "";
            public vec4 gravity = vec4.zero;
            public vec4 rotationOffset = vec4.zero;
            public float throttleSpeedMultiplier = 0.01f;
            public float hoverForce = 1.0f;
            public float hoverHeight = 0.1f;
            public float brakeForceMultiplier = 0.01f;
            public float rotationSpeed = 0.0f;
            public float graivtyMultiplier = 0.0f;
            public float sideSlipCoefficient = 1.0f;
            public float linearDamping = 1.0f;
        }

        [System.Serializable]
        public class model_prefab_data
        {
            public string prefabName = "";
            public string prefabPath = "";
            public string prefabPathLOD = "";
            public string prefabPathAiLOD = "";
            public int modelType = 0;
            public bool isNitro = false;
            public int scale = 0;
        }



        [System.Serializable]
        public class race_view_data
        {
            public int id = 0;
            public int startRank = 0;
            public int rank = 0;
            public int lap = 0;
            public int currentIndex = 0;
            public float raceCompletion = 0.0f;
            public float progress = 0.0f;
            public float bestLapCounter = 0.0f;
        }


        [System.Serializable]
        public class race_manager_view_data
        {
            public int totalLaps = 3;
            public int totalRacers = 1;
            public List<race_view_data> raceViewData = new List<race_view_data>();
        }

        [System.Serializable]
        public class joystick_function_data
        {
            public string modelType = "";
            public string function = "";
            public int id = 0;
            public int channel = 0;
            public int button = 0;
            public bool reverse = false;
        }



        [System.Serializable]
        public class joystick_map_data
        {
            public List<joystick_function_data> functions = new List<joystick_function_data>();
        }



        [System.Serializable]
        public class flight_recording_file
        {
            public string pilotName = "";
            public string fileName = "";
            public string description = "";
        };



        [System.Serializable]
        public class flightfileinfo
        {
            public string filename = "";
            public string title = "";
            public string date = "";
            public string duration = "";
            public string description = "";
            public string sceneryName = "";
            public string modelName = "";
            public string pilotName = "";
            public double recordingFPS = 0.0;
            public double totalTime = 0.0;
            public string data = "";
            public int modelId = 0;
            public int refModelId = 0;
        };


        [System.Serializable]
        public class file_data
        {
            public string fileName = "";
            public string filePath = "";
        };




        [System.Serializable]
        public class directorylisting
        {
            public string folderName = "";
            public List<file_data> files = new List<file_data>();
            public List<directorylisting> folders = new List<directorylisting>();
        };

        [System.Serializable]
        public class s_custom
        {
            //public bool hideAllDialogs = false;
            //public bool showWorkbenchDialogs = false;
            //public bool showFlightModeDialogs = false;
            //public List<string> showDialogs = new List<string>();
            //public List<string> hideDialogs = new List<string>();
            //public string type = "";
            //public string value = "";
            //public string dataGroup = "";
            //public string dataType = "";
            //public component_data data = new component_data();
            //public flight_recording_file flight_recording_file = new flight_recording_file();
            //public string destination = "";
            //public string routeDialog = "";
            //public string sql = "";
            //public string state = "";
            //public string refModelID = "";
            //public string userModelName = "";
            //public bool changeTo = false;
            //public int num_rows = 0;
            //public rows rows = new rows();
            public List<flightfileinfo> flightfiles = new List<flightfileinfo>();
            public directorylisting directorylisting;
            //public dialogstate dialogstate;
            //public multiplayer multiplayer = new multiplayer();
            //public game game = new game();
        };



        [System.Serializable]
        public class s_action
        {
            public int caller = 0;
            public string action = "";
            public string tag = "";
            public string dialogName = "";
            public s_custom custom = new s_custom();
        };



        [System.Serializable]
        public class cg_info_mode
        {
            public float mass = 0.0f;
            public float ailLeft = 0.0f;
            public float ailRight = 0.0f;
            public float elevLeft = 0.0f;
            public float elevRight = 0.0f;
            public float rudder = 0.0f;
            public float flapLeft = 0.0f;
            public float flapRight = 0.0f;
            public List<float> data = new List<float>();
        };


        [System.Serializable]
        public class ui_visibility_state
        {
            public bool bHudVisible = false;
            public bool bVisualTxVisible = false;
        };


        public class car_differential
        {
            float frontLock = 0.2f;
            float rearLock = 0.2f;
        }


        [System.Serializable]
        public class car_setup
        {
            public car_differential differential = new car_differential();
        }

        [System.Serializable]
        public class channel_object
        {
            public enum ModelType
            {
                Heli,
                Plane,
                Drone,
                Car,
                Misc,
                Count
            }

            public void Update()
            {
                this.derived_channel_value = GetDerivedValue();
                this.totalDelta += GetDelta();
            }

            public float GetDelta()
            {
                return this.prev_channel_value - this.channel_value;
            }

            public float GetDerivedValue()
            {
                //if (Mathf.Abs(this.min_throw_multiplier) < 0.05f ||
                //    Mathf.Abs(this.max_throw_multiplier) < 0.05f)
                //{
                //    return this.channel_value;
                //}

                float val = this.channel_value;
                float low_m = 0.0f;
                float high_m = 0.0f;

                if (!this.reverse)
                {
                    low_m = this.min_throw_multiplier;
                    high_m = this.max_throw_multiplier;
                }
                else
                {
                    low_m = this.min_throw_multiplier;
                    high_m = this.max_throw_multiplier;
                }

                // if (!Mathf.Equals(low_m, high_m))
                {
                    if (low_m != 0.0f)
                    {
                        low_m = -0.8f / (low_m);
                    }
                    else
                    {
                        low_m = 1.0f;
                    }

                    if (high_m != 0.0f)
                    {
                        high_m = 0.8f / (high_m);
                    }
                    else
                    {
                        low_m = 1.0f;
                    }

                    if (val < 0.0f)
                    {
                        val = val * low_m;
                    }
                    else
                    {
                        val = val * high_m;
                    }

                    if (this.reverse)
                    {
                        val = -val;
                    }

                    return val;
                }

                return this.channel_value;
            }

            public void Reset()
            {
                this.cmap = -1;
                this.max_throw = float.MinValue;
                this.min_throw = float.MaxValue;
                this.totalDelta = 0.0f;
                //this.channel_value = 0.0f;
            }

            public void SetChannelValue(float chVal)
            {
                float diff = chVal - this.channel_value;
                //if (Mathf.Abs(diff) > 0.05f)
                {
                    this.prev_channel_value = this.channel_value;
                    this.channel_value = chVal;
                    this.hasChangedSinceInit = true;
                    //this.input_time = Time.time;
                }
            }

            public void ResetInputTime()
            {
                this.hasChangedSinceInit = true;
                //this.input_time = Time.time;
            }

            public float timeSinceInput
            {
                get { return 0.0f; } //{ return Time.time - input_time; }
            }

            public int id = 0;
            public int currentFunction = 0;
            public int channel_number = 0;
            public int cmap = 0;
            public int realValue = 0;
            public int progressValue = 0;
            public int displayValue = 0;
            public int output = 0;
            public float totalDelta = 0.0f;
            public float db_offset = 0.0f;
            public float newOffset = 0.0f;
            public float db_multiplier = 0.0f;
            public float max_throw = float.MinValue;
            public float min_throw = float.MaxValue;
            public float max_throw_multiplier = 0.0f;
            public float min_throw_multiplier = 0.0f;
            public float prev_channel_value = 0.0f;
            public float channel_value = 0.0f;
            public float derived_channel_value = 0.0f;
            public float input_time = 0.0f;
            public bool reverse = false;
            public string value = "";
            public string txMake = "";
            public string stickDirectionLabel = "";
            public string color = "";
            public string emulation = "";
            public int buttonValues = 0;
            public int prevButtonValues = 0;
            public int changedButtons = 0;
            public bool isButton = false;
            public bool hasChangedSinceInit = false;
            public channel_object[] subFunctions = new channel_object[0];
        }


        [System.Serializable]
        public class vehicle_attachment_data
        {
            public transform_data localTransform = new transform_data();
            //public transform_data worldTransform = new transform_data();
            public bool controllable = false;
            public string name = "";
        }



        [System.Serializable]
        public class aircraft_attachment_data : vehicle_attachment_data
        {
        }



        [System.Serializable]
        public class aircraft_engine_data : aircraft_attachment_data
        {
            public transform_data worldAnimatedPropellerPivot = new transform_data();
            public transform_data localAnimatedPropellerPivot = new transform_data();
            public vec4 animatedPropellerPivotRotateAxis = vec4.zero;
            public float rpmToUseFastProp = 0.0f;
            public float idleRPM = 0.0f;
            public float maxRPM = 0.0f;
            public float forceAtMaxRPM = 0.0f;
            public float rpmToAddPerKTOfSpeed = 0.0f;
            public float rpmLerpSpeed = 0.0f;
            public float pitchAtIdleRPM = 0.0f;
            public float pitchAtMaxRPM = 0.0f;
            public float currentRPM = 0.0f;
            public float desiredRPM = 0.0f;
            public float engineRunVolume = 0.0f;

            public float thrustMultiplier = 1.0f;
            public float torqueMultiplier = 0.0f;

            public int currentEngineState = 0;
            public vec4 thrust = vec4.zero;
            public float propwashSpeed = 0.0f;
        }

        [System.Serializable]
        public class aircraft_wing_data : aircraft_attachment_data
        {
            public string type = "";

            public int sectionCount = 10;
            public float wingTipWidthZeroToOne = 1.0f;
            public float wingTipSweep = 0.0f;
            public float wingTipAngle = 0.0f;
            public float cdOverride = 0.045f;
            public float wingArea = 0.0f;
            public float angleOfAttack = 0.0f;
            public float liftLineChordPosition = 0.75f;

            public float aoaMultiplier = 1.0f;
            public float cdMultiplier = 1.0f;
            public float clMultiplier = 1.0f;
            public float cmMultiplier = 1.0f;

            public float stallControlCL = 0.0f;
            public float stallControlCD = 0.0f;
            public float stallControlCM = 0.0f;

            public float stallThreshold = 0.0f;

            public bool isMirror = false;

            public vec4 size = vec4.zero;
            public vec4 wingBoxColliderMin = vec4.zero;
            public vec4 wingBoxColliderMax = vec4.zero;
            public vec4 wingRootLeadingEdge = vec4.zero;
            public vec4 wingRootTrailingEdge = vec4.zero;
            public vec4 wingTipLeadingEdge = vec4.zero;
            public vec4 wingTipTrailingEdge = vec4.zero;
            public vec4 rootLiftPosition = vec4.zero;
            public vec4 tipLiftPosition = vec4.zero;
            public vec4 subDivision = vec4.zero;

            public string aerofoilName = "";
            public string controlSurfaceName = "";
            public string propWashName = "";
            public string groundEffectName = "";
            public string mirrorObject = "";


        }


        [System.Serializable]
        public class aircraft_input_data
        {
            public string name = "";
            public bool invert = false;
            public string axisName = "";
            public string buttonName = "";
        }

        [System.Serializable]
        public class aircraft_control_surface_data : aircraft_attachment_data
        {
            public aircraft_input_data inputData = new aircraft_input_data();

            public float minDeflectionDegrees = 0.0f;
            public float maxDeflectionDegrees = 0.0f;

            public float rootHingeDistanceFromTrailingEdge = 0.0f;
            public float tipHingeDistanceFromTrailingEdge = 0.0f;
            public float currentDeflection = 0.0f;

            public float aoaMultiplier = 1.0f;
            public float clMultiplier = 0.0f;
            public float cdMultiplier = 0.0f;
            public float cmMultiplier = 0.0f;

            public float stallControlCL = 0.0f;
            public float stallControlCD = 0.0f;
            public float stallControlCM = 0.0f;
            public float stallThreshold = 0.0f;

            public List<float> clLookup = new List<float>();
            public List<float> cdLookup = new List<float>();
            public List<float> cmLookup = new List<float>();

            public List<int> affectedSections = new List<int>();

            public vec4 modelRotationAxis = vec4.zero;

            public int surfaceId = 0;
            public bool reverse = false;
        }

        [System.Serializable]
        public class aircraft_prop_wash_data : aircraft_attachment_data
        {
            public string propwashSource = "";
            public float strength = 0.0f;
            public List<int> affectedSections = new List<int>();
            public List<float> sectionMultipliers = new List<float>();
        }

        [System.Serializable]
        public class aircraft_ground_effect_data : aircraft_attachment_data
        {

        }

        [System.Serializable]
        public class aircraft_curve_value
        {
            public aircraft_curve_value(float time, float value)
            {
                this.time = time;
                this.value = value;
            }

            public float time = 0.0f;
            public float value = 0.0f;
        }

        [System.Serializable]
        public class aircraft_curve_data
        {
            public List<aircraft_curve_value> values = new List<aircraft_curve_value>();
        }

        [System.Serializable]
        public class aircraft_airfoil_data
        {
            public aircraft_curve_data cl = new aircraft_curve_data();
            public aircraft_curve_data cd = new aircraft_curve_data();
            public aircraft_curve_data cm = new aircraft_curve_data();

            public aircraft_curve_data clPlus10 = new aircraft_curve_data();
            public aircraft_curve_data cdPlus10 = new aircraft_curve_data();
            public aircraft_curve_data cmPlus10 = new aircraft_curve_data();

            public aircraft_curve_data clMinus10 = new aircraft_curve_data();
            public aircraft_curve_data cdMinus10 = new aircraft_curve_data();
            public aircraft_curve_data cmMinus10 = new aircraft_curve_data();
        }

        [System.Serializable]
        public class aircraft_wheel_data : aircraft_attachment_data
        {
            public float mass = 1.0f;
            public float radius = 0.02f;
            public float wheelDamping = 0.25f;
            public float suspensionDistance = 0.1f;
            public float springRate = 10.0f;
            public float suspensionDamper = 0;
            public float targetPosition = 0;

            public float forwardExtremumSlip = 0.4f;
            public float forwardExtrememValue = 1.0f;
            public float forwardAsymptoteSlip = 0.8f;
            public float forwardAsymptoteValue = 0.5f;
            public float forwardStiffness = 1.0f;

            public float sidewaysExtremumSlip = 0.2f;
            public float sidewaysExtrememValue = 1.0f;
            public float sidewaysAsymptoteSlip = 0.5f;
            public float sidewaysAsymptoteValue = 0.5f;
            public float sidewaysStiffness = 1.0f;
        }

        [System.Serializable]
        public class aircraft_data
        {
            public transform_data localTransform = new transform_data();
            public transform_data worldTransform = new transform_data();
            public List<aircraft_engine_data> engineData = new List<aircraft_engine_data>();
            public List<aircraft_wing_data> wingData = new List<aircraft_wing_data>();
            public List<aircraft_control_surface_data> controlSurfaceData = new List<aircraft_control_surface_data>();
            public List<aircraft_prop_wash_data> propWashData = new List<aircraft_prop_wash_data>();
            public List<aircraft_wheel_data> wheelData = new List<aircraft_wheel_data>();
            public List<aircraft_ground_effect_data> groundEffectData = new List<aircraft_ground_effect_data>();
            public bool aircraftEnabledAtStart = false;
            public bool overrideInertiaTensor = false;
            public vec4 inertiaTensor = vec4.zero;
            public vec4 cgPosition = vec4.zero;
            public vec4 drag = vec4.zero;
            public float rollwiseDamping = 0.0f;
            public float sectionMultiplier = 1.0f;
            public int debugFlags = 0;
        }

        [System.Serializable]
        public class vehicle_wheel_data : vehicle_attachment_data
        {
            public float mass = 1.0f;
            public float radius = 0.02f;
            public float wheelDamping = 0.25f;
            public float suspensionDistance = 0.1f;
            public float springRate = 10.0f;
            public float suspensionDamper = 0;
            public float targetPosition = 0;

            public float forwardExtremumSlip = 0.4f;
            public float forwardExtrememValue = 1.0f;
            public float forwardAsymptoteSlip = 0.8f;
            public float forwardAsymptoteValue = 0.5f;
            public float forwardStiffness = 1.0f;

            public float sidewaysExtremumSlip = 0.2f;
            public float sidewaysExtrememValue = 1.0f;
            public float sidewaysAsymptoteSlip = 0.5f;
            public float sidewaysAsymptoteValue = 0.5f;
            public float sidewaysStiffness = 1.0f;
        }

        [System.Serializable]
        public class truck_data
        {
            public transform_data localTransform = new transform_data();
            public transform_data worldTransform = new transform_data();
            public List<aircraft_engine_data> engineData = new List<aircraft_engine_data>();
            public List<vehicle_wheel_data> wheelData = new List<vehicle_wheel_data>();
            public bool overrideInertiaTensor = false;
            public vec4 inertiaTensor = vec4.zero;
            public float rollwiseDamping = 0.0f;
        }

        [System.Serializable]
        public class hue_material
        {
            public string name = "";
            public string shaderName = "";
        }


        [System.Serializable]
        public class hue_renderer
        {
            public List<hue_material> hueMaterials = new List<hue_material>();
        }

        [System.Serializable]
        public class hue_object
        {
            public string objectName = "";
            public List<hue_renderer> hueRenderers = new List<hue_renderer>();
            public List<hue_object> children = new List<hue_object>();
        }

        [System.Serializable]
        public class hue_data
        {
            public int selectedPreset = -1;
            public List<hue_object> hueObjectData = new List<hue_object>();
        }



        [System.Serializable]
        public class player_stats
        {
            public string playerName = "";

        }

        [System.Serializable]
        public class training_game_result
        {
            public string time = "00:00:00";
        }


        [System.Serializable]
        public class player_training
        {
            public List<training_game_result> trainingGameResults = new List<training_game_result>();
        }


        public class ServoHoleData
        {
            string name = "";
            float midPoint = 0.0f;
            float minPoint = 0.0f;
            float maxPoint = 0.0f;

            float minControlPoint = 0.0f;
            float maxControlPoint = 0.0f;
        }


        public class ServoData
        {
            string name = "";
            List<ServoHoleData> holes = new List<ServoHoleData>();
        }


        public class SwashPlateData
        {
            string name = "";
            List<ServoData> servos = new List<ServoData>();
            float collective = 0.0f;
            float pitch = 0.0f;
            float roll = 0.0f;
        }


        public class ModelControlData
        {
            List<SwashPlateData> swashPlates = new List<SwashPlateData>();
            List<ServoData> servos = new List<ServoData>();
        }

        public class ChannelAssignControlData
        {
            public void CopySettings(ChannelAssignControlData other)
            {
                id = other.id;
                axis = other.axis;
                button = other.button;
                minRange = other.minRange;
                maxRange = other.maxRange;
                isReversed = other.isReversed;
                isToggleMode = other.isToggleMode;
            }

            public int id = -1;
            public int axis = -1;
            public int button = -1;
            public float minRange = 0.0f;
            public float maxRange = 0.0f;
            public bool isReversed = false;
            public bool isToggleMode = false;
        }

        public class CMapData
        {
            public void CopySettings(CMapData other)
            {
                axis = other.axis;
                button = other.button;
                key = other.key;
                minRange = other.minRange;
                maxRange = other.maxRange;
                isReversed = other.isReversed;
                isToggleMode = other.isToggleMode;
                offset = other.offset;
                low_multiplier = other.low_multiplier;
                high_multiplier = other.high_multiplier;

                controlData = other.controlData;
            }

            public int axis = -1;
            public int button = -1;
            public int key = -1;
            public float minRange = 0.8f;
            public float maxRange = 1.0f;
            public bool isReversed = false;
            public bool isToggleMode = false;
            public float offset = 0.0f;
            public float low_multiplier = 1.0f;
            public float high_multiplier = 1.0f;

            public List<ChannelAssignControlData> controlData = new List<ChannelAssignControlData>();
        }

        [Serializable]
        public class TransmitterChannel
        {
            public void makeValid()
            {
                //if (!FB.FBMath.IsFinite(offset))
                //{
                //    offset = 1.0f;
                //}

                //if (!FB.FBMath.IsFinite(multiplier))
                //{
                //    multiplier = 1.0f;
                //}

                //if (!FB.FBMath.IsFinite(low_multiplier))
                //{
                //    low_multiplier = 1.0f;
                //}

                //if (!FB.FBMath.IsFinite(high_multiplier))
                //{
                //    high_multiplier = 1.0f;
                //}
            }

            public string Emulation
            {
                get { return emulation; }
                set
                {
                    if (emulation != value)
                    {
                        emulation = value;
                    }
                }
            }

            public string Map
            {
                get
                {
                    return cmap;
                }
                set
                {
                    cmap = value;

                    try
                    {
                        int val = -1;
                        if (!int.TryParse(cmap, out val))
                        {
                            mapData = JsonConvert.DeserializeObject<json.CMapData>(cmap);
                        }
                        else
                        {
                            mapData.axis = val;
                        }
                    }
                    catch (System.Exception ex)
                    {
                        // Debug.LogException(ex);
                    }
                }
            }

            public void CopySettings(TransmitterChannel other)
            {
                id = other.id;
                channel = other.channel;
                value = other.value;
                type = other.type;
                cmap = other.cmap;
                reverse = other.reverse;
                emulation = other.emulation;
                offset = other.offset;
                multiplier = other.multiplier;
                low_multiplier = other.low_multiplier;
                high_multiplier = other.high_multiplier;
                mapData = other.mapData;
            }

            public int id = -1;
            public int channel = -1;
            public string value = "";
            public string type = "";
            public string cmap = "-1";
            public bool reverse = false;
            public string emulation = "";
            public float offset = 0.0f;
            public float multiplier = 1.0f;
            public float low_multiplier = 1.0f;
            public float high_multiplier = 1.0f;
            public json.CMapData mapData = new json.CMapData();
        }


        [Serializable]
        public class TransmitterProfile
        {
            public TransmitterProfile()
            {

            }

            public void CopySettings(TransmitterProfile other)
            {
                id = other.id;
                guid = other.guid;
                name = other.name;
                preset = other.preset;
                adapterMode = other.adapterMode;
                deviceName = other.deviceName;
                deviceIndex = other.deviceIndex;
                cyclicDeadband = other.cyclicDeadband;
                gyroDeadband = other.gyroDeadband;
                rawInput = other.rawInput;
                functions = other.functions;
            }

            public void makeValid()
            {
                try
                {
                    foreach (var function in this.functions)
                    {
                        if (function != null)
                        {
                            function.makeValid();
                        }
                    }
                }
                catch (System.Exception ex)
                {
                    //Debug.LogException(ex);
                }
            }

            public int id = 0;
            public string guid = "";
            public string name = "My Transmitter";
            public string preset = "";
            public string adapterMode = "";
            public string deviceName = "";
            public int deviceIndex = 0;
            public int cyclicDeadband = 15;
            public int gyroDeadband = 15;
            public bool rawInput = true;
            public List<TransmitterChannel> functions = new List<TransmitterChannel>();

        }

    } // end namespace json

    static class JsonUtil
    {
        public static string GetFieldValue(this json.rows row, int idx)
        {
            List<json.field> fieldPairs = row.fieldPairs;
            return fieldPairs.ElementAt(idx).value;
        }

        public static string GetFieldValue(this json.query q, int idx)
        {
            List<json.field> fieldPairs = q.rows[0].fieldPairs;
            return fieldPairs.ElementAt(idx).value;
        }

        public static int GetFieldValueAsInt(this json.rows row, int idx)
        {
            List<json.field> fieldPairs = row.fieldPairs;
            return int.Parse(fieldPairs.ElementAt(idx).value);
        }

        public static int GetFieldValueAsInt(this json.query q, int idx)
        {
            List<json.field> fieldPairs = q.rows[0].fieldPairs;
            return int.Parse(fieldPairs.ElementAt(idx).value);
        }

        public static string GetFieldValue(this json.rows row, string name)
        {
            List<json.field> fieldPairs = row.fieldPairs;
            foreach (var fieldPair in fieldPairs)
            {
                if (fieldPair.name == name)
                {
                    return fieldPair.value;
                }
            }

            return "";
        }

        public static float GetFieldValueAsFloat(this json.rows row, string name)
        {
            List<json.field> fieldPairs = row.fieldPairs;
            foreach (var fieldPair in fieldPairs)
            {
                if (fieldPair.name == name)
                {
                    if (!string.IsNullOrEmpty(fieldPair.value))
                    {
                        return float.Parse(fieldPair.value);
                    }
                }
            }

            return 0.0f;
        }

        public static int GetFieldValueAsInt(this json.rows row, string name)
        {
            List<json.field> fieldPairs = row.fieldPairs;
            foreach (var fieldPair in fieldPairs)
            {
                if (fieldPair.name == name)
                {
                    if (!string.IsNullOrEmpty(fieldPair.value))
                    {
                        return int.Parse(fieldPair.value);
                    }
                }
            }

            return 0;
        }

        public static bool GetFieldValueAsBool(this json.rows row, string name)
        {
            List<json.field> fieldPairs = row.fieldPairs;
            foreach (var fieldPair in fieldPairs)
            {
                if (fieldPair.name == name)
                {
                    if (!string.IsNullOrEmpty(fieldPair.value))
                    {
                        return fieldPair.value == "true" || fieldPair.value == "True";
                    }
                }
            }

            return false;
        }

        public static json.vec4 StringToVector3(string sVector, char splitChar = ' ')
        {
            // Remove the parentheses
            if (sVector.StartsWith("(") && sVector.EndsWith(")"))
            {
                sVector = sVector.Substring(1, sVector.Length - 2);
            }

            // split the items
            string[] sArray = sVector.Split(splitChar);

            // store as a Vector3
            if (sArray.Length >= 3)
            {
                json.vec4 result = new json.vec4(
                    float.Parse(sArray[0]),
                    float.Parse(sArray[1]),
                    float.Parse(sArray[2]));

                return result;
            }

            return new vec4();
        }

        public static json.vec4 StringToQuaternion(string sVector, char splitChar = ' ')
        {
            // Remove the parentheses
            if (sVector.StartsWith("(") && sVector.EndsWith(")"))
            {
                sVector = sVector.Substring(1, sVector.Length - 2);
            }

            // split the items
            string[] sArray = sVector.Split(splitChar);

            // store as a Vector3
            if (sArray.Length >= 4)
            {
                json.vec4 result = new json.vec4(
                float.Parse(sArray[0]),
                float.Parse(sArray[1]),
                float.Parse(sArray[2]),
                float.Parse(sArray[3]));

                return result;
            }

            return new vec4();
        }
    }


} // end namespace saracen


