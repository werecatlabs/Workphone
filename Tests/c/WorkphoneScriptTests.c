#include "workphone_script_method.h"
#include "workphone_script_object.h"
#include "workphone_script_parser.h"
#include "workphone_script_state.h"
#include "workphone_script_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct wp_script_test_counter
{
    wp_f64 total;
    wp_s32 destroy_count;
} wp_script_test_counter;

static void *test_create_counter( wp_script_state *state, wp_script_object *self,
                                  void *user_data )
{
    (void)state;
    (void)self;
    return user_data;
}

static void test_destroy_counter( void *native_object, void *user_data )
{
    wp_script_test_counter *counter = (wp_script_test_counter *)native_object;
    (void)user_data;
    counter->destroy_count++;
}

static wp_s32 test_add( wp_script_state *state, wp_script_object *self,
                        const wp_script_value *args, wp_s32 arg_count,
                        wp_script_value *result, void *user_data )
{
    wp_script_test_counter *counter;
    (void)user_data;

    counter = (wp_script_test_counter *)wp_script_object_get_native( self );
    if( !counter || arg_count != 1 || args[0].type != VAL_NUMBER )
    {
        wp_script_set_error( state, "invalid Counter:add call" );
        return 0;
    }
    counter->total += args[0].as.number;
    *result = wp_script_make_number( counter->total );
    return 1;
}

static wp_s32 test_expect( wp_s32 condition, const char *message )
{
    if( condition )
        return 1;
    fprintf( stderr, "FAIL: %s\n", message );
    return 0;
}

int main( void )
{
    static const char script[] =
        "class 'Intermediate' (Counter)\n"
        "function Intermediate:addTwice(value)\n"
        "  self:add(value)\n"
        "  Counter.add(self, value)\n"
        "end\n"
        "class 'CounterApp' (Intermediate)\n"
        "function CounterApp:__init()\n"
        "  self:addTwice(2)\n"
        "end\n"
        "function CounterApp:update(dt)\n"
        "  local total = 0\n"
        "  for i = 1, dt do\n"
        "    total = total + 1\n"
        "  end\n"
        "  self:add(total)\n"
        "end\n"
        "function CounterApp:echo(value)\n"
        "  return value\n"
        "end\n"
        "app = CounterApp()\n"
        "app:update(3)\n";
    wp_script_state *state;
    wp_script_state *bad_state;
    wp_script_class *counter_class;
    wp_script_value app_value;
    wp_script_value argument;
    wp_script_value result;
    wp_script_test_counter counter;
    wp_script_table collision_table;
    char first_key[32];
    char second_key[32];
    wp_u32 target_bucket;
    wp_s32 i;
    wp_s32 ok;

    memset( &counter, 0, sizeof( counter ) );
    memset( &collision_table, 0, sizeof( collision_table ) );
    state = wp_script_create_state();
    if( !test_expect( state != NULL, "state creation" ) )
        return 1;
    if( !test_expect( wp_script_get_lua_state( state ) != NULL,
                      "Lua state creation" ) )
        return 1;

    counter_class = wp_script_bind_class( state, "Counter", NULL, test_create_counter,
                                          test_destroy_counter, &counter );
    ok = test_expect( counter_class != NULL, "native class binding" );
    ok = test_expect( wp_script_bind_method( counter_class, "add", test_add, NULL ),
                      "native method binding" ) && ok;
    ok = test_expect( wp_script_compile_ex( state, script ), wp_script_get_last_error( state ) ) &&
         ok;
    if( ok )
    {
        wp_s32 run_ok = wp_script_run_from_ex( state, wp_script_get_compile_offset( state ),
                                               &result );
        ok = test_expect( run_ok, wp_script_get_last_error( state ) ) && ok;
    }

    app_value = wp_script_table_get( wp_script_get_globals_table( state ), "app" );
    ok = test_expect( app_value.type == VAL_OBJECT && app_value.as.object != NULL,
                      "script-created derived object" ) &&
         ok;
    ok = test_expect( counter.total == 7.0,
                      "script method bodies, inheritance, and native calls" ) &&
         ok;

    argument = wp_script_make_number( 7.0 );
    if( app_value.type == VAL_OBJECT )
    {
        wp_s32 call_ok = wp_script_call_method_args( app_value.as.object, "echo", &argument, 1,
                                                     &result );
        ok = test_expect( call_ok, wp_script_get_last_error( state ) ) && ok;
        ok = test_expect( result.type == VAL_NUMBER && result.as.number == 7.0,
                          "script parameters and return values" ) &&
             ok;
    }

    strcpy( first_key, "key0" );
    target_bucket = wp_script_table_hash( first_key ) % TABLE_SIZE;
    second_key[0] = '\0';
    for( i = 1; i < 10000; ++i )
    {
        sprintf( second_key, "key%d", i );
        if( wp_script_table_hash( second_key ) % TABLE_SIZE == target_bucket )
            break;
    }
    wp_script_table_set( &collision_table, first_key, wp_script_make_number( 1.0 ) );
    wp_script_table_set( &collision_table, second_key, wp_script_make_number( 2.0 ) );
    ok = test_expect( wp_script_table_get( &collision_table, first_key ).as.number == 1.0 &&
                          wp_script_table_get( &collision_table, second_key ).as.number == 2.0,
                      "hash collision probing" ) &&
         ok;
    wp_script_table_clear( &collision_table );

    bad_state = wp_script_create_state();
    ok = test_expect( bad_state != NULL, "invalid-script test state" ) && ok;
    if( bad_state )
    {
        ok = test_expect( !wp_script_compile_ex( bad_state,
                                                 "class 'Broken'\n"
                                                 "function Broken:missingEnd()\n" ),
                          "malformed script rejection" ) &&
             ok;
        ok = test_expect( wp_script_get_last_error( bad_state )[0] != '\0',
                          "malformed script error message" ) &&
             ok;
        wp_script_destroy_state( bad_state );
    }

    wp_script_destroy_state( state );
    ok = test_expect( counter.destroy_count == 1, "native object destructor" ) && ok;
    if( !ok )
        return 1;
    printf( "WorkphoneScript tests passed\n" );
    return 0;
}
