#ifndef metadata_h
#define metadata_h

#include "../deadbeef.h"
#include <stdio.h>
#include <dispatch/dispatch.h>

typedef enum ddb_keyValueFlags_e {
    DDB_KV_BINARY_VALUE = 1,
} ddb_keyValueFlags_t;

typedef struct ddb_keyValue_s {
    struct ddb_keyValue_s *next;
    const char *key;
    const char *value;
    size_t valuesize;
    uint64_t file_offs;
    uint64_t chunk_size;
    uint64_t flags;
} ddb_keyValue_t;

typedef enum ddb_keyValueOperationType_e {
    DDB_KV_OPERATION_TYPE_ADD = 1,
    DDB_KV_OPERATION_TYPE_UPDATE = 2,
    DDB_KV_OPERATION_TYPE_DELETE = 3,
} ddb_keyValueOperationType_t;

typedef struct ddb_keyValueIoOperation_s {
    struct ddb_keyValueIoOperation_s *next;
    ddb_keyValueOperationType_t type;
    ddb_keyValue_t *keyValue;
} ddb_keyValueIoOperation_t;

/// Thread-safe unordered set of key-values
typedef struct {
    /// keyvalue list
    ddb_keyValue_t *head; // only access in data_queue

    /// IO Queue
    char *filename;
    int multiple_io_operations_mode;
    ddb_keyValueIoOperation_t *io_operations; // only aссess in sync queue
    ddb_keyValueIoOperation_t *io_operations_tail;

    /// Dispatch queues
    dispatch_queue_t data_queue; // for accessing the `head` keyvalue list
    dispatch_queue_t io_operation_sync_queue; // for accessing `io_operations` list
    dispatch_queue_t io_operation_execution_queue; // for executing io operations
} ddb_keyValueList_t;

ddb_keyValueList_t *
md_alloc (void);

void
md_init (ddb_keyValueList_t *md);

void
md_deinit (ddb_keyValueList_t *md);

void
md_free (ddb_keyValueList_t *md);

void
md_add_with_size (ddb_keyValueList_t *md, const char *key, const char *value, size_t valuesize);

void
md_append_value (ddb_keyValueList_t *md, const char *key, const char *value);

void
md_append_with_size (ddb_keyValueList_t *md, const char *key, const char *value, size_t valuesize);

/// Returns `value` if found, otherwise NULL
const char *
md_find_value (ddb_keyValueList_t *md, const char *key, char *value, size_t valuesize);

int
md_find_value_int (ddb_keyValueList_t *md, const char *key, int def);

int64_t
md_find_value_int64 (ddb_keyValueList_t *md, const char *key, int64_t def);

float
md_find_value_float (ddb_keyValueList_t *md, const char *key, float def);

// FIXME: need a version of this method with valuesize
void
md_replace_value (ddb_keyValueList_t *md, const char *key, const char *value);

void
md_set_value_int (ddb_keyValueList_t *md, const char *key, int value);

void
md_set_value_int64 (ddb_keyValueList_t *md, const char *key, int64_t value);

void
md_set_value_float (ddb_keyValueList_t *md, const char *key, float value);

void
md_delete_value (ddb_keyValueList_t *md, const char *key);

void
md_delete_all_values (ddb_keyValueList_t *md);

int
md_value_exists (ddb_keyValueList_t *md, const char *key);

#endif /* metadata_h */
