#ifndef workphone_utf8_h__
#define workphone_utf8_h__

#include "workphone_prerequisites.h"

WORKPHONE_API int wp_utf_decode( const char *, wp_rune *, int );
WORKPHONE_API int wp_utf_encode( wp_rune, char *, int );
WORKPHONE_API int wp_utf_len( const char *, int byte_len );
WORKPHONE_API const char *wp_utf_at( const char *buffer, int length, int index, wp_rune *unicode,
                                     int *len );

#endif  // workphone_utf8_h__
