
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "wtv_calls.h"
#include "parse.h"
#include "wtv_pa.h"
#include "wtv_xcb.h"

/*****************************************************************************/
static int
wtv_out_stream(struct wtv_info* winfo, struct stream* out_s)
{
    int bytes;
    struct stream* lout_s;

    lout_s = xnew0(struct stream, 1);
    if (lout_s == NULL)
    {
        return WTV_ERROR_MEMORY;
    }
    bytes = (int)(out_s->end - out_s->data);
    if ((bytes < 1) || (bytes > 1024 * 1024))
    {
        free(lout_s);
        return WTV_ERROR_PARAMETER;
    }
    lout_s->data = xnew(char, bytes);
    if (lout_s->data == NULL)
    {
        free(lout_s);
        return WTV_ERROR_MEMORY;
    }
    lout_s->p = lout_s->data;
    out_uint8p(lout_s, out_s->data, bytes);
    lout_s->end = lout_s->p;
    lout_s->p = lout_s->data;
    if (winfo->out_s_tail == NULL)
    {
        winfo->out_s_head = lout_s;
        winfo->out_s_tail = lout_s;
    }
    else
    {
        winfo->out_s_tail->next = lout_s;
        winfo->out_s_tail = lout_s;
    }
    return wtv_gui_sched_write(winfo);
}

/*****************************************************************************/
int
wtv_check_audio(struct wtv_info* winfo)
{
    struct stream* audio_s;
    int bytes;
    int data_bytes_processed;

    LOGLN10((wtv, LOG_INFO, LOGS, LOGP));
    for (;;)
    {
        audio_s = winfo->audio_head;
        if (audio_s == NULL)
        {
            break;
        }
        bytes = audio_s->end - audio_s->p;
        if (wtv_pa_play_non_blocking(winfo->pa, audio_s->p, bytes,
                                     &data_bytes_processed) == 0)
        {
            LOGLN10((winfo, LOG_INFO, LOGS "data_bytes_processed %d", LOGP,
                     data_bytes_processed));
            if (data_bytes_processed < 1)
            {
                break;
            }
            audio_s->p += data_bytes_processed;
            if (audio_s->p >= audio_s->end)
            {
                if (audio_s->next == NULL)
                {
                    winfo->audio_head = NULL;
                    winfo->audio_tail = NULL;
                }
                else
                {
                    winfo->audio_head = winfo->audio_head->next;
                }
                free(audio_s->data);
                free(audio_s);
            }
            winfo->audio_bytes -= data_bytes_processed;
        }
        else
        {
            break;
        }
    }
    if (winfo->audio_head != NULL)
    {
        wtv_gui_sched_audio(winfo);
    }
    return WTV_ERROR_NONE;
}

