#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* C89 bool equivalent */
typedef int WP_BOOL;
#define WP_TRUE 1
#define WP_FALSE 0

/* Texture Scope equivalent to Ogre::CompositionTechnique::TextureScope */
typedef enum
{
    WP_GFX_TS_LOCAL,
    WP_GFX_TS_CHAIN,
    WP_GFX_TS_GLOBAL
} WP_GFX_TextureScope;

/* Texture Definition equivalent to Ogre::CompositionTechnique::TextureDefinition */
typedef struct
{
    char *name;
    char *refCompName;
    char *refTexName;
    unsigned int width;
    unsigned int height;
    int type; /* TextureType */
    float widthFactor;
    float heightFactor;
    /* PixelFormatList would be a complex type, using a placeholder or int for now */
    int formatList;
    WP_BOOL fsaa;
    WP_BOOL hwGammaWrite;
    unsigned short depthBufferId;
    WP_BOOL pooled;
    WP_GFX_TextureScope scope;
} WP_GFX_TextureDefinition;

/* Target Pass placeholder - Ogre code references CompositionTargetPass */
typedef struct WP_GFX_TargetPass
{
    /* Details would come from CompositionTargetPass.h */
    int dummy;
} WP_GFX_TargetPass;

/* Composition Technique equivalent to Ogre::CompositionTechnique */
typedef struct
{
    struct WP_GFX_Compositor *parent;
    WP_GFX_TextureDefinition **textureDefinitions;
    size_t numTextureDefinitions;
    size_t textureDefinitionsCapacity;
    WP_GFX_TargetPass **targetPasses;
    size_t numTargetPasses;
    size_t targetPassesCapacity;
    char *schemeName;
} WP_GFX_CompositionTechnique;

/* Compositor equivalent to Ogre::Compositor */
typedef struct WP_GFX_Compositor
{
    char *name;
    char *group;
    WP_GFX_CompositionTechnique **techniques;
    size_t numTechniques;
    size_t techniquesCapacity;
    WP_GFX_CompositionTechnique **supportedTechniques;
    size_t numSupportedTechniques;
    size_t supportedTechniquesCapacity;
    WP_BOOL compilationRequired;
} WP_GFX_Compositor;

/* --- Prototypes --- */

WP_GFX_Compositor *wp_graphics_compositor_create( const char *name, const char *group );
void wp_graphics_compositor_destroy( WP_GFX_Compositor *compositor );
WP_GFX_CompositionTechnique *wp_graphics_compositor_create_technique( WP_GFX_Compositor *compositor );
void wp_graphics_compositor_remove_technique( WP_GFX_Compositor *compositor, size_t index );
void wp_graphics_compositor_remove_all_techniques( WP_GFX_Compositor *compositor );
void wp_graphics_compositor_compile( WP_GFX_Compositor *compositor );
WP_GFX_CompositionTechnique *wp_graphics_compositor_get_supported_technique(
    WP_GFX_Compositor *compositor, const char *schemeName );
void wp_graphics_composition_technique_destroy( WP_GFX_CompositionTechnique *t );

const char *wp_graphics_compositor_get_texture_instance_name( WP_GFX_Compositor *compositor,
                                                              const char *name, size_t mrtIndex );
void *wp_graphics_compositor_get_texture_instance( WP_GFX_Compositor *compositor, const char *name,
                                                   size_t mrtIndex );
void *wp_graphics_compositor_get_render_target( WP_GFX_Compositor *compositor, const char *name,
                                                int slice );

/* --- Implementations --- */

/* Helper to duplicate strings for C89 (since strdup is not C89) */
static char *wp_gfx_strdup( const char *s )
{
    if( !s )
        return NULL;
    size_t len = strlen( s ) + 1;
    char *res = (char *)malloc( len );
    if( res )
        memcpy( res, s, len );
    return res;
}

