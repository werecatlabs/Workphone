#ifndef ParamHelper_h__
#define ParamHelper_h__

#include "WPPythonBind/WPPythonBindPrerequisites.hpp"
#include <Workphone/Base/Parameter.hpp>

namespace fb
{

    //---------------------------------------------------------------------------------------------------
    class ParamHelper
    {
    public:
        static bool getBool( const Parameter &param );
        static python_Integer getInt( const Parameter &param );
        static python_Number getNumber( const Parameter &param );
        static SmartPtr<ISharedObject> getScriptObject( const Parameter &param );
        static SmartPtr<scene::IActor> getEntity( const Parameter &param );

        static Parameters createIntParam( s32 value );
        static Parameters createF64Param( f64 value );
        static Parameters createParams( Parameter &param );
        static Parameters createParams( Parameter &param0, Parameter &param1 );
        static Parameters createParams( Parameter &param0, Parameter &param1, Parameter &param2 );
        static Parameters createParams( Parameter &param0, Parameter &param1, Parameter &param2,
                                        Parameter &param3 );
        static void addParamAsBool( Parameters &params, bool value );
        static void addParamAsInt( Parameters &params, python_Integer value );
        static void addParamAsNumber( Parameters &params, python_Number value );
        static Parameter getParam( Parameters &params, python_Integer index );
        static bool getParamAsBool( Parameters &params, python_Integer index );
        static python_Integer getParamAsInt( Parameters &params, python_Integer index );
        static python_Number getParamAsNumber( Parameters &params, python_Integer index );
        static SmartPtr<ISharedObject> getParamAsObject( Parameters &params, python_Integer index );
        static SmartPtr<scene::IActor> getParamAsEntity( Parameters &params, python_Integer index );
        static Parameter getParamAsStateMessage( Parameters &params, python_Integer index );
        static python_Integer getListSize( Parameters &params );
        //static void push_back(ParamArray& params, Parameter& param);
        //static Parameter getParamFromArray(ParamArray& params, python_Integer index);

        //template <class T>
        //static void _push_back_ptr(ParamArray& params, SmartPtr<T> newValue)
        //{
        //	Parameter param;
        //	param.setPtr(newValue.getPtr());
        //	params.push_back(param);
        //}
    };

}  // end namespace fb

#endif  // ParamHelper_h__
