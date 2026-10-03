#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Helpers/ParamHelper.hpp>
#include <Workphone/Workphone.hpp>

namespace fb
{
    bool ParamHelper::getBool( const Parameter &param )
    {
        return param.data.bData;
    }

    python_Integer ParamHelper::getInt( const Parameter &param )
    {
        return param.data.iData;
    }

    python_Number ParamHelper::getNumber( const Parameter &param )
    {
        return param.data.fData;
    }

    SmartPtr<ISharedObject> ParamHelper::getScriptObject( const Parameter &param )
    {
        return static_cast<IObject *>(param.data.pData);
    }

    SmartPtr<scene::IActor> ParamHelper::getEntity( const Parameter &param )
    {
        return static_cast<scene::IActor *>(param.data.pData);
    }

    Parameters ParamHelper::createIntParam( s32 value )
    {
        Parameters params;
        //params.push_back( Parameter( value ) );
        return params;
    }

    Parameters ParamHelper::createF64Param( f64 value )
    {
        Parameters params;
        //params.push_back( Parameter( value ) );
        return params;
    }

    Parameters ParamHelper::createParams( Parameter &param )
    {
        Parameters params;
        //params.push_back( param );
        return params;
    }

    Parameters ParamHelper::createParams( Parameter &param0, Parameter &param1 )
    {
        Parameters params;
        //params.push_back( param0 );
        //params.push_back( param1 );
        return params;
    }

    Parameters ParamHelper::createParams( Parameter &param0, Parameter &param1, Parameter &param2 )
    {
        Parameters params;
        //params.push_back( param0 );
        //params.push_back( param1 );
        //params.push_back( param2 );
        return params;
    }

    Parameters ParamHelper::createParams( Parameter &param0, Parameter &param1, Parameter &param2,
                                          Parameter &param3 )
    {
        Parameters params;
        //params.push_back( param0 );
        //params.push_back( param1 );
        //params.push_back( param2 );
        //params.push_back( param3 );
        return params;
    }

    void ParamHelper::addParamAsBool( Parameters &params, bool value )
    {
        //params.push_back( Parameter( value ) );
    }

    void ParamHelper::addParamAsInt( Parameters &params, python_Integer value )
    {
        // params.push_back( Parameter( value ) );
    }

    void ParamHelper::addParamAsNumber( Parameters &params, python_Number value )
    {
        //params.push_back( Parameter( value ) );
    }

    Parameter ParamHelper::getParam( Parameters &params, python_Integer index )
    {
        return params[index];
    }

    bool ParamHelper::getParamAsBool( Parameters &params, python_Integer index )
    {
        return params[index].data.bData;
    }

    python_Integer ParamHelper::getParamAsInt( Parameters &params, python_Integer index )
    {
        return params[index].data.iData;
    }

    python_Number ParamHelper::getParamAsNumber( Parameters &params, python_Integer index )
    {
        return params[index].data.fData;
    }

    SmartPtr<ISharedObject> ParamHelper::getParamAsObject( Parameters &params, python_Integer index )
    {
        return static_cast<IObject *>(params[index].data.pData);
    }

    SmartPtr<scene::IActor> ParamHelper::getParamAsEntity( Parameters &params, python_Integer index )
    {
        return static_cast<scene::IActor *>(params[index].data.pData);
    }

    Parameter ParamHelper::getParamAsStateMessage( Parameters &params, python_Integer index )
    {
        Parameter param;
        param.setPtr( params[index].data.pData );
        return param;
    }

    python_Integer ParamHelper::getListSize( Parameters &params )
    {
        return static_cast<python_Integer>(params.size());
    }

    //void ParamHelper::push_back( ParamArray& params, Parameter& param )
    //{
    //	params.push_back(param);
    //}

    //fb::Parameter ParamHelper::getParamFromArray( ParamArray& params, python_Integer index )
    //{
    //	return params[index];
    //}
} // end namespace fb
