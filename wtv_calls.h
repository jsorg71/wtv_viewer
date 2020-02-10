
#if !defined(WTV_CALLS_H)
#define WTV_CALLS_H

struct wtv_info
{
    void* gui_obj;
    void* pa;
    struct stream* in_s;
    struct stream* out_s_head;
    struct stream* out_s_tail;
    int sck;
    int drawable;
    int drawable_width;
    int drawable_height;
    int pad0;
    int pict_format_default;
    void* xcb;
};

#ifdef __cplusplus
extern "C" {
#endif

int
wtv_connect_to_uds(const char* filename);
int
wtv_start(struct wtv_info* winfo);
int
wtv_request_frame(struct wtv_info* winfo);
int
wtv_read(struct wtv_info* winfo);
int
wtv_write(struct wtv_info* winfo);

/* in gui */
int
wtv_check_write(struct wtv_info* winfo);
int
get_mstime(int* mstime);;

#ifdef __cplusplus
}
#endif

#endif
