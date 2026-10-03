#ifndef workphone_cursor_h__
#define workphone_cursor_h__

#include "workphone_image.h"
#include "workphone_vector.h"

struct wp_cursor
{
    struct wp_image img;
    struct wp_vec2f size;
    struct wp_vec2f offset;
};

#endif  // workphone_cursor_h__
