
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
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

    lout_s = (struct stream*)calloc(1, sizeof(struct stream));
    if (lout_s == NULL)
    {
        return 1;
    }
    bytes = (int)(out_s->end - out_s->data);
    if ((bytes < 1) || (bytes > 1024 * 1024))
    {
        free(lout_s);
        return 2;
    }
    lout_s->data = (char*)malloc(bytes);
    if (lout_s->data == NULL)
    {
        free(lout_s);
        return 3;
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
    wtv_check_write(winfo);
    return 0;
}

/*****************************************************************************/
int
wtv_check_audio(struct wtv_info* winfo)
{
    struct stream* audio_s;
    int bytes;
    int data_bytes_processed;

    //printf("wtv_check_audio:\n");
    audio_s = winfo->audio_head;
    if (audio_s != NULL)
    {
        bytes = audio_s->end - audio_s->p;
        if (wtv_pa_play_non_blocking(winfo->pa, audio_s->p, bytes,
                                     &data_bytes_processed) == 0)
        {
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
    }
    if (winfo->audio_head != NULL)
    {
        wtv_sched_audio(winfo);
    }
    return 0;
}

/*****************************************************************************/
static int
wtv_process_msg_audio(struct wtv_info* winfo)
{
    int pts;
    int dts;
    int channels;
    int bytes;
    int data_bytes_processed;
    int format;
    struct stream* in_s;
    struct stream* out_s;
    struct stream* audio_s;

    in_s = winfo->in_s;
    if (!s_check_rem(in_s, 16))
    {
        return 1;
    }
    in_uint32_le(in_s, pts);
    in_uint32_le(in_s, dts);
    in_uint32_le(in_s, channels);
    in_uint32_le(in_s, bytes);
    if (!s_check_rem(in_s, bytes))
    {
        return 2;
    }
    //printf("wtv_process_msg: audio data, pts %d dts %d "
    //       "channels %d bytes %d\n", pts, dts, channels, bytes);wtv_sched_audio
    if (winfo->pa == NULL)
    {
        if (wtv_pa_init("wtv_viewer", &(winfo->pa)) != 0)
        {
            return 3;
        }
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
        if (wtv_pa_start(winfo->pa, "wtv_viewer", 1000, format) != 0)
        {
            return 4;
        }
    }
    if (winfo->pa != NULL)
    {
#if 1
        if (winfo->audio_bytes < 32 * 1024)
        {
            audio_s = calloc(1, sizeof(struct stream));
            audio_s->size = bytes;
            audio_s->data = (char*)malloc(audio_s->size);
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
            wtv_check_audio(winfo);
        }
        else
        {
            printf("wtv_process_msg_audio: dropping audio data\n");
        }
        //printf("winfo->audio_bytes %d\n", winfo->audio_bytes);
#else
        //wtv_pa_play(winfo->pa, in_s->p, bytes);
        if (wtv_pa_play_non_blocking(winfo->pa, in_s->p, bytes,
                                     &data_bytes_processed) != 0)
        {
            printf("error\n");
        }
        if (data_bytes_processed != bytes)
        {
            printf("lost bytes\n");
        }
#endif
    }
    return 0;
}

/*****************************************************************************/
int
read_fd(int sck, int *fd)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    union
    {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof (int))];
    } cmsgu;
    struct cmsghdr *cmsg;
    char text[4];
    int *fds;

    iov.iov_base = text;
    iov.iov_len = 4;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);
    size = recvmsg(sck, &msg, 0);
    if (size < 4)
    {
        return 1;
    }
    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)))
    {
        if (cmsg->cmsg_level != SOL_SOCKET)
        {
            return 1;
        }
        if (cmsg->cmsg_type != SCM_RIGHTS)
        {
            return 1;
        }
        fds = (int *) CMSG_DATA(cmsg);
        *fd = *fds;
        return 0;
    }
    return 1;
}

/*****************************************************************************/
static int
wtv_process_msg_video(struct wtv_info* winfo)
{
    int pts;
    int dts;
    int fd;
    int fd_width;
    int fd_height;
    int fd_stride;
    int fd_size;
    int fd_bpp;
    int rv;
    struct stream* in_s;

    in_s = winfo->in_s;
    if (!s_check_rem(in_s, 32))
    {
        return 1;
    }
    in_uint32_le(in_s, pts);
    in_uint32_le(in_s, dts);
    in_uint32_le(in_s, fd);
    in_uint32_le(in_s, fd_width);
    in_uint32_le(in_s, fd_height);
    in_uint32_le(in_s, fd_stride);
    in_uint32_le(in_s, fd_size);
    in_uint32_le(in_s, fd_bpp);
    //printf("wtv_process_msg_video: fd %d fd_width %d fd_height %d "
    //       "fd_stride %d fd_size %d fd_bpp %d\n",
    //       fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp);

    rv = read_fd(winfo->sck, &fd);
    //printf("rv %d fd %d\n", rv, fd);
    if (rv == 0)
    {
        wtv_fd_to_drawable(winfo, fd, fd_width, fd_height, fd_stride, fd_size, fd_bpp);
        close(fd);
    }
    return 0;
}

