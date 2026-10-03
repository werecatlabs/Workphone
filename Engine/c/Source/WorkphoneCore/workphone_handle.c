#include "workphone_handle.h"

wp_handle wp_handle_ptr( void *ptr )
{
    wp_handle handle = { 0 };
    handle.ptr = ptr;
    return handle;
}

wp_handle wp_handle_id( wp_s32 id )
{
    wp_handle handle;
    wp_zero_struct( handle );
    handle.id = id;
    return handle;
}
