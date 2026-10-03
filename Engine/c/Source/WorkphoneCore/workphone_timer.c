#include <workphone_timer.h>
#include <string.h>

#ifdef _WIN32
#    include <windows.h>
#endif

typedef struct
{
    double start_real_time;    ///< Real time at initialization/reset
    double current_real_time;  ///< Last updated real time
    double virtual_time;       ///< Current virtual time
    double delta_time;         ///< Delta time since last update
    float virtual_speed;       ///< Speed multiplier
    bool is_paused;            ///< Pause state
    uint64_t freq;             ///< QPC frequency
} wp_timer_state_t;

static wp_timer_state_t g_timer_state = { 0 };

static double get_raw_real_time( void )
{
#ifdef _WIN32
    LARGE_INTEGER t;
    if( QueryPerformanceCounter( &t ) )
    {
        return (double)t.QuadPart / g_timer_state.freq;
    }
#endif
    return 0.0;
}

void wp_timer_init( void )
{
#ifdef _WIN32
    LARGE_INTEGER freq;
    QueryPerformanceFrequency( &freq );
    g_timer_state.freq = freq.QuadPart;
#endif
    g_timer_state.virtual_speed = 1.0f;
    g_timer_state.is_paused = false;
    g_timer_state.start_real_time = get_raw_real_time();
    g_timer_state.current_real_time = g_timer_state.start_real_time;
    g_timer_state.virtual_time = 0.0;
    g_timer_state.delta_time = 0.0;
}

void wp_timer_update( void )
{
    double now = get_raw_real_time();
    double real_delta = now - g_timer_state.current_real_time;

    // Guard against negative delta (clock jump/reset)
    if( real_delta < 0.0 )
    {
        real_delta = 0.0;
    }

    g_timer_state.current_real_time = now;

    if( g_timer_state.is_paused )
    {
        g_timer_state.delta_time = 0.0;
    }
    else
    {
        g_timer_state.delta_time = real_delta * g_timer_state.virtual_speed;
        g_timer_state.virtual_time += g_timer_state.delta_time;
    }
}

double wp_timer_get_time( void )
{
    return g_timer_state.virtual_time;
}

double wp_timer_get_delta_time( void )
{
    return g_timer_state.delta_time;
}

double wp_timer_get_real_time( void )
{
    return get_raw_real_time() - g_timer_state.start_real_time;
}

void wp_timer_set_speed( float speed )
{
    g_timer_state.virtual_speed = ( speed < 0.0f ) ? 0.0f : speed;
}

float wp_timer_get_speed( void )
{
    return g_timer_state.virtual_speed;
}

void wp_timer_pause( void )
{
    g_timer_state.is_paused = true;
}

void wp_timer_resume( void )
{
    g_timer_state.is_paused = false;
}

bool wp_timer_is_paused( void )
{
    return g_timer_state.is_paused;
}

void wp_timer_set_time( double time )
{
    g_timer_state.virtual_time = time;
}

void wp_timer_reset( void )
{
    g_timer_state.start_real_time = get_raw_real_time();
    g_timer_state.current_real_time = g_timer_state.start_real_time;
    g_timer_state.virtual_time = 0.0;
    g_timer_state.delta_time = 0.0;
}
