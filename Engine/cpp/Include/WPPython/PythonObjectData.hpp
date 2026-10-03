#ifndef PythonScriptData_h__
#define PythonScriptData_h__

#include "WPPython/WPPythonPrerequisites.hpp"
#include <Workphone/Interface/Script/IScriptData.hpp>
#include <Workphone/Base/StringTypes.hpp>
#include <Workphone/Memory/CSharedObject.hpp>
#include <boost/python/object.hpp>

namespace fb
{

    class PythonObjectData : public CSharedObject<IScriptData>
    {
    public:
        PythonObjectData();
        ~PythonObjectData();

        virtual void setOwner( SmartPtr<ISharedObject> owner );
        virtual SmartPtr<ISharedObject> getOwner() const;

        void *getObjectData() const;

        String getClassName() const;
        void setClassName( const String &className );

        boost::python::object &getObject();

    protected:
        SmartPtr<ISharedObject> m_owner;
        String m_className;

        /// Object if using a single lua state.
        boost::python::object m_object;
    };

}  // end namespace fb

#endif  // PythonScriptData_h__
