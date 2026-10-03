#include "HelloWorld.h"

#include <stdio.h>

#include <workphone.h>

#define SAMPLE_MEMORY_SIZE ( 64u * 1024u )

int sample_hello_world_run( void )
{
    unsigned char memory[SAMPLE_MEMORY_SIZE];
    struct wp_context context;
    struct wp_rect bounds;
    const struct wp_command *command;
    unsigned int command_count;

    bounds = wp_make_rect( 20.0f, 20.0f, 320.0f, 160.0f );

    if( !wp_init_fixed( &context, memory, sizeof( memory ), 0 ) )
    {
        fprintf( stderr, "SampleHelloWorldC89: failed to initialize Workphone.\n" );
        return 1;
    }

    wp_input_begin( &context );

    if( wp_begin( &context, "Hello World", bounds, WORKPHONE_WINDOW_TITLE ) )
    {
        wp_layout_row_dynamic( &context, 32.0f, 1 );
        wp_label( &context, "Hello, Workphone!", WORKPHONE_WIDGET_CENTERED );
        wp_end( &context );
    }

    wp_input_end( &context );
    wp_build( &context );

    command_count = 0;
    command = wp__begin( &context );
    while( command )
    {
        ++command_count;
        command = wp__next( &context, command );
    }

    printf( "Hello, Workphone!\n" );
    printf( "Recorded %u UI command(s).\n", command_count );

    wp_clear( &context );
    return 0;
}

int main( void )
{
    return sample_hello_world_run();
}
