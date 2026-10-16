#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "workphone_graphics_rtt_job.h"

#define CHECK(x) do { if(!(x)) { fprintf(stderr, "RTT contract line %d: %s\n", __LINE__, #x); return 1; } } while(0)
typedef struct fixture {
    wp_graphics_rtt_job *job;
    wp_graphics_rtt_desc desc;
    wp_graphics_rtt_ops ops;
    char order[32];
    int count, refs, retain_fail, begin_fail, draw_fail, finish_fail;
    int pending, stale, cancel_draw, reentry_ok, published, finish_publish, finish_cancel;
    int validations;
} fixture;
static void mark(fixture *f, char c) { f->order[f->count++] = c; f->order[f->count] = 0; }
static wp_s32 retain(void *p) { fixture *f = (fixture *)p; if(f->retain_fail) return 0; ++f->refs; return 1; }
static void release(void *p) { --((fixture *)p)->refs; }
static wp_rtt_result dependency(void *p, const wp_graphics_rtt_desc *d)
{ fixture *f = (fixture *)p; (void)d; mark(f, 'D'); return f->pending ? WP_RTT_NOT_READY : WP_RTT_OK; }
static wp_rtt_result validate(void *p, const wp_graphics_rtt_desc *d)
{
    fixture *f = (fixture *)p; mark(f, 'V'); ++f->validations;
    if(d->target_generation != 17 || d->width != 32 || (f->stale && f->validations == 2)) return WP_RTT_STALE;
    return WP_RTT_OK;
}
static wp_rtt_result begin(void *p, const wp_graphics_rtt_desc *d)
{ fixture *f = (fixture *)p; (void)d; mark(f, 'B'); return f->begin_fail ? WP_RTT_BACKEND_FAILED : WP_RTT_OK; }
static wp_rtt_result draw(void *p, const wp_graphics_rtt_desc *d)
{
    fixture *f = (fixture *)p; (void)d; mark(f, 'R');
    f->reentry_ok = wp_graphics_rtt_job_execute(f->job) == WP_RTT_BUSY &&
        wp_graphics_rtt_job_configure(f->job, &f->desc, &f->ops, f) == WP_RTT_BUSY &&
        wp_graphics_rtt_job_destroy(f->job) == WP_RTT_BUSY;
    if(f->cancel_draw) wp_graphics_rtt_job_cancel(f->job);
    return f->draw_fail ? WP_RTT_BACKEND_FAILED : WP_RTT_OK;
}
static wp_rtt_result finish(void *p, wp_s32 publish)
{
    fixture *f = (fixture *)p; mark(f, 'F'); f->finish_publish = publish;
    f->finish_cancel = wp_graphics_rtt_job_cancel(f->job) == WP_RTT_BUSY;
    if(f->finish_fail) return WP_RTT_BACKEND_FAILED;
    if(publish) ++f->published;
    return WP_RTT_OK;
}
static void reset(fixture *f)
{
    memset(f, 0, sizeof(*f)); f->job = wp_graphics_rtt_job_create();
    f->desc.width = f->desc.height = 32; f->desc.viewport.width = f->desc.viewport.height = 32;
    f->desc.target_generation = 17;
    f->ops.retain = retain; f->ops.release = release; f->ops.dependency = dependency;
    f->ops.validate = validate; f->ops.begin = begin; f->ops.draw = draw; f->ops.finish = finish;
}
int main(void)
{
    fixture f;
    int mode;
    reset(&f); CHECK(f.job && wp_graphics_rtt_job_status(f.job) == WP_RTT_IDLE);
    CHECK(wp_graphics_rtt_job_execute(f.job) == WP_RTT_INVALID);
    CHECK(wp_graphics_rtt_job_configure(f.job, &f.desc, &f.ops, &f) == WP_RTT_OK && f.refs == 1);
    f.desc.width = 99; /* the configured input is copied */
    f.pending = 1;
    CHECK(wp_graphics_rtt_job_execute(f.job) == WP_RTT_NOT_READY && !strcmp(f.order, "D"));
    CHECK(wp_graphics_rtt_job_status(f.job) == WP_RTT_READY && f.published == 0);
    f.pending = 0; f.count = 0;
    CHECK(wp_graphics_rtt_job_execute(f.job) == WP_RTT_OK && !strcmp(f.order, "DVBRVF"));
    CHECK(f.reentry_ok && f.finish_cancel && f.published == 1 && wp_graphics_rtt_job_status(f.job) == WP_RTT_SUCCEEDED);
    CHECK(wp_graphics_rtt_job_execute(f.job) == WP_RTT_INVALID);
    f.retain_fail = 1;
    CHECK(wp_graphics_rtt_job_configure(f.job, &f.desc, &f.ops, &f) == WP_RTT_BACKEND_FAILED);
    CHECK(f.refs == 1 && wp_graphics_rtt_job_status(f.job) == WP_RTT_SUCCEEDED && wp_graphics_rtt_job_result(f.job) == WP_RTT_OK);
    CHECK(wp_graphics_rtt_job_destroy(f.job) == WP_RTT_OK && f.refs == 0);
    for(mode = 0; mode < 5; ++mode) {
        reset(&f); f.begin_fail = mode == 0; f.draw_fail = mode == 1;
        f.stale = mode == 2; f.cancel_draw = mode == 3; f.finish_fail = mode == 4;
        CHECK(wp_graphics_rtt_job_configure(f.job, &f.desc, &f.ops, &f) == WP_RTT_OK);
        CHECK(wp_graphics_rtt_job_execute(f.job) == (mode == 2 ? WP_RTT_STALE : mode == 3 ? WP_RTT_CANCEL : WP_RTT_BACKEND_FAILED));
        CHECK(f.published == 0 && f.order[f.count - 1] == 'F' && (mode == 4 || !f.finish_publish));
        CHECK(wp_graphics_rtt_job_status(f.job) == (mode == 3 ? WP_RTT_CANCELLED : WP_RTT_FAILED));
        CHECK(wp_graphics_rtt_job_destroy(f.job) == WP_RTT_OK && f.refs == 0);
    }
    reset(&f);
    f.desc.viewport.x = INT_MAX;
    CHECK(wp_graphics_rtt_job_configure(f.job, &f.desc, &f.ops, &f) == WP_RTT_INVALID && f.refs == 0);
    f.desc.viewport.x = 0;
    CHECK(wp_graphics_rtt_job_configure(f.job, &f.desc, &f.ops, &f) == WP_RTT_OK);
    CHECK(wp_graphics_rtt_job_cancel(f.job) == WP_RTT_OK && wp_graphics_rtt_job_status(f.job) == WP_RTT_CANCELLED);
    CHECK(wp_graphics_rtt_job_execute(f.job) == WP_RTT_INVALID && f.count == 0);
    CHECK(wp_graphics_rtt_job_destroy(f.job) == WP_RTT_OK && f.refs == 0);
    puts("C90 RTT lifecycle, dependency publication, reentry, cancellation, stale generation and transactional finish contracts passed.");
    return 0;
}
