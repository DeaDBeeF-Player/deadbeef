#ifndef metadata_h
#define metadata_h

#include <deadbeef/deadbeef.h>
#include <stdio.h>
#include <dispatch/dispatch.h>

typedef enum ddb_keyValueFlags_e {
    DDB_KV_BINARY_VALUE = 1,
} ddb_keyValueFlags_t;

typedef struct ddb_keyValue_s {
    struct ddb_keyValue_s *next;
    uint64_t itemIndex;
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

typedef void *ddb_keyValueDataStream_t;

typedef struct ddb_keyValueDataStreamInterface_s {
    ddb_keyValueDataStream_t (*open) (const char *filename, int write);
    int (*close) (ddb_keyValueDataStream_t);
    ssize_t (*read) (ddb_keyValueDataStream_t stream, void *ptr, size_t size);
    ssize_t (*write) (ddb_keyValueDataStream_t stream, void *ptr, size_t size);
    off_t (*seek) (ddb_keyValueDataStream_t stream, off_t offset, int whence);
    off_t (*tell) (ddb_keyValueDataStream_t stream); // could be implemented via `lseek(fd, 0, SEEK_CUR)`
    int (*truncate) (ddb_keyValueDataStream_t stream, off_t length);
} ddb_keyValueDataStreamInterface_t;

enum {
    DDB_KEYVALUE_HASH_SIZE = 1024
};

typedef struct ddb_keyValueItem_s {
    struct ddb_keyValueItem_s *hashNext;
    struct ddb_keyValueItem_s *listNext;
    uint64_t index;
    const char *name; // expected to be composed of Album:Artist:Dics:Track:Title
    ddb_keyValue_t *keyValues;
} ddb_keyValueHashItem_t;

/// Thread-safe unordered set of key-values
typedef struct {
    ddb_keyValueHashItem_t *itemHash[DDB_KEYVALUE_HASH_SIZE]; // only access in data_queue
    ddb_keyValueHashItem_t *itemListHead; // only access in data_queue
    ddb_keyValueHashItem_t *itemListTail; // only access in data_queue
    uint64_t nextItemIndex;

    /// IO Queue
    char *filename;
    ddb_keyValueDataStreamInterface_t *dataStreamInterface;
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
md_init_with_filename (ddb_keyValueList_t *md, const char *filename, ddb_keyValueDataStreamInterface_t *dataStreamInterface);

void
md_deinit (ddb_keyValueList_t *md);

void
md_free (ddb_keyValueList_t *md);

void
md_add_with_size (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value, size_t valuesize);

void
md_append_value (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value);

void
md_append_with_size (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value, size_t valuesize);

/// Returns `value` if found, otherwise NULL
const char *
md_find_value (ddb_keyValueList_t *md, const char *itemName, const char *key, char *value, size_t valuesize);

int
md_find_value_int (ddb_keyValueList_t *md, const char *itemName, const char *key, int def);

int64_t
md_find_value_int64 (ddb_keyValueList_t *md, const char *itemName, const char *key, int64_t def);

float
md_find_value_float (ddb_keyValueList_t *md, const char *itemName, const char *key, float def);

// FIXME: need a version of this method with valuesize
void
md_replace_value (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value);

void
md_set_value_int (ddb_keyValueList_t *md, const char *itemName, const char *key, int value);

void
md_set_value_int64 (ddb_keyValueList_t *md, const char *itemName, const char *key, int64_t value);

void
md_set_value_float (ddb_keyValueList_t *md, const char *itemName, const char *key, float value);

void
md_delete_value (ddb_keyValueList_t *md, const char *itemName, const char *key);

void
md_delete_all_values (ddb_keyValueList_t *md);

int
md_value_exists (ddb_keyValueList_t *md, const char *itemName, const char *key);

#endif /* metadata_h */
