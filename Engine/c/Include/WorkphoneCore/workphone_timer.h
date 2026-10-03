#ifndef WORKPHONE_TIMER_H
#define WORKPHONE_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the timer system.
 * Must be called before any other timer functions.
 */
void wp_timer_init( void );

/**
 * @brief Updates the timer state.
 * Should be called once per frame. Calculates delta time and updates virtual time.
 */
void wp_timer_update( void );

/**
 * @brief Gets the current virtual (game) time in seconds.
 * @return The virtual time in seconds.
 */
double wp_timer_get_time( void );

/**
 * @brief Gets the time elapsed since the last update in seconds.
 * @return The delta time in seconds.
 */
double wp_timer_get_delta_time( void );

/**
 * @brief Gets the current real (wall-clock) time in seconds since initialization.
 * @return The real time in seconds.
 */
double wp_timer_get_real_time( void );

/**
 * @brief Sets the speed multiplier for the virtual timer.
 * @param speed The speed multiplier (1.0 = real time, 2.0 = double speed, 0.0 = paused).
 */
void wp_timer_set_speed( float speed );

/**
 * @brief Gets the current speed multiplier.
 * @return The speed multiplier.
 */
float wp_timer_get_speed( void );

/**
 * @brief Pauses the virtual timer.
 */
void wp_timer_pause( void );

/**
 * @brief Resumes the virtual timer.
 */
void wp_timer_resume( void );

/**
 * @brief Checks if the virtual timer is paused.
 * @return True if paused, false otherwise.
 */
bool wp_timer_is_paused( void );

/**
 * @brief Sets the current virtual time.
 * @param time The virtual time to set in seconds.
 */
void wp_timer_set_time( double time );

/**
 * @brief Resets the timer.
 */
void wp_timer_reset( void );

#ifdef __cplusplus
}
#endif

#endif  // WORKPHONE_TIMER_H
