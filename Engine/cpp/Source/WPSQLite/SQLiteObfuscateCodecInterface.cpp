//#include "WPSQLite/SQLiteObfuscateCodecInterface.hpp"
//#include <sqlite3.hpp>

#ifndef SQLITE_OMIT_DISKIO
#    ifdef SQLITE_HAS_CODEC

extern "C" {

#        ifdef __cplusplus
using Bool = unsigned char;
#        endif
}

#    endif  // SQLITE_HAS_CODEC

#endif  // SQLITE_OMIT_DISKIO
