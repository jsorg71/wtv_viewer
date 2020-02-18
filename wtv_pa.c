/**
 * pulseaudio calls
 *
 * Copyright 2015-2018 Jay Sorg <jay.sorg@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pulse/pulseaudio.h>

#include "wtv_pa.h"
//#include "hdhome_run_log.h"

struct wtv_pa
{
    pa_threaded_mainloop* pa_mainloop;
    pa_context* pa_context;
    pa_stream* pa_stream;
};

/******************************************************************************/
static void
wtv_pa_context_state_callback(pa_context* context, void* userdata)
{
    struct wtv_pa* self;
    pa_context_state_t state;

    self = (struct wtv_pa*)userdata;
    state = pa_context_get_state(self->pa_context);
    switch (state)
    {
        case PA_CONTEXT_READY:
            //LOGLN(0, (0, LOGF "PA_CONTEXT_READY", LOGP));
            pa_threaded_mainloop_signal(self->pa_mainloop, 0);
            break;

        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED:
            //LOGLN(0, (0, LOGF "PA_CONTEXT_FAILED/PA_CONTEXT_TERMINATED %d",
            //      LOGP, (int)state));
            pa_threaded_mainloop_signal(self->pa_mainloop, 0);
            break;

        default:
            //LOGLN(0, (0, LOGF "state %d", LOGP, (int)state));
            break;
    }
}

/******************************************************************************/
int
wtv_pa_init(const char* name, void** handle)
{
    struct wtv_pa* self;
    pa_mainloop_api* api;
    pa_context_state_t state;

    self = (struct wtv_pa*)malloc(sizeof(struct wtv_pa));
    memset(self, 0, sizeof(struct wtv_pa));
    self->pa_mainloop = pa_threaded_mainloop_new();
    if (self->pa_mainloop == 0)
    {
        //LOGLN(0, (0, LOGF "pa_threaded_mainloop_new failed", LOGP));
        return 1;
    }
    api = pa_threaded_mainloop_get_api(self->pa_mainloop);
    self->pa_context = pa_context_new(api, name);
    if (self->pa_context == 0)
    {
        //LOGLN(0, (0, LOGF "pa_context_new failed", LOGP));
        pa_threaded_mainloop_free(self->pa_mainloop);
        return 2;
    }
    pa_context_set_state_callback(self->pa_context,
                                  wtv_pa_context_state_callback, self);
    if (pa_context_connect(self->pa_context, NULL, PA_CONTEXT_NOFLAGS, NULL))
    {
        //LOGLN(0, (0, LOGF "pa_context_connect failed", LOGP));
        pa_context_unref(self->pa_context);
        pa_threaded_mainloop_free(self->pa_mainloop);
        return 3;
    }
    pa_threaded_mainloop_lock(self->pa_mainloop);
    if (pa_threaded_mainloop_start(self->pa_mainloop) < 0)
    {
        //LOGLN(0, (0, LOGF "pa_threaded_mainloop_start failed", LOGP));
        pa_threaded_mainloop_unlock(self->pa_mainloop);
        /* todo: cleanup */
        return 4;
    }
    while (1)
    {
        state = pa_context_get_state(self->pa_context);
        if (state == PA_CONTEXT_READY)
        {
            break;
        }
        if (!PA_CONTEXT_IS_GOOD(state))
        {
            //LOGLN(0, (0, LOGF "bad context state (%d)", LOGP,
            //       pa_context_errno(self->pa_context)));
            pa_threaded_mainloop_unlock(self->pa_mainloop);
            /* todo: cleanup */
            return 5;
        }
        pa_threaded_mainloop_wait(self->pa_mainloop);
    }
    pa_threaded_mainloop_unlock(self->pa_mainloop);
    *handle = self;
    return 0;
}

/******************************************************************************/
int
wtv_pa_deinit(void* handle)
{
    struct wtv_pa* self;

    self = (struct wtv_pa*)handle;
    if (self == 0)
    {
        return 0;
    }
    return 0;
}

/******************************************************************************/
static void
wtv_pa_stream_state_callback(pa_stream* stream, void* userdata)
{
    struct wtv_pa* self;
    pa_stream_state_t state;

    self = (struct wtv_pa*)userdata;
    state = pa_stream_get_state(self->pa_stream);
    switch (state)
    {
        case PA_STREAM_READY:
            //LOGLN(0, (0, LOGF "PA_STREAM_READY", LOGP));
            pa_threaded_mainloop_signal(self->pa_mainloop, 0);
            break;
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            //LOGLN(0, (0, LOGF "state %d", LOGP, (int)state));
            pa_threaded_mainloop_signal(self->pa_mainloop, 0);
            break;
        default:
            //LOGLN(0, (0, LOGF "state %d", LOGP, (int)state));
            break;
    }
}