WP_GFX_Compositor *wp_graphics_compositor_create( const char *name, const char *group )
{
    WP_GFX_Compositor *c = (WP_GFX_Compositor *)malloc( sizeof( WP_GFX_Compositor ) );
    if( !c )
        return NULL;

    c->name = wp_gfx_strdup( name );
    c->group = wp_gfx_strdup( group );
    c->techniques = NULL;
    c->numTechniques = 0;
    c->techniquesCapacity = 0;
    c->supportedTechniques = NULL;
    c->numSupportedTechniques = 0;
    c->supportedTechniquesCapacity = 0;
    c->compilationRequired = WP_TRUE;

    return c;
}

void wp_graphics_composition_technique_destroy( WP_GFX_CompositionTechnique *t )
{
    if( !t )
        return;

    size_t i;
    /* Clean up texture definitions */
    for( i = 0; i < t->numTextureDefinitions; ++i )
    {
        WP_GFX_TextureDefinition *td = t->textureDefinitions[i];
        if( td )
        {
            free( td->name );
            free( td->refCompName );
            free( td->refTexName );
            free( td );
        }
    }
    free( t->textureDefinitions );

    /* Clean up target passes */
    for( i = 0; i < t->numTargetPasses; ++i )
    {
        free( t->targetPasses[i] );
    }
    free( t->targetPasses );

    free( t->schemeName );
    free( t );
}

void wp_graphics_compositor_remove_all_techniques( WP_GFX_Compositor *compositor )
{
    size_t i;
    if( !compositor || !compositor->techniques )
        return;

    for( i = 0; i < compositor->numTechniques; ++i )
    {
        wp_graphics_composition_technique_destroy( compositor->techniques[i] );
    }
    free( compositor->techniques );
    compositor->techniques = NULL;
    compositor->numTechniques = 0;
    compositor->techniquesCapacity = 0;

    if( compositor->supportedTechniques )
    {
        free( compositor->supportedTechniques );
        compositor->supportedTechniques = NULL;
        compositor->numSupportedTechniques = 0;
        compositor->supportedTechniquesCapacity = 0;
    }
    compositor->compilationRequired = WP_TRUE;
}

void wp_graphics_compositor_destroy( WP_GFX_Compositor *compositor )
{
    if( !compositor )
        return;

    wp_graphics_compositor_remove_all_techniques( compositor );

    free( compositor->name );
    free( compositor->group );
    free( compositor );
}

WP_GFX_CompositionTechnique *wp_graphics_compositor_create_technique( WP_GFX_Compositor *compositor )
{
    if( !compositor )
        return NULL;

    WP_GFX_CompositionTechnique *t =
        (WP_GFX_CompositionTechnique *)malloc( sizeof( WP_GFX_CompositionTechnique ) );
    if( !t )
        return NULL;

    t->parent = compositor;
    t->textureDefinitions = NULL;
    t->numTextureDefinitions = 0;
    t->textureDefinitionsCapacity = 0;
    t->targetPasses = NULL;
    t->numTargetPasses = 0;
    t->targetPassesCapacity = 0;
    t->schemeName = NULL;

    /* Add technique to compositor */
    if( compositor->numTechniques >= compositor->techniquesCapacity )
    {
        size_t newCapacity =
            ( compositor->techniquesCapacity == 0 ) ? 4 : compositor->techniquesCapacity * 2;
        WP_GFX_CompositionTechnique **newTechniques = (WP_GFX_CompositionTechnique **)realloc(
            compositor->techniques, newCapacity * sizeof( WP_GFX_CompositionTechnique * ) );
        if( !newTechniques )
        {
            free( t );
            return NULL;
        }
        compositor->techniques = newTechniques;
        compositor->techniquesCapacity = newCapacity;
    }

    compositor->techniques[compositor->numTechniques++] = t;
    compositor->compilationRequired = WP_TRUE;

    return t;
}

void wp_graphics_compositor_remove_technique( WP_GFX_Compositor *compositor, size_t index )
{
    if( !compositor || index >= compositor->numTechniques )
        return;

    /* Destroy the technique */
    wp_graphics_composition_technique_destroy( compositor->techniques[index] );

    /* Shift remaining techniques */
    size_t i;
    for( i = index; i < compositor->numTechniques - 1; ++i )
    {
        compositor->techniques[i] = compositor->techniques[i + 1];
    }
    compositor->numTechniques--;
    compositor->compilationRequired = WP_TRUE;
}

