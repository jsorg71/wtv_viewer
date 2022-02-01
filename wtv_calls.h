
#if !defined(WTV_CALLS_H)
#define WTV_CALLS_H

#define WTV_ERROR_NONE          0
#define WTV_ERROR_MEMORY        1
#define WTV_ERROR_PARAMETER     2
#define WTV_ERROR_STREAM        3
#define WTV_ERROR_SOCKET        4
#define WTV_ERROR_SEND          5
#define WTV_ERROR_RECV          6
#define WTV_ERROR_CLOCK         7

struct wtv_info
{
    void* gui_obj;
    void* pa;
    struct stream* in_s;
    struct stream* out_s_head;
    struct stream* out_s_tail;
    struct stream* audio_head;
    struct stream* audio_tail;
    int audio_bytes;
    int sck;
    int pict_format_default;
    int gc;
    unsigned int drawable_id;
    int drawable_width;
    int drawable_height;
    int ms_latency;
    int is_audio_playing;
    int volume; /* 0 - 100 */
    void* xcb;
};

#ifdef __cplusplus
extern "C" {
#endif

int
wtv_connect_to_uds(struct wtv_info* winfo, const char* filename);
int
wtv_start(struct wtv_info* winfo);
int
wtv_stop(struct wtv_info* winfo);
int
wtv_exit(struct wtv_info* winfo);
int
wtv_request_frame(struct wtv_info* winfo);
int
wtv_read(struct wtv_info* winfo);
int
wtv_write(struct wtv_info* winfo);
int
wtv_check_audio(struct wtv_info* winfo);
int
get_mstime(int* mstime);;
int
wtv_print_stats(struct wtv_info* winfo);
int
wtv_snprintf(char* buffer, size_t count, const char *format, ...);
int
wtv_vsnprintf(char* buffer, size_t count, const char *format, va_list ap);
int
wtv_logln(struct wtv_info* winfo, int log_level, const char* format, ...);
int
wtv_set_volume(struct wtv_info* winfo);

/* in gui */
int
wtv_gui_sched_write(struct wtv_info* winfo);
int
wtv_gui_sched_audio(struct wtv_info* winfo);
int
wtv_gui_writeln(struct wtv_info* winfo, const char* msg);
int
wtv_gui_draw_drawable(struct wtv_info* winfo);

#define LOG_ERROR 0
#define LOG_WARN  1
#define LOG_INFO  2
#define LOG_DEBUG 3

#define LOGS "[%s][%d][%s]:"
#define LOGP __FILE__, __LINE__, __FUNCTION__

#if !defined(__FUNCTION__) && defined(__FUNC__)
#define LOG_PRE const char* __FUNCTION__ = __FUNC__; (void)__FUNCTION__;
#else
#define LOG_PRE
#endif

#if !defined(LOG_LEVEL)
#define LOG_LEVEL 1
#endif
#if LOG_LEVEL > 0
#define LOGLN0(_args) do { LOG_PRE wtv_logln _args ; } while (0)
#else
#define LOGLN0(_args)
#endif
#if LOG_LEVEL > 10
#define LOGLN10(_args) do { LOG_PRE wtv_logln _args ; } while (0)
#else
#define LOGLN10(_args)
#endif

#define xnew(type_, count_) (type_ *) malloc(count_ * sizeof(type_))
#define xnew0(type_, count_) (type_ *) calloc(count_, sizeof(type_))

#ifdef __cplusplus
}
#endif

#endif