/******************************************************************************/
static void
wtv_pa_stream_request_callback(pa_stream* stream, size_t length, void* userdata)
{
    struct wtv_pa* self;

    self = (struct wtv_pa*)userdata;
    pa_threaded_mainloop_signal(self->pa_mainloop, 0);
}

/******************************************************************************/
/* returns error */
int
wtv_pa_start(void* handle, const char* name, int ms_latency, int format)
{
    struct wtv_pa* self;
    pa_sample_spec sample_spec;
    pa_stream_flags_t flags;
    pa_buffer_attr buffer_attr;
    pa_buffer_attr* pbuffer_attr;
    pa_stream_state_t state;
    pa_channel_map channel_map;
    pa_channel_map* channel_map_p;

    self = (struct wtv_pa*)handle;
    channel_map_p = 0;
    memset(&sample_spec, 0, sizeof(sample_spec));
    switch (format)
    {
        case CAP_PA_FORMAT_48000_1CH_16LE:
            sample_spec.rate = 48000;
            sample_spec.channels = 1;
            sample_spec.format = PA_SAMPLE_S16LE;
            break;
        case CAP_PA_FORMAT_48000_2CH_16LE:
            sample_spec.rate = 48000;
            sample_spec.channels = 2;
            sample_spec.format = PA_SAMPLE_S16LE;
            break;
        case CAP_PA_FORMAT_48000_6CH_16LE:
            sample_spec.rate = 48000;
            sample_spec.channels = 6;
            sample_spec.format = PA_SAMPLE_S16LE;
            memset(&channel_map, 0, sizeof(channel_map));
            channel_map_p = &channel_map;
            channel_map.channels = 6;
#if 0
            channel_map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
            channel_map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
            channel_map.map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
            channel_map.map[3] = PA_CHANNEL_POSITION_REAR_LEFT;
            channel_map.map[4] = PA_CHANNEL_POSITION_REAR_RIGHT;
            channel_map.map[5] = PA_CHANNEL_POSITION_LFE;
#else
            channel_map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
            channel_map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
            channel_map.map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
            channel_map.map[3] = PA_CHANNEL_POSITION_LFE;
            channel_map.map[4] = PA_CHANNEL_POSITION_REAR_LEFT;
            channel_map.map[5] = PA_CHANNEL_POSITION_REAR_RIGHT;
#endif
            break;
        default:
            return 1;
    }
    if (pa_sample_spec_valid(&sample_spec) == 0)
    {
        return 2;
    }
    pa_threaded_mainloop_lock(self->pa_mainloop);
    self->pa_stream = pa_stream_new(self->pa_context, name, &sample_spec,
                                    channel_map_p);
    if (self->pa_stream == 0)
    {
        return 3;
    }
    /* install essential callbacks */
    pa_stream_set_state_callback(self->pa_stream,
                                 wtv_pa_stream_state_callback, self);
    pa_stream_set_write_callback(self->pa_stream,
                                 wtv_pa_stream_request_callback, self);
    flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE;
    pbuffer_attr = 0;
    if (ms_latency > 0)
    {
        pbuffer_attr = &buffer_attr;
        memset(&buffer_attr, 0, sizeof(buffer_attr));
        buffer_attr.maxlength =
            pa_usec_to_bytes(ms_latency * sample_spec.channels * 1000,
                             &sample_spec);
        buffer_attr.tlength = pa_usec_to_bytes(ms_latency * 1000, &sample_spec);
        buffer_attr.prebuf = (uint32_t) -1;
        buffer_attr.minreq = (uint32_t) -1;
        buffer_attr.fragsize = (uint32_t) -1;
        flags |= PA_STREAM_ADJUST_LATENCY;
    }
    if (pa_stream_connect_playback(self->pa_stream, 0, pbuffer_attr,
                                   flags, 0, 0) < 0)
    {
        /* todo: cleanup */
        self->pa_stream = 0;
        return 4;
    }
    while (1)
    {
        state = pa_stream_get_state(self->pa_stream);
        if (state == PA_STREAM_READY)
        {
            break;
        }
        if (!PA_STREAM_IS_GOOD(state))
        {
            //LOGLN(0, (0, LOGF "bad stream state (%d)", LOGP,
            //      pa_context_errno(self->pa_context)));
            /* todo: cleanup */
            self->pa_stream = 0;
            return 5;
        }
        pa_threaded_mainloop_wait(self->pa_mainloop);
    }
    pa_threaded_mainloop_unlock(self->pa_mainloop);
    return 0;
}