/* Placeholder for technique support check */
static WP_BOOL wp_graphics_composition_technique_is_supported( WP_GFX_CompositionTechnique *t )
{
    /* In a real implementation, this would check hardware capabilities */
    return WP_TRUE;
}

void wp_graphics_compositor_compile( WP_GFX_Compositor *compositor )
{
    if( !compositor )
        return;

    size_t i;
    /* Clear existing supported techniques list */
    if( compositor->supportedTechniques )
    {
        free( compositor->supportedTechniques );
        compositor->supportedTechniques = NULL;
        compositor->numSupportedTechniques = 0;
        compositor->supportedTechniquesCapacity = 0;
    }

    for( i = 0; i < compositor->numTechniques; ++i )
    {
        WP_GFX_CompositionTechnique *t = compositor->techniques[i];
        if( wp_graphics_composition_technique_is_supported( t ) )
        {
            /* Expand supportedTechniques array if needed */
            if( compositor->numSupportedTechniques >= compositor->supportedTechniquesCapacity )
            {
                size_t newCapacity = ( compositor->supportedTechniquesCapacity == 0 )
                                         ? 4
                                         : compositor->supportedTechniquesCapacity * 2;
                WP_GFX_CompositionTechnique **newSupported = (WP_GFX_CompositionTechnique **)realloc(
                    compositor->supportedTechniques,
                    newCapacity * sizeof( WP_GFX_CompositionTechnique * ) );
                if( !newSupported )
                {
                    /* Error handling: return or log */
                    return;
                }
                compositor->supportedTechniques = newSupported;
                compositor->supportedTechniquesCapacity = newCapacity;
            }
            compositor->supportedTechniques[compositor->numSupportedTechniques++] = t;
        }
    }

    if( compositor->numSupportedTechniques == 0 )
    {
        printf( "Compositor '%s' has no supported techniques\n", compositor->name );
    }

    compositor->compilationRequired = WP_FALSE;
}

WP_GFX_CompositionTechnique *wp_graphics_compositor_get_supported_technique(
    WP_GFX_Compositor *compositor, const char *schemeName )
{
    if( !compositor || !compositor->supportedTechniques )
        return NULL;

    size_t i;
    /* First pass: Look for a match with the specified scheme name */
    for( i = 0; i < compositor->numSupportedTechniques; ++i )
    {
        WP_GFX_CompositionTechnique *t = compositor->supportedTechniques[i];
        if( t->schemeName && schemeName && strcmp( t->schemeName, schemeName ) == 0 )
        {
            return t;
        }
    }

    /* Second pass: Look for a technique with no scheme (empty or NULL) */
    for( i = 0; i < compositor->numSupportedTechniques; ++i )
    {
        WP_GFX_CompositionTechnique *t = compositor->supportedTechniques[i];
        if( !t->schemeName || strlen( t->schemeName ) == 0 )
        {
            return t;
        }
    }

    return NULL;
}

const char *wp_graphics_compositor_get_texture_instance_name( WP_GFX_Compositor *compositor,
                                                              const char *name, size_t mrtIndex )
{
    /* Placeholder: In Ogre this maps a definition name to a real texture name */
    (void)compositor;
    (void)name;
    (void)mrtIndex;
    return NULL;
}

void *wp_graphics_compositor_get_texture_instance( WP_GFX_Compositor *compositor, const char *name,
                                                   size_t mrtIndex )
{
    /* Placeholder: Returns the actual texture object */
    (void)compositor;
    (void)name;
    (void)mrtIndex;
    return NULL;
}

void *wp_graphics_compositor_get_render_target( WP_GFX_Compositor *compositor, const char *name,
                                                int slice )
{
    /* Placeholder: Returns the RenderTarget object */
    (void)compositor;
    (void)name;
    (void)slice;
    return NULL;
}
