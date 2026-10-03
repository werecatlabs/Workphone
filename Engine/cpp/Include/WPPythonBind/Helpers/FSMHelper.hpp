#ifndef FSMHelper_h__
#define FSMHelper_h__

#include <WPPythonBind/WPPythonBindPrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace fb
{

    class FSMHelper
    {
    public:
        static void setFSMState( SmartPtr<IFSM> fsm, python_Integer state );
        static python_Integer getFSMState( SmartPtr<IFSM> fsm );
        static void setFSMInitialState( SmartPtr<IFSM> fsm, python_Integer state );

        //static SmartPtr<IFSM> _getFSM( SmartPtr<scene::FSMContainer> container, python_Integer id );

        //static SmartPtr<IFSM> _getFSMByName( FSMContainerPtr container, const char *name );
    };

}  // end namespace fb

#endif  // FSMHelper_h__