/*****************************************************************************/
static int
wtv_process_msg_audio(struct wtv_info* winfo)
{
    int channels;
    int bytes;
    int format;
    struct stream* in_s;
    struct stream* audio_s;

    LOGLN10((winfo, LOG_INFO, LOGS, LOGP));
    in_s = winfo->in_s;
    if (!s_check_rem(in_s, 16))
    {
        return 1;
    }
    in_uint8s(in_s, 4); /* pts */
    in_uint8s(in_s, 4); /* dts */
    in_uint32_le(in_s, channels);
    in_uint32_le(in_s, bytes);
    if (!s_check_rem(in_s, bytes))
    {
        return 2;
    }
    if (winfo->pa == NULL)
    {
        if (wtv_pa_create("wtv_viewer", &(winfo->pa)) != 0)
        {
            return 3;
        }
    }
    LOGLN10((winfo, LOG_INFO, LOGS "is_audio_playing %d", LOGP,
             winfo->is_audio_playing));
    if (winfo->is_audio_playing == 0)
    {
        format = CAP_PA_FORMAT_48000_6CH_16LE;
        switch (channels)
        {
            case 1:
                format = CAP_PA_FORMAT_48000_1CH_16LE;
                break;
            case 2:
                format = CAP_PA_FORMAT_48000_2CH_16LE;
                break;
        }
        LOGLN0((winfo, LOG_INFO, LOGS "starting audio", LOGP));
        if (wtv_pa_start(winfo->pa, "wtv_viewer", winfo->ms_latency,
                         format) != 0)
        {
            LOGLN0((winfo, LOG_ERROR, LOGS "wtv_pa_start failed", LOGP));
            return 4;
        }
        if (wtv_pa_set_volume(winfo->pa, winfo->volume) != 0)
        {
            LOGLN0((winfo, LOG_ERROR, LOGS "wtv_pa_set_volume failed", LOGP));
        }
        LOGLN0((winfo, LOG_INFO, LOGS "audio started", LOGP));
        winfo->is_audio_playing = 1;
    }
    if (winfo->pa != NULL)
    {
        if (winfo->audio_bytes < channels * 16 * 1024)
        {
            audio_s = xnew0(struct stream, 1);
            if (audio_s == NULL)
            {
                LOGLN0((winfo, LOG_ERROR, LOGS "xnew0 failed", LOGP));
                return 5;
            }
            audio_s->size = bytes;
            audio_s->data = xnew(char, audio_s->size);
            if (audio_s->data == NULL)
            {
                LOGLN0((winfo, LOG_ERROR, LOGS "xnew failed", LOGP));
                free(audio_s);
                return 6;
            }
            audio_s->p = audio_s->data;
            out_uint8p(audio_s, in_s->p, bytes);
            audio_s->end = audio_s->p;
            audio_s->p = audio_s->data;
            if (winfo->audio_tail == NULL)
            {
                winfo->audio_head = audio_s;
                winfo->audio_tail = audio_s;
            }
            else
            {
                winfo->audio_tail->next = audio_s;
                winfo->audio_tail = audio_s;
            }
            winfo->audio_bytes += bytes;
        }
        else
        {
            LOGLN0((winfo, LOG_INFO, LOGS "dropping audio data audio_bytes %d",
                    LOGP, winfo->audio_bytes));
        }
        wtv_check_audio(winfo);
    }
    return 0;
}

/*****************************************************************************/
int
read_fd(int sck, int* fd)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    char control[CMSG_SPACE(sizeof(int))];
    struct cmsghdr* cmsg;
    char text[4];
    int* fds;

    memset(&iov, 0, sizeof(iov));
    iov.iov_base = text;
    iov.iov_len = 4;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    memset(control, 0, sizeof(control));
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    size = recvmsg(sck, &msg, 0);
    if (size == 4)
    {
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg != NULL)
        {
            if (cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
            {
                if (cmsg->cmsg_level == SOL_SOCKET)
                {
                    if (cmsg->cmsg_type == SCM_RIGHTS)
                    {
                        fds = (int*)CMSG_DATA(cmsg);
                        *fd = *fds;
                        return WTV_ERROR_NONE;
                    }
                }
            }
        }
    }
    return WTV_ERROR_SOCKET;
}

/*****************************************************************************/
static int
wtv_process_msg_video(struct wtv_info* winfo)
{
    int fd;
    int fd_width;
    int fd_height;
    int fd_stride;
    int fd_size;
    int fd_bpp;
    int error;
    struct stream* in_s;

    in_s = winfo->in_s;
    if (!s_check_rem(in_s, 32))
    {
        return WTV_ERROR_STREAM;
    }
    in_uint8s(in_s, 4); /* pts */
    in_uint8s(in_s, 4); /* dts */
    in_uint32_le(in_s, fd);
    in_uint32_le(in_s, fd_width);
    in_uint32_le(in_s, fd_height);
    in_uint32_le(in_s, fd_stride);
    in_uint32_le(in_s, fd_size);
    in_uint32_le(in_s, fd_bpp);
    LOGLN10((winfo, LOG_INFO, LOGS "fd %d fd_width %d fd_height %d "
             "fd_stride %d fd_size %d fd_bpp %d", LOGP,
             fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp));
    error = read_fd(winfo->sck, &fd);
    if (error == WTV_ERROR_NONE)
    {
        wtv_fd_to_drawable(winfo, fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp);
        close(fd);
        wtv_gui_draw_drawable(winfo);
    }
    return error;
}

