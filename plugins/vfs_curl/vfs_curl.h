//
//  vfs_curl.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 11/10/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#ifndef vfs_curl_h
#define vfs_curl_h

#include <curl/curl.h>
#include <deadbeef/deadbeef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_SIZE (0x10000)
#define BUFFER_MASK 0xffff

#define MAX_METADATA 1024

#define TIMEOUT 10 // in seconds

enum {
    STATUS_INITIAL  = 0,
    STATUS_READING  = 1,
    STATUS_FINISHED = 2,
    STATUS_ABORTED  = 3,
    STATUS_SEEK     = 4,
    STATUS_DESTROY  = 5,
};

typedef struct {
    DB_vfs_t *vfs;
    char *url;
    uint8_t buffer[BUFFER_SIZE];

    DB_playItem_t *track;
    int64_t pos; // position in stream; use "& BUFFER_MASK" to make it index into ringbuffer
    int64_t length;
    int32_t remaining; // remaining bytes in buffer read from stream
    int64_t skipbytes;
    intptr_t tid; // thread id which does http requests
    intptr_t mutex;
    uint8_t nheaderpackets;
    char *content_type;
    CURL *curl;
    struct timeval last_read_time;
    uint8_t status;
    int icy_metaint;
    int wait_meta;

    char metadata[MAX_METADATA];
    size_t metadata_size; // size of metadata in stream
    size_t metadata_have_size; // amount which is already in metadata buffer

    char http_err[CURL_ERROR_SIZE];

    float prev_playtime;
    time_t started_timestamp;

    uint64_t identifier;

    // flags (bitfields to save some space)
    unsigned seektoend : 1; // indicates that next tell must return length
    unsigned gotheader : 1; // tells that all headers (including ICY) were processed (to start reading body)
    unsigned icyheader : 1; // tells that we're currently reading ICY headers
    unsigned gotsomeheader : 1; // tells that we got some headers before body started
} HTTP_FILE;

size_t
vfs_curl_handle_icy_headers (size_t avail, HTTP_FILE *fp, const char *ptr);

void
vfs_curl_free_file (HTTP_FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* vfs_curl_h */
