#include <nfd.h>

extern "C"
{
    nfdresult_t NFD_OpenDialog( const nfdchar_t *, const nfdchar_t *, nfdchar_t **outPath )
    {
        if( outPath )
        {
            *outPath = nullptr;
        }

        return NFD_CANCEL;
    }

    nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *, const nfdchar_t *, nfdpathset_t *outPaths )
    {
        if( outPaths )
        {
            outPaths->buf = nullptr;
            outPaths->indices = nullptr;
            outPaths->count = 0;
        }

        return NFD_CANCEL;
    }

    nfdresult_t NFD_SaveDialog( const nfdchar_t *, const nfdchar_t *, nfdchar_t **outPath )
    {
        if( outPath )
        {
            *outPath = nullptr;
        }

        return NFD_CANCEL;
    }

    nfdresult_t NFD_PickFolder( const nfdchar_t *, nfdchar_t **outPath )
    {
        if( outPath )
        {
            *outPath = nullptr;
        }

        return NFD_CANCEL;
    }

    const char *NFD_GetError( void )
    {
        return "Native file dialogs are not available on Android.";
    }

    void NFD_PathSet_Free( nfdpathset_t *pathSet )
    {
        if( pathSet )
        {
            pathSet->buf = nullptr;
            pathSet->indices = nullptr;
            pathSet->count = 0;
        }
    }

    size_t NFD_PathSet_GetCount( const nfdpathset_t *pathSet )
    {
        return pathSet ? pathSet->count : 0;
    }

    nfdchar_t *NFD_PathSet_GetPath( const nfdpathset_t *, size_t )
    {
        return nullptr;
    }
}