/*****************************************************************************/
static int
wtv_process_msg_version(struct wtv_info* winfo)
{
    int version_major;
    int version_minor;
    struct stream* in_s;

    in_s = winfo->in_s;
    if (!s_check_rem(in_s, 12))
    {
        return WTV_ERROR_STREAM;
    }
    in_uint32_le(in_s, version_major);
    in_uint32_le(in_s, version_minor);
    in_uint32_le(in_s, winfo->ms_latency);
    LOGLN0((winfo, LOG_INFO, LOGS "version_major %d version_minor %d "
            "latency %d", LOGP, version_major, version_minor,
            winfo->ms_latency));
    return WTV_ERROR_NONE;
}

/*****************************************************************************/
static int
wtv_process_msg(struct wtv_info* winfo)
{
    int code;
    int rv;
    struct stream* in_s;

    rv = WTV_ERROR_NONE;
    in_s = winfo->in_s;
    in_uint32_le(in_s, code);
    in_uint8s(in_s, 4); /* bytes */
    LOGLN10((winfo, LOG_INFO, LOGS "code %d", LOGP, code));
    switch (code)
    {
        case 2:
            rv = wtv_process_msg_audio(winfo);
            break;
        case 4:
            rv = wtv_process_msg_video(winfo);
            break;
        case 5:
            rv = wtv_process_msg_version(winfo);
            break;
        default:
            LOGLN0((winfo, LOG_ERROR, LOGS "unknown code %d", LOGP, code));
            break;
    }
    return rv;
}

/*****************************************************************************/
int
wtv_connect_to_uds(struct wtv_info* winfo, const char* filename)
{
    struct sockaddr_un s;
    int sck;

    if (filename == NULL)
    {
        return -1;
    }
    LOGLN0((winfo, LOG_INFO, LOGS "connecting to %s", LOGP, filename));
    sck = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sck == -1)
    {
        return -1;
    }
    memset(&s, 0, sizeof(struct sockaddr_un));
    s.sun_family = AF_UNIX;
    strncpy(s.sun_path, filename, sizeof(s.sun_path));
    s.sun_path[sizeof(s.sun_path) - 1] = 0;
    if (connect(sck, (struct sockaddr*)&s, sizeof(struct sockaddr_un)) != 0)
    {
        close(sck);
        return -1;
    }
    return sck;
}

/*****************************************************************************/
int
wtv_start(struct wtv_info* winfo)
{
    int error;
    struct stream* out_s;

    out_s = xnew0(struct stream, 1);
    if (out_s == NULL)
    {
        return WTV_ERROR_MEMORY;
    }
    out_s->data = xnew(char, 1024 * 1024);
    if (out_s->data == NULL)
    {
        free(out_s);
        return WTV_ERROR_MEMORY;
    }
    out_s->p = out_s->data;
    out_uint32_le(out_s, 1); /* subscribe audio */
    out_uint32_le(out_s, 9);
    out_uint8(out_s, 1); /* on */
    out_s->end = out_s->p;
    error = wtv_out_stream(winfo, out_s);
    if (error != 0)
    {
        LOGLN0((winfo, LOG_ERROR, LOGS "wtv_out_stream failed, error %d",
                LOGP, error));
    }
    free(out_s->data);
    free(out_s);
    return error;
}

/*****************************************************************************/
int
wtv_stop(struct wtv_info* winfo)
{
    struct stream* s;
    struct stream* s1;

    LOGLN10((winfo, LOG_INFO, LOGS, LOGP));
    s = winfo->out_s_head;
    while (s != NULL)
    {
        s1 = s;
        s = s->next;
        free(s1->data);
        free(s1);
    }
    winfo->out_s_head = NULL;
    winfo->out_s_tail = NULL;

    if (winfo->is_audio_playing)
    {
        LOGLN0((winfo, LOG_INFO, LOGS "stopping audio", LOGP));
        if (wtv_pa_stop(winfo->pa) != 0)
        {
            LOGLN0((winfo, LOG_ERROR, LOGS "wtv_pa_stop failed", LOGP));
        }
        LOGLN0((winfo, LOG_INFO, LOGS "audio stopped", LOGP));
        winfo->is_audio_playing = 0;
    }
    s = winfo->audio_head;
    while (s != NULL)
    {
        s1 = s;
        s = s->next;
        free(s1->data);
        free(s1);
    }
    winfo->audio_head = NULL;
    winfo->audio_tail = NULL;
    winfo->audio_bytes = 0;

    if (winfo->in_s != NULL)
    {
        free(winfo->in_s->data);
        free(winfo->in_s);
        winfo->in_s = NULL;
    }
    return WTV_ERROR_NONE;
}

