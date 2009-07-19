#define CHANNEL_MONO   1
#define CHANNEL_STEREO 2

#define RESAMPLING_ALIASING  0
#define RESAMPLING_LINEAR    1
#define RESAMPLING_LINEAR2   2
#define RESAMPLING_QUADRATIC 3
#define RESAMPLING_CUBIC     4

#define PRIORITY_NORMAL  0
#define PRIORITY_HIGH    1
#define PRIORITY_HIGHEST 2


extern void config(HWND hwndParent);
extern void about(HWND hwndParent);

extern void config_init(void);
extern void config_quit(void);

extern int config_bits_per_sample;
extern int config_frequency;
extern int config_stereo;
extern int config_resampling;
extern int config_buffer_size;
extern int config_thread_priority;
