/**
 * @ingroup   api68_devel
 * @file      api68/api68.h
 * @author    Benjamin Gerard <ben@sashipa.com>
 * @date      2003/08/07
 * @brief     sc68 API.
 *
 * $Id: api68.h,v 2.8 2003/09/30 06:29:57 benjihan Exp $
 */

#ifndef _API68_H_
#define _API68_H_

#ifdef __cplusplus
extern "C" {
#endif

  //#include "config.h"
#include "file68/istream68.h"
#include "file68/debugmsg68.h"

/** @defgroup  api68_api  sc68 main API
 *  @ingroup   api68_devel
 *
 *  This API provides functions to use sc68 libraries efficiently.
 *
 *  @par Multi-threading issue
 *
 *  The API is not thread safe. Currently the 68000 emulator does not
 *  handle multi instance. Anyway the API may be used in multi-thread
 *  context as soon as you take care to play only one disk at a time.
 *
 *  Have a look to the xmms68 package to see how to use this API in
 *  multi-thread context.
 *
 *  @par Quick start
 *
 *  @code
 *  #include "api68/api68.h"
 *
 *  api68_init_t init68;
 *  api68_t * sc68 = 0;
 *  char buffer[512*4];
 *
 *  // Clean up init structure (required).
 *  memset(&init68, 0, sizeof(init68));
 *  // Set dynamic handler (required).
 *  init68.alloc = malloc;
 *  init68.free = free;
 *  // Set debug message handler (optionnal).
 *  init68.debug = vfprintf;
 *  init68.debug_cookie = stderr;
 *  sc68 = api68_init(&init68);
 *  if (!sc68) goto error;
 *
 *  // Load an sc68 file.
 *  if (api68_load_file(sc68, fname)) {
 *    goto error;
 *  }
 *
 *  // Set a track (optionnal).
 *  api68_play(sc68, track);
 *
 *  // Loop until the end of disk. You can use API68_LOOP to wait the end
 *  // of the track. Notice that API68_ERROR set all bits and make the loop
 *  // break too.
 *  while ( ! (api68_process(sc68, buffer, sizeof(buffer) >> 2) & API68_END)) {
 *    // Do something with buffer[] here.
 *  }
 *
 *  // Stop current track (optionnal).
 *  api68_stop(sc68);
 *
 * error:
 *  // Shutdown everything (0 pointer could be sent safely).
 *  api68_shutdown(sc68);
 *
 * @endcode
 *
 *  @{
 */


/** API initialization.
 *
 *    The api68_init_t must be properly filled before calling the
 *    api68_init() function.
 *    
 * @code
 * api68_init_t init;
 * memset(&init,0,sizeof(init));
 * init.alloc = malloc;
 * init.free = free;
 * @endcode
 */
typedef struct {

  /** sampling rate in hz (non 0 value overrides config default).
   *  The real used value is set by api68_init().
   */
  unsigned int sampling_rate;

  /** dynamic memory allocation handler (malloc).
   *  @see SC68set_alloc().
   */
  void * (*alloc)(unsigned int); 

  /** dynamic memory free handler (free).
   *  @see SC68set_free().
   */
  void (*free)(void *);

  /** user resource path (0 default).
   *  @see SC68rsc_set_user().
   */
  const char * user_path;

  /** shared resource path (0 default).
   *  @see SC68rsc_set_shared().
   */
  const char * shared_path;

  /** debug message handler. */
  debugmsg68_t debug;

  /** debug cookie. */
  void * debug_cookie;

} api68_init_t;


/** Music information.
 *
 * @warning  Most string in this structure point on disk and must not be used
 *           after the api68_close() call.
 *
 */
typedef struct {
  int track;             /**< Track number (0:disk information).  */
  int tracks;            /**< Number of track.                    */
  const char * title;    /**< Disk or track title.                */
  const char * author;   /**< Author name.                        */
  const char * composer; /**< Composer name.                      */
  const char * replay;   /**< Replay name.                        */
  const char * hwname;   /**< Hardware description.               */
  char time[12];         /**< Time in format TT MM:SS.            */
  /** Hardware used. */
  struct {
    unsigned ym:1;        /**< Music uses YM-2149 (ST).           */
    unsigned ste:1;       /**< Music uses STE specific hardware.  */
    unsigned amiga:1;     /**< Music uses Paula Amiga hardware.   */
  } hw;
  unsigned int time_ms;   /**< Duration in ms.                    */
  unsigned int start_ms;  /**< Absolute start time in disk in ms. */
  unsigned int rate;      /**< Replay rate.                       */
  unsigned int addr;      /**< Load address.                      */
} api68_music_info_t;

/** API information. */
typedef struct _api68_s api68_t;

/** API disk. */
typedef void * api68_disk_t;

/** @name  Process status (as returned by api68_process() function)
 *  @{
 */

#define API68_IDLE_BIT   1 /**< Set if no emulation pass has been runned. */
#define API68_CHANGE_BIT 2 /**< Set when track has changed.               */
#define API68_LOOP_BIT   4 /**< Set when track has loop.                  */
#define API68_END_BIT    5 /**< Set when finish with all tracks.          */

#define API68_IDLE       (1<<API68_IDLE_BIT)   /**< @see API68_IDLE_BIT   */
#define API68_CHANGE     (1<<API68_CHANGE_BIT) /**< @see API68_CHANGE_BIT */
#define API68_LOOP       (1<<API68_LOOP_BIT)   /**< @see API68_LOOP_BIT   */
#define API68_END        (1<<API68_END_BIT)    /**< @see API68_END_BIT    */

#define API68_MIX_OK     0  /**< Not really used. */
#define API68_MIX_ERROR  -1 /**< Error.           */

/**@}*/

/** @name API control functions.
 *  @{
 */

/** Initialise sc68 API.
 *
 * @param  init  Initialization parameters.
 *
 * @return Pointer ti initialized API.
 * @retval 0  Error.
 *
 * @warning  Currently only one API can be initialized.
 */
api68_t * api68_init(api68_init_t * init);

/** Shutdown sc68 API.
 *
 * @param  api  sc68 api.
 *
 * @note  It is safe to call with null api.
 */
void api68_shutdown(api68_t * api);

/** Set/Get sampling rate.
 *
 * @param  api  sc68 api.
 * @param  f    New sampling rate in hz or 0 to read current.
 *
 * @return Sampling rate (could be diffrent from requested one).
 * @retval Error (not initialized).
 */
unsigned int api68_sampling_rate(api68_t * api, unsigned int f);

/** Set share data path.
 *
 * @param  api  sc68 api.
 * @param  path New shared data path.
 */
void api68_set_share(api68_t * api, const char * path);

/** Set user data path.
 *
 * @param  api  sc68 api.
 * @param  path New user data path.
 */
void api68_set_user(api68_t * api, const char * path);

/** Pop and return last stacked error message.
 *
 * @return  Error message
 * @retval  0  No stacked error message
 */
const char * api68_error(void);

/** Display debug message.
 *
 * @param  fmt  printf() like format string.
 *
 * @see debugmsg68()
 */
void api68_debug(const char * fmt, ...);

/**@}*/


/** @name Music control functions.
 *  @{
 */

/** Fill PCM buffer.
 *
 *    The api68_process() function fills the PCM buffer with the current
 *    music data. If the current track is finished and it is not the last
 *    the next one is automatically loaded. The function returns status
 *    value that report events that have occured during this pass.
 *
 * @param  api   sc68 api.
 * @param  buf   PCM buffer (must be at leat 4*n bytes).
 * @param  n     Number of sample to fill.
 *
 * @return Process status
 *
 */
int api68_process(api68_t * api, void * buf, int n);

/** Set/Get current track.
 *
 *   The api68_play() function get or set current track.
 *
 *   If track == -1 the function returns the current track or 0 if none.
 *
 *   Else the function will test the requested track number. If it is 0, the
 *   disk default track will be use. If the track is out of range, the function
 *   fails and returns -1 else it returns 0.
 *   To avoid multi-threading issus the track is not changed directly but
 *   a change-track event is posted. This ecvent will be processed at the
 *   next call to the api68_process() function.
 *
 * @param  api    sc68 api.
 * @param  track  track number [-1:read current, 0:set disk default]
 *
 * @return error code or track number.
 * @retval 0  Success or no current track
 * @retval >0 Current track 
 * @retval -1 Failure.
 *
 */
int api68_play(api68_t * api, int track);

/** Stop playing.
 *
 *     The api68_stop() function stop current playing track. Like the
 *     api68_play() function the api68_stop() function does not really
 *     stop the music but send a stop-event that will be processed by
 *     the next call to api68_process() function.
 *
 * @param  api    sc68 api.
 * @return error code
 * @retval 0  Success
 * @retval -1 Failure
 */
int api68_stop(api68_t * api);

/** Set/Get current play position.
 *
 *    The api68_seek() functions get or set the current play position.
 *
 *    If time_ms == -1 the function will returns the current play position
 *    or -1 if not currently playing.
 *
 *    If time_ms >= 0 the function will try to seek to the given position.
 *    If time_ms is out of range the function returns -1.
 *    If time_ms is inside the current playing track the function does not
 *    seek and returns -1.
 *    Else the function change to the track which time_ms belong and returns
 *    the time position at the beginning of this track.
 *
 *    The returned time is always the number of millisecond since the disk
 *    has started (not the track).
 *    
 * @param  api      sc68 api.
 * @param  time_ms  new time position in ms (-1:read current time).
 *
 * @return error code
 * @retval 0  Success
 * @retval -1 Failure
 */
int api68_seek(api68_t * api, int time_ms);

/** Get disk/track information.
 *
 * @param  api   sc68 api
 * @param  info  track/disk information structure to be filled.
 * @param  track track number (-1:current/default 0:disk-info).
 * @param  disk  disk to get information from (0 means API disk).
 *
 * @return error code
 * @retval 0  Success.
 * @retval -1 Failure.
 *
 * @warning API disk informations are valid as soon as the disk is loaded and
 *          must not be used after api_load() or api_close() function call.
 *          If disk was given the information are valid until the disk is
 *          freed.
 *          
 */
int api68_music_info(api68_t * api, api68_music_info_t * info, int track,
		     api68_disk_t disk);

/**@}*/


/** @name File functions.
 *  @{
 */

/** Verify an sc68 disk. */
int api68_verify(istream_t * is);
int api68_verify_file(const char * filename);
int api68_verify_mem(const void * buffer, int len);

/** Load an sc68 disk for playing. */
int api68_load(api68_t * api, istream_t * is);
int api68_load_file(api68_t * api, const char * filename);
int api68_load_mem(api68_t * api, const void * buffer, int len);

/** Load an sc68 disk outside the API. Free it with api68_free() function. */
api68_disk_t api68_load_disk(istream_t * is);
api68_disk_t api68_load_disk_file(const char * filename);
api68_disk_t api68_disk_load_mem(const void * buffer, int len);


/** Change current disk.
 *
 * @param  api   sc68 api
 * @param  disk  New disk (0 does a api68_close())
 *
 * @return error code
 * @retval 0  Success, disk has been loaded.
 * @retval -1 Failure, no disk has been loaded (occurs if disk was 0).
 *
 * @note    Can be safely call with null api.
 * @warning After api68_open() failure, the disk has been freed.
 * @warning Beware not to use disk information after api68_close() call
 *          because the disk should have been destroyed.
 */
int api68_open(api68_t * api, api68_disk_t disk);

/** Close current disk.
 *
 * @param  api  sc68 api
 *
 * @note   Can be safely call with null api or if no disk has been loaded.
 */
void api68_close(api68_t * api);

/** Get number of tracks.
 *
 * @param  api  sc68 api
 *
 * @return Number of track
 * @retval -1 error
 *
 * @note Could be use to check if a disk is loaded.
 */
int api68_tracks(api68_t * api);

/**@}*/


/** @name Configuration functions
 *  @{
 */

/** Load config file.
 *
 * @param  api  sc68 api
 */
int api68_config_load(api68_t * api);

/** Save config file.
 *
 * @param  api  sc68 api
 */
int api68_config_save(api68_t * api);

/** Get config variable idex.
 *
 * @param  api   sc68 api
 * @param  name  name of config variable
 *
 * @return  config index
 * @retval -1 Error
 */
int api68_config_id(api68_t * api, const char * name);

/** Get config variable value.
 *
 * @param  api  sc68 api
 * @param  idx  config index
 * @param  v    pointer to store config value
 *
 * @return      Error code
 * @retval  0   Success
 * @retval  -1  Failure
 */
int api68_config_get(api68_t * api, int idx, int * v);

/** Set config variable value.
 *
 * @param  api  sc68 api
 * @param  idx  config index
 * @param  v    new config value
 *
 * @return      Error code
 * @retval  0   Success
 * @retval  -1  Failure
 */
int api68_config_set(api68_t * api, int idx, int v);

/** Apply current configuration to api.
 *
 * @param  api  sc68 api
 */
void api68_config_apply(api68_t * api);

/**@}*/


/** @name Dynamic memory access.
 *  @{
 */

/** Allocate dynamic memory.
 *
 *   The api68_alloc() function calls the SC68alloc() function.
 *
 * @param  n  Size of buffer to allocate.
 *
 * @return pointer to allocated memory buffer.
 * @retval 0 error
 *
 * @see SC68alloc()
 *
 */
void * api68_alloc(unsigned int n);

/** Free dynamic memory.
 *
 *   The api68_free() function calls the SC68free() function.
 *
 * @param  data  Previously allocated memory buffer.
 *
 */
void api68_free(void * data);

/**@}*/

/** 
 *  @}
 */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _API68_H_ */