/*****************************************************************************/
int
wtv_exit(struct wtv_info* winfo)
{
    wtv_stop(winfo);
    wtv_pa_delete(winfo->pa);
    winfo->pa = NULL;
    return 0;
}

/*****************************************************************************/
int
wtv_request_frame(struct wtv_info* winfo)
{
    int error;
    struct stream* out_s;

    out_s = xnew0(struct stream, 1);
    if (out_s == NULL)
    {
        return WTV_ERROR_MEMORY;
    }
    out_s->data = xnew(char, 1024 * 1024);
    if (out_s->data == NULL)
    {
        free(out_s);
        return WTV_ERROR_MEMORY;
    }
    out_s->p = out_s->data;
    out_uint32_le(out_s, 3); /* request video frame */
    out_uint32_le(out_s, 8);
    out_s->end = out_s->p;
    error = wtv_out_stream(winfo, out_s);
    if (error != WTV_ERROR_NONE)
    {
        LOGLN0((winfo, LOG_ERROR, LOGS "wtv_out_stream failed, error %d",
                LOGP, error));
    }
    free(out_s->data);
    free(out_s);
    return error;
}

/*****************************************************************************/
int
wtv_read(struct wtv_info* winfo)
{
    int rv;
    int toread;
    int bytes;
    int sck;

    sck = winfo->sck;
    LOGLN10((winfo, LOG_INFO, LOGS, LOGP));
    if (winfo->in_s == NULL)
    {
        winfo->in_s = xnew0(struct stream, 1);
        if (winfo->in_s == NULL)
        {
            return WTV_ERROR_MEMORY;
        }
        winfo->in_s->data = xnew(char, 1024 * 1024);
        if (winfo->in_s->data == NULL)
        {
            free(winfo->in_s);
            winfo->in_s = NULL;
            return WTV_ERROR_MEMORY;
        }
        winfo->in_s->p = winfo->in_s->data;
        winfo->in_s->end = winfo->in_s->data + 8;
    }
    toread = winfo->in_s->end - winfo->in_s->p;
    LOGLN10((winfo, LOG_INFO, LOGS "toread %d", LOGP, toread));
    rv = recv(sck, winfo->in_s->p, toread, 0);
    if (rv < 1)
    {
        return WTV_ERROR_RECV;
    }
    winfo->in_s->p += rv;
    LOGLN10((winfo, LOG_INFO, LOGS "recv rv %d", LOGP, rv));
    if (winfo->in_s->p == winfo->in_s->end)
    {
        if (winfo->in_s->p == winfo->in_s->data + 8)
        {
            winfo->in_s->p = winfo->in_s->data;
            in_uint8s(winfo->in_s, 4); /* code */
            in_uint32_le(winfo->in_s, bytes);
            if ((bytes < 8) || (bytes > 1024 * 1024))
            {
                LOGLN0((winfo, LOG_ERROR, LOGS "bad bytes %d", LOGP, bytes));
                return WTV_ERROR_PARAMETER;
            }
            winfo->in_s->end = winfo->in_s->data + bytes;
        }
    }
    rv = WTV_ERROR_NONE;
    if (winfo->in_s->p == winfo->in_s->end)
    {
        LOGLN10((winfo, LOG_INFO, LOGS "got all", LOGP));
        winfo->in_s->p = winfo->in_s->data;
        rv = wtv_process_msg(winfo);
        winfo->in_s->p = winfo->in_s->data;
        winfo->in_s->end = winfo->in_s->data + 8;
    }
    return rv;
}