/******************************************************************************/
static void
wtv_pa_pulse_stream_success_callback(pa_stream* stream, int success,
                                     void* userdata)
{
    struct wtv_pa* self;

    self = (struct wtv_pa*)userdata;
    pa_threaded_mainloop_signal(self->pa_mainloop, 0);
}

/******************************************************************************/
int
wtv_pa_stop(void* handle)
{
    struct wtv_pa* self;
    pa_operation* operation;

    self = (struct wtv_pa*)handle;
    pa_threaded_mainloop_lock(self->pa_mainloop);
    operation = pa_stream_drain(self->pa_stream,
                                wtv_pa_pulse_stream_success_callback,
                                self);
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
    {
        pa_threaded_mainloop_wait(self->pa_mainloop);
    }
    pa_operation_unref(operation);
    pa_stream_disconnect(self->pa_stream);
    pa_stream_unref(self->pa_stream);
    self->pa_stream = 0;
    pa_threaded_mainloop_unlock(self->pa_mainloop);
    return 0;
}

/******************************************************************************/
int
wtv_pa_play(void* handle, void* data, int data_bytes)
{
    struct wtv_pa* self;
    int len;
    int ret;
    char* src;

    self = (struct wtv_pa*)handle;
    pa_threaded_mainloop_lock(self->pa_mainloop);
    src = (char*)data;
    while (data_bytes > 0)
    {
        while ((len = pa_stream_writable_size(self->pa_stream)) == 0)
        {
            pa_threaded_mainloop_wait(self->pa_mainloop);
        }
        if (len < 0)
        {
            //LOGLN(0, (0, LOGF "pa_stream_writable_size failed", LOGP));
            pa_threaded_mainloop_unlock(self->pa_mainloop);
            return 1;
        }
        if (len > data_bytes)
        {
            len = data_bytes;
        }
        ret = pa_stream_write(self->pa_stream, src, len, NULL, 0LL,
                              PA_SEEK_RELATIVE);
        if (ret < 0)
        {
            //LOGLN(0, (0, LOGF "pa_stream_write failed (%d)", LOGP,
            //       pa_context_errno(self->pa_context)));
            pa_threaded_mainloop_unlock(self->pa_mainloop);
            return 2;
        }
        src += len;
        data_bytes -= len;
    }
    pa_threaded_mainloop_unlock(self->pa_mainloop);
    return 0;
}

/******************************************************************************/
int
wtv_pa_play_non_blocking(void* handle, void* data, int data_bytes,
                         int* data_bytes_processed)
{
    struct wtv_pa* self;
    int len;
    int ret;
    int bytes_played;
    char* src;

    self = (struct wtv_pa*)handle;
    pa_threaded_mainloop_lock(self->pa_mainloop);

#if 0
    pa_usec_t latency;
    int negative;
    pa_stream_get_latency(self->pa_stream, &latency, &negative);
    printf("latency %ld negative %d\n", latency, negative);
#endif

    src = (char*)data;
    bytes_played = 0;
    if (data_bytes > 0)
    {
        len = pa_stream_writable_size(self->pa_stream);
        if (len < 0)
        {
            //LOGLN(0, (0, LOGF "pa_stream_writable_size failed", LOGP));
            pa_threaded_mainloop_unlock(self->pa_mainloop);
            return 1;
        }
        if (len > data_bytes)
        {
            len = data_bytes;
        }
        ret = pa_stream_write(self->pa_stream, src, len, NULL, 0LL,
                              PA_SEEK_RELATIVE);
        if (ret < 0)
        {
            //LOGLN(0, (0, LOGF "pa_stream_write failed (%d)", LOGP,
            //       pa_context_errno(self->pa_context)));
            pa_threaded_mainloop_unlock(self->pa_mainloop);
            return 2;
        }
        bytes_played += len;
    }
    pa_threaded_mainloop_unlock(self->pa_mainloop);
    if (data_bytes_processed != 0)
    {
        *data_bytes_processed = bytes_played;
    }
    return 0;
}

/******************************************************************************/
int
wtv_pa_print_stats(void* handle)
{
    struct wtv_pa* self;
    pa_usec_t latency;
    int negative;

    self = (struct wtv_pa*)handle;
    if (self != NULL)
    {
        if (self->pa_stream != NULL)
        {
            pa_stream_get_latency(self->pa_stream, &latency, &negative);
            printf("latency %ld negative %d\n", latency, negative);
        }
    }
    return 0;
}

