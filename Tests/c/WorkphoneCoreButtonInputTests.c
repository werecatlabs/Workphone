#include "workphone.h"
#include <stdio.h>
#include <string.h>

static unsigned int assertions;
#define CHECK(value) do { ++assertions; if (!(value)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #value); return 0; } } while (0)

static struct wp_rect button_bounds(void)
{
    return wp_make_rect(10, 10, 100, 50);
}

static void frame(struct wp_context *ctx, int x, int y)
{
    wp_input_begin(ctx);
    wp_input_motion(ctx, x, y);
}

static int fast_complete_click(void)
{
    struct wp_context ctx;
    wp_flags state;
    memset(&ctx, 0, sizeof(ctx));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_false);
    wp_input_end(&ctx);
    CHECK(ctx.input.mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked == 2);
    CHECK(!ctx.input.mouse.buttons[WORKPHONE_BUTTON_LEFT].down);
    CHECK(wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    CHECK(state & WORKPHONE_WIDGET_STATE_HOVER);
    frame(&ctx, 30, 30);
    wp_input_end(&ctx);
    CHECK(!wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    return 1;
}

static int press_hold_release_is_one_activation(void)
{
    struct wp_context ctx;
    wp_flags state;
    memset(&ctx, 0, sizeof(ctx));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    wp_input_end(&ctx);
    CHECK(wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    CHECK(state & WORKPHONE_WIDGET_STATE_ACTIVE);
    frame(&ctx, 30, 30);
    wp_input_end(&ctx);
    CHECK(!wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_false);
    wp_input_end(&ctx);
    CHECK(ctx.input.mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked == 1);
    CHECK(!wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    return 1;
}

static int foreign_press_cannot_activate_on_inside_release(void)
{
    struct wp_context ctx;
    wp_flags state;
    memset(&ctx, 0, sizeof(ctx));
    frame(&ctx, 0, 0);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 0, 0, wp_true);
    wp_input_motion(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_false);
    wp_input_end(&ctx);
    CHECK(!wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    return 1;
}

static int outside_release_and_disabled_input_do_not_activate(void)
{
    struct wp_context ctx;
    wp_flags state;
    memset(&ctx, 0, sizeof(ctx));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    wp_input_motion(&ctx, 150, 150);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 150, 150, wp_false);
    wp_input_end(&ctx);
    CHECK(!wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    CHECK(!wp_button_behavior(&state, button_bounds(), NULL, WORKPHONE_BUTTON_DEFAULT));
    return 1;
}

static int duplicate_native_down_events_do_not_make_a_second_click(void)
{
    struct wp_context ctx;
    wp_flags state;
    memset(&ctx, 0, sizeof(ctx));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    CHECK(ctx.input.mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked == 1);
    wp_input_end(&ctx);
    CHECK(wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_false);
    wp_input_end(&ctx);
    CHECK(!wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    return 1;
}

static int new_click_after_previous_consumed_press_activates(void)
{
    struct wp_context ctx;
    wp_flags state;
    memset(&ctx, 0, sizeof(ctx));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    wp_input_end(&ctx);
    CHECK(wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_false);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_false);
    wp_input_end(&ctx);
    CHECK(ctx.input.mouse.buttons[WORKPHONE_BUTTON_LEFT].clicked == 3);
    CHECK(wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_DEFAULT));
    return 1;
}

static int repeater_still_requires_held_input(void)
{
    struct wp_context ctx;
    wp_flags state;
    memset(&ctx, 0, sizeof(ctx));
    frame(&ctx, 30, 30);
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_true);
    wp_input_end(&ctx);
    CHECK(wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_REPEATER));
    frame(&ctx, 30, 30);
    wp_input_end(&ctx);
    CHECK(wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_REPEATER));
    wp_input_button(&ctx, WORKPHONE_BUTTON_LEFT, 30, 30, wp_false);
    CHECK(!wp_button_behavior(&state, button_bounds(), &ctx.input, WORKPHONE_BUTTON_REPEATER));
    return 1;
}

int main(void)
{
    const struct { const char *name; int (*run)(void); } tests[] = {
        {"fast complete click", fast_complete_click},
        {"press hold release", press_hold_release_is_one_activation},
        {"foreign press", foreign_press_cannot_activate_on_inside_release},
        {"outside and disabled input", outside_release_and_disabled_input_do_not_activate},
        {"duplicate native down", duplicate_native_down_events_do_not_make_a_second_click},
        {"new click after consumed press", new_click_after_previous_consumed_press_activates},
        {"repeater", repeater_still_requires_held_input}
    };
    unsigned int index, failures = 0;
    for (index = 0; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (tests[index].run()) printf("PASS %s\n", tests[index].name);
        else { ++failures; printf("FAIL %s\n", tests[index].name); }
    }
    printf("WorkphoneCoreButtonInput: %u cases, %u assertions, %u failures\n", index, assertions, failures);
    return failures ? 1 : 0;
}
