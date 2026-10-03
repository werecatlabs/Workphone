#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/DisableScreenSave.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, DisableScreenSave, ISharedObject );

#if defined WP_PLATFORM_WIN32
    static u32 dss_GetList[] = { SPI_GETLOWPOWERTIMEOUT, SPI_GETPOWEROFFTIMEOUT,
                                 SPI_GETSCREENSAVETIMEOUT };
    static u32 dss_SetList[] = { SPI_SETLOWPOWERTIMEOUT, SPI_SETPOWEROFFTIMEOUT,
                                 SPI_SETSCREENSAVETIMEOUT };

    static const int dss_ListCount = _countof( dss_GetList );
#endif

    DisableScreenSave::DisableScreenSave()
    {
#if defined WP_PLATFORM_WIN32
        m_pValue = new int[dss_ListCount];

        for( int x = 0; x < dss_ListCount; x++ )
        {
            // Get the current value
            SystemParametersInfo( dss_GetList[x], 0, &m_pValue[x], 0 );

            //_T("%d = %d\n"), dss_GetList[x], m_pValue[x];

            // Turn off the parameter
            SystemParametersInfo( dss_SetList[x], 0, nullptr, 0 );
        }
#endif
    }

    DisableScreenSave::~DisableScreenSave()
    {
#if defined WP_PLATFORM_WIN32
        for( int x = 0; x < dss_ListCount; x++ )
        {
            // Set the old value
            SystemParametersInfo( dss_SetList[x], m_pValue[x], nullptr, 0 );
        }

        delete[] m_pValue;
#endif
    }
}  // namespace workphone
