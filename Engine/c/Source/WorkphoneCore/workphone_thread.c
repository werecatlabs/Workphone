#include "workphone_thread.h"

#ifdef _WIN32
#    include <windows.h>
#else
#    include <time.h>
#    include <errno.h>
#endif

WORKPHONE_LIB void wp_thread_sleep( wp_f32 seconds )
{
    if( seconds <= 0.0f )
        return;

#ifdef _WIN32
    Sleep( (DWORD)( seconds * 1000.0f ) );
#else
    {
        struct timespec req;
        struct timespec rem;
        req.tv_sec = (time_t)seconds;
        req.tv_nsec = (long)( ( seconds - (wp_f32)req.tv_sec ) * 1e9f );
        while( nanosleep( &req, &rem ) == -1 && errno == EINTR )
            req = rem;
    }
#endif
}