/*****************************************************************************/
int
wtv_write(struct wtv_info* winfo)
{
    struct stream* out_s;
    int sent;
    int out_bytes;
    int sck;

    sck = winfo->sck;
    LOGLN10((winfo, LOG_INFO, LOGS, LOGP));
    out_s = winfo->out_s_head;
    if (out_s != NULL)
    {
        out_bytes = (int)(out_s->end - out_s->p);
        sent = send(sck, out_s->p, out_bytes, 0);
        if (sent < 1)
        {
            /* error */
            return WTV_ERROR_SEND;
        }
        else
        {
            LOGLN10((LOG_DEBUG, LOGS "send ok, sent %d", LOGP, sent));
            out_s->p += sent;
            if (out_s->p >= out_s->end)
            {
                if (out_s->next == NULL)
                {
                    winfo->out_s_head = NULL;
                    winfo->out_s_tail = NULL;
                }
                else
                {
                    winfo->out_s_head = out_s->next;
                }
                free(out_s->data);
                free(out_s);
            }
        }
    }
    return WTV_ERROR_NONE;
}

/*****************************************************************************/
int
get_mstime(int* mstime)
{
    struct timespec ts;
    int the_tick;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return WTV_ERROR_CLOCK;
    }
    the_tick = ts.tv_nsec / 1000000;
    the_tick += ts.tv_sec * 1000;
    *mstime = the_tick;
    return WTV_ERROR_NONE;
}

/*****************************************************************************/
int
wtv_print_stats(struct wtv_info* winfo)
{
    int latency;

    wtv_pa_get_latency(winfo->pa, &latency);
    LOGLN0((winfo, LOG_INFO, LOGS "left over audio_bytes %d pa latency %d",
            LOGP, winfo->audio_bytes, latency));
    return WTV_ERROR_NONE;
}

static int g_log_level = 4;

static const char g_log_pre[][8] =
{
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

/*****************************************************************************/
int
wtv_snprintf(char* buffer, size_t count, const char *format, ...)
{
    va_list ap;
    int rv;

    va_start(ap, format);
#if defined(_MSC_VER)
    rv = _vsnprintf_s(buffer, count, count, format, ap);
#else
    rv = vsnprintf(buffer, count, format, ap);
#endif
    va_end(ap);
    return rv;
}

/*****************************************************************************/
int
wtv_vsnprintf(char* buffer, size_t count, const char *format, va_list ap)
{
    int rv;

#if defined(_MSC_VER)
    rv = _vsnprintf_s(buffer, count, count, format, ap);
#else
    rv = vsnprintf(buffer, count, format, ap);
#endif
    return rv;
}

/*****************************************************************************/
int
wtv_logln(struct wtv_info* winfo, int log_level, const char* format, ...)
{
    va_list ap;
    char* log_line;
    int mstime;

    if (winfo == NULL)
    {
        return WTV_ERROR_NONE;
    }
    if (log_level < g_log_level)
    {
        log_line = xnew(char, 2048);
        va_start(ap, format);
        wtv_vsnprintf(log_line, 1024, format, ap);
        va_end(ap);
        get_mstime(&mstime);
        wtv_snprintf(log_line + 1024, 1024, "[%10.10u][%s]%s",
                     mstime, g_log_pre[log_level % 4], log_line);
        wtv_gui_writeln(winfo, log_line + 1024);
        free(log_line);
    }
    return WTV_ERROR_NONE;
}

/*****************************************************************************/
int
wtv_set_volume(struct wtv_info* winfo)
{
    int error;

    LOGLN10((winfo, LOG_INFO, LOGS "volume %d", LOGP, winfo->volume));
    error = wtv_pa_set_volume(winfo->pa, winfo->volume);
    if (error != WTV_ERROR_NONE)
    {
        LOGLN0((winfo, LOG_ERROR, LOGS "wtv_pa_set_volume failed error %d",
                LOGP, error));
        return error;
    }
    return WTV_ERROR_NONE;
}
