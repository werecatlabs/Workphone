#include "workphone_graphics_rtt_job.h"
#include <stdlib.h>
#include <string.h>

struct wp_graphics_rtt_job {
    wp_graphics_rtt_desc desc;
    wp_graphics_rtt_ops ops;
    void *context;
    wp_rtt_status status;
    wp_rtt_result result;
    wp_s32 cancelled, finishing;
};

wp_graphics_rtt_job *wp_graphics_rtt_job_create(void)
{
    wp_graphics_rtt_job *job = (wp_graphics_rtt_job *)calloc(1, sizeof(*job));
    if(job) { job->status = WP_RTT_IDLE; job->result = WP_RTT_INVALID; }
    return job;
}

wp_rtt_result wp_graphics_rtt_job_configure(wp_graphics_rtt_job *job,
    const wp_graphics_rtt_desc *desc, const wp_graphics_rtt_ops *ops, void *context)
{
    wp_rtt_status previous;
    wp_graphics_rtt_desc next_desc;
    wp_graphics_rtt_ops next_ops;
    if(!job) return WP_RTT_INVALID;
    if(job->status == WP_RTT_RUNNING) return WP_RTT_BUSY;
    if(!desc || !ops || !ops->validate || !ops->begin || !ops->draw || !ops->finish ||
       (!!ops->retain != !!ops->release) || desc->width <= 0 || desc->height <= 0 ||
       desc->viewport.x < 0 || desc->viewport.y < 0 ||
       desc->viewport.width <= 0 || desc->viewport.height <= 0 ||
       desc->viewport.width > desc->width || desc->viewport.height > desc->height ||
       desc->viewport.x > desc->width - desc->viewport.width ||
       desc->viewport.y > desc->height - desc->viewport.height) return WP_RTT_INVALID;
    next_desc = *desc; next_ops = *ops;
    previous = job->status;
    job->status = WP_RTT_RUNNING; /* protect ownership callbacks from reentry */
    if(next_ops.retain && !next_ops.retain(context)) {
        job->status = previous;
        return WP_RTT_BACKEND_FAILED;
    }
    if(job->ops.release) job->ops.release(job->context);
    job->desc = next_desc; job->ops = next_ops; job->context = context;
    job->cancelled = 0; job->status = WP_RTT_READY; job->result = WP_RTT_NOT_READY;
    return WP_RTT_OK;
}

wp_rtt_result wp_graphics_rtt_job_execute(wp_graphics_rtt_job *job)
{
    wp_rtt_result result, finish;
    if(!job) return WP_RTT_INVALID;
    if(job->status == WP_RTT_RUNNING) return WP_RTT_BUSY;
    if(job->status != WP_RTT_READY) return WP_RTT_INVALID;
    job->status = WP_RTT_RUNNING;
    result = job->ops.dependency ? job->ops.dependency(job->context, &job->desc) : WP_RTT_OK;
    if(job->cancelled) result = WP_RTT_CANCEL;
    if(result == WP_RTT_NOT_READY) {
        job->status = WP_RTT_READY; job->result = result; return result;
    }
    if(result == WP_RTT_OK) result = job->ops.validate(job->context, &job->desc);
    if(job->cancelled) result = WP_RTT_CANCEL;
    if(result == WP_RTT_OK) {
        result = job->ops.begin(job->context, &job->desc);
        if(job->cancelled) result = WP_RTT_CANCEL;
        if(result == WP_RTT_OK) result = job->ops.draw(job->context, &job->desc);
        if(job->cancelled) result = WP_RTT_CANCEL;
        if(result == WP_RTT_OK) result = job->ops.validate(job->context, &job->desc);
        if(job->cancelled) result = WP_RTT_CANCEL;
        job->finishing = 1; /* publication/restore is the non-cancellable boundary */
        finish = job->ops.finish(job->context, result == WP_RTT_OK);
        job->finishing = 0;
        if(result == WP_RTT_OK) result = finish;
    }
    job->result = result;
    job->status = result == WP_RTT_OK ? WP_RTT_SUCCEEDED :
                  result == WP_RTT_CANCEL ? WP_RTT_CANCELLED : WP_RTT_FAILED;
    return result;
}

wp_rtt_result wp_graphics_rtt_job_cancel(wp_graphics_rtt_job *job)
{
    if(!job) return WP_RTT_INVALID;
    if(job->finishing) return WP_RTT_BUSY;
    if(job->status != WP_RTT_READY && job->status != WP_RTT_RUNNING) return WP_RTT_INVALID;
    job->cancelled = 1;
    if(job->status == WP_RTT_READY) { job->status = WP_RTT_CANCELLED; job->result = WP_RTT_CANCEL; }
    return WP_RTT_OK;
}

wp_rtt_status wp_graphics_rtt_job_status(const wp_graphics_rtt_job *job)
{ 
    return job ? job->status : WP_RTT_IDLE; 
}

wp_rtt_result wp_graphics_rtt_job_result(const wp_graphics_rtt_job *job)
{ 
    return job ? job->result : WP_RTT_INVALID; 
}

wp_rtt_result wp_graphics_rtt_job_destroy(wp_graphics_rtt_job *job)
{
    if(!job) return WP_RTT_OK;
    if(job->status == WP_RTT_RUNNING) return WP_RTT_BUSY;
    job->status = WP_RTT_RUNNING;
    if(job->ops.release) job->ops.release(job->context);
    free(job); return WP_RTT_OK;
}