/*****************************************************************************/
static int
wtv_process_msg(struct wtv_info* winfo)
{
    int code;
    int rv;
    struct stream* in_s;

    rv = 0;
    in_s = winfo->in_s;
    in_uint32_le(in_s, code);
    in_uint8s(in_s, 4); /* bytes */
    //printf("wtv_process_msg: code %d\n", code);
    switch (code)
    {
        case 2:
            rv = wtv_process_msg_audio(winfo);
            break;
        case 4:
            rv = wtv_process_msg_video(winfo);
            break;
        default:
            printf("wtv_process_msg: unknown code %d\n", code);
            break;
    }
    return rv;
}

/*****************************************************************************/
const char *
get_filename(char* filename, int bytes)
{
    DIR * ldir;
    struct dirent * entry;
    int count;

    count = 0;
    ldir = opendir("/tmp");
    if (ldir != NULL)
    {
        entry = readdir(ldir);
        while (entry != NULL)
        {
            if (strncmp(entry->d_name, "wtv_", 3) == 0)
            {
                if (entry->d_type == DT_SOCK)
                {
                    snprintf(filename, bytes, "/tmp/%s", entry->d_name);
                    count++;
                }
            }
            entry = readdir(ldir);
        }
        closedir(ldir);
    }
    if (count == 1)
    {
        return filename;
    }
    return NULL;
}

/*****************************************************************************/
int
wtv_connect_to_uds(const char* filename)
{
    struct sockaddr_un s;
    int sck;
    char lfilename[256];

    if (filename == NULL)
    {
        filename = get_filename(lfilename, 255);
    }
    if (filename == NULL)
    {
        return -1;
    }
    printf("wtv_connect_to_uds: connecting to %s\n", filename);
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
    struct stream* out_s;

    out_s = (struct stream*)calloc(1, sizeof(struct stream));
    out_s->data = (char*)malloc(1024 * 1024);
    out_s->p = out_s->data;
    out_uint32_le(out_s, 1); /* subscribe audio */
    out_uint32_le(out_s, 9);
    out_uint8(out_s, 1); /* on */
    out_s->end = out_s->p;
    if (wtv_out_stream(winfo, out_s) != 0)
    {
        printf("wtv_start: wtv_out_stream failed\n");
    }
    free(out_s->data);
    free(out_s);
    return 0;
}

/*****************************************************************************/
int
wtv_request_frame(struct wtv_info* winfo)
{
    struct stream* out_s;

    out_s = (struct stream*)calloc(1, sizeof(struct stream));
    out_s->data = (char*)malloc(1024 * 1024);
    out_s->p = out_s->data;
    out_uint32_le(out_s, 3); /* request video frame */
    out_uint32_le(out_s, 8);
    out_s->end = out_s->p;
    if (wtv_out_stream(winfo, out_s) != 0)
    {
        printf("wtv_request_frame: wtv_out_stream failed\n");
    }
    free(out_s->data);
    free(out_s);
    return 0;
}

/*****************************************************************************/
int
wtv_read(struct wtv_info* winfo)
{
    //char mybuf[1024];
    int rv;
    int toread;
    //int code;
    int bytes;
    int sck;

    sck = winfo->sck;

    //printf("wtv_read:\n");
    if (winfo->in_s == NULL)
    {
        //printf("wtv_read: 1\n");
        winfo->in_s = (struct stream*)calloc(1, sizeof(struct stream));
        winfo->in_s->data = (char*)malloc(1024 * 1024);
        winfo->in_s->p = winfo->in_s->data;
        winfo->in_s->end = winfo->in_s->data + 8;
    }
    toread = winfo->in_s->end - winfo->in_s->p;
    //printf("toread %d\n", toread);
    rv = recv(sck, winfo->in_s->p, toread, 0);
    if (rv < 1)
    {
        return 1;
    }
    winfo->in_s->p += rv;
    //printf("wtv_read: recv rv %d\n", rv);
    if (winfo->in_s->p == winfo->in_s->end)
    {
        if (winfo->in_s->p == winfo->in_s->data + 8)
        {
            winfo->in_s->p = winfo->in_s->data;
            in_uint8s(winfo->in_s, 4); /* code */
            in_uint32_le(winfo->in_s, bytes);
            if ((bytes < 8) || (bytes > 1024 * 1024))
            {
                printf("wtv_read: bad bytes %d\n", bytes);
                return 1;
            }
            winfo->in_s->end = winfo->in_s->data + bytes;
        }
    }
    rv = 0;
    if (winfo->in_s->p == winfo->in_s->end)
    {
        //printf("got all\n");
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

    //printf("wtv_write:\n");
    out_s = winfo->out_s_head;
    if (out_s != NULL)
    {
        out_bytes = (int)(out_s->end - out_s->p);
        sent = send(sck, out_s->p, out_bytes, 0);
        if (sent < 1)
        {
            /* error */
            //LOGLN0((LOG_ERROR, LOGS "failed failed", LOGP));
            //hdhrd_peer_remove_one(hdhrd, &peer, &last_peer);
            //continue;
            return 1;
        }
        else
        {
            //LOGLN10((LOG_DEBUG, LOGS "send ok, sent %d", LOGP, sent));
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
    return 0;
}

/*****************************************************************************/
int
get_mstime(int* mstime)
{
    struct timespec ts;
    int the_tick;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 1;
    }
    the_tick = ts.tv_nsec / 1000000;
    the_tick += ts.tv_sec * 1000;
    *mstime = the_tick;
    return 0;
}

/*****************************************************************************/
int
wtv_print_stats(struct wtv_info* winfo)
{
    wtv_pa_print_stats(winfo->pa);
    return 0;
}

