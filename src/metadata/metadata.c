#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "metacache.h"
#include "metadata.h"
#include "threading.h"

#pragma mark - Util

#define min(x,y) ((x)<(y)?(x):(y))

#pragma mark - Private functions

static char *
_strip_empty (const char *value, size_t size, size_t *outsize) {
    char *data = malloc (size);
    if (!data) {
        return NULL;
    }

    *outsize = 0;
    const char *p = value;
    const char *e = value + size;
    char *out = data;
    while (p < e) {
        size_t l = strlen (p) + 1;
        if (l > 1) {
            memcpy (out, p, l);
            out += l;
            *outsize += l;
        }
        p += l;
    }

    return data;
}

static void
_meta_set_value (ddb_keyValue_t *m, const char *value, size_t size) {
    size_t len = strlen (value) + 1;
    if (len != size) {
        // multivalue -- need to strip empty parts
        size_t valuesize;
        char *data = _strip_empty (value, size, &valuesize);
        m->valuesize = (int)valuesize;

        if (m->valuesize > 0) {
            m->value = metacache_add_value (data, m->valuesize);
        }
        else {
            m->value = metacache_add_value ("", 1);
            m->valuesize = 1;
        }
        free (data);
    }
    else {
        m->value = metacache_add_value (value, size);
        m->valuesize = (int)size;
    }
}

static uint32_t
_hash_sdbm (const char *str, size_t len) {
    uint32_t h = 0;
    int c;

    const char *end = str+len;

    while (str < end) {
        c = *str++;
        h = c + (h << 6) + (h << 16) - h;
    }

    return h;
}

static ddb_keyValueHashItem_t *
_get_item_create_if_needed (ddb_keyValueList_t *md, const char *itemName, int create_if_needed) {
    uint32_t hash = _hash_sdbm (itemName, strlen (itemName)) % DDB_KEYVALUE_HASH_SIZE;

    __block ddb_keyValueHashItem_t *result = NULL;

    dispatch_sync(md->data_queue, ^{
        ddb_keyValueHashItem_t *item = md->itemHash[hash];

        const char *cached_name = NULL;
        for (; item; item = item->hashNext) {
            if (!cached_name) {
                cached_name = metacache_add_string(itemName);
            }
            if (item->name == cached_name) {
                break;
            }
        }

        if (!item && create_if_needed) {
            if (!cached_name) {
                cached_name = metacache_add_string(itemName);
            }
            item = calloc (1, sizeof (ddb_keyValueHashItem_t));
            item->name = cached_name;
            item->index = md->nextItemIndex++;
            md->itemHash[hash] = item;
            if (md->itemListTail) {
                md->itemListTail->listNext = item;
            }
            else {
                md->itemListHead = item;
            }
            md->itemListTail = item;
        }
        else if (cached_name) {
            metacache_remove_string(cached_name);
        }

        result = item;
    });

    return result;
}

static ddb_keyValueHashItem_t *
_get_item (ddb_keyValueList_t *md, const char *itemName) {
    return _get_item_create_if_needed(md, itemName, 1);
}

static ddb_keyValue_t *
_add_empty_meta_for_key (ddb_keyValueList_t *md, const char *itemName, const char *key) {
    ddb_keyValueHashItem_t *item = _get_item(md, itemName);

    // check if it's already set
    ddb_keyValue_t *normaltail = NULL;
    ddb_keyValue_t *propstart = NULL;
    ddb_keyValue_t *tail = NULL;
    ddb_keyValue_t *m = item->keyValues;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            // duplicate key
            return NULL;
        }
        // find end of properties
        tail = m;
        m = m->next;
    }
    // add
    m = calloc (1, sizeof (DB_metaInfo_t));
    m->itemIndex = item->index;
    m->key = metacache_add_string (key);

    m->next = propstart;
    if (normaltail) {
        normaltail->next = m;
    }
    else {
        item->keyValues = m;
    }

    return m;
}

static char *
_combine_into_unique_multivalue (const char *value1, size_t size1, const char *value2, size_t size2, size_t *outsize) {
    char *buf = NULL;
    size_t buflen = 0;

    const char *v = value2;
    const char *ve = value2 + size2;
    while (v < ve) {
        const char *p = value1;
        const char *e = value1 + size1;
        while (p < e) {
            if (!strcmp (p, v)) {
                // dupe
                break;
            }
            p += strlen (p) + 1;
        }
        size_t len = strlen (v);
        if (p >= e) {
            // append
            if (!buf) {
                buf = malloc (size1 + len + 1);
                buflen = size1;
                memcpy (buf, value1, size1);
            }
            else {
                buf = realloc (buf, buflen + len + 1);
            }
            memcpy (buf + buflen, v, len + 1);
            buflen += len + 1;
        }

        v += len + 1;
    }

    *outsize = buflen;
    return buf;
}

void
_meta_free_values (ddb_keyValue_t *meta) {
    metacache_remove_value (meta->value, meta->valuesize);
    meta->value = NULL;
    meta->valuesize = 0;
}

ddb_keyValue_t *
_meta_for_key (ddb_keyValueList_t *md, const char *itemName, const char *key) {
    ddb_keyValueHashItem_t *item = _get_item(md, itemName);
    ddb_keyValue_t *m = item->keyValues;
    while (m) {
        if (!strcasecmp (key, m->key)) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}

ddb_keyValue_t *
_meta_copy (ddb_keyValue_t *meta) {
    ddb_keyValue_t *kv = calloc (1, sizeof (ddb_keyValue_t));
    kv->itemIndex = meta->itemIndex;
    kv->key = metacache_add_string(meta->key);
    kv->value = metacache_add_value(meta->value, meta->valuesize);
    kv->valuesize = meta->valuesize;
    kv->file_offs = meta->file_offs;
    kv->chunk_size = meta->chunk_size;
    kv->flags = meta->flags;
    return kv;
}

void
_keyValueFree (ddb_keyValue_t *keyValue) {
    metacache_remove_string (keyValue->key);
    _meta_free_values (keyValue);
    free (keyValue);
}

#pragma mark IO Operations

static void
_io_execute_operation (ddb_keyValueList_t * restrict md, ddb_keyValueIoOperation_t * restrict op) {
    ddb_keyValueOperationType_t type = op->type;
    switch (type) {
    case DDB_KV_OPERATION_TYPE_ADD:
        // FIXME

        // calculate size
        // create a buffer
        // append to file

        break;
    case DDB_KV_OPERATION_TYPE_UPDATE:
        // FIXME

        // calculate size
        // create a buffer
        // seek to known existing position
        // if new size <= old size:
        //  overwrite, leave padding as needed
        // else
        //  mark as deleted
        //  append to file

        break;
    case DDB_KV_OPERATION_TYPE_DELETE:
        // FIXME

        // seek to known existing position
        // mark as deleted

        break;
    }
}

static void
_io_execute_operations (ddb_keyValueList_t *md, void (^completion_handler)(int error)) {
    ddb_keyValueIoOperation_t *ops = md->io_operations;
    md->io_operations = NULL;

    dispatch_async(md->io_operation_execution_queue, ^{
        ddb_keyValueDataStream_t file = md->dataStreamInterface->open(md->filename, 1);
        off_t offs = md->dataStreamInterface->seek(file, 0, SEEK_END);

        if (offs == -1) {
            completion_handler (-1);
            return;
        }
        if (offs == 0) {
            // FIXME: write header
        }

        ddb_keyValueIoOperation_t *head = ops;

        for (ddb_keyValueIoOperation_t *op = head; op; op = op->next) {
            // execute the op
            _io_execute_operation (md, op);
        }

        int closeRes = md->dataStreamInterface->close(file);

        // free all operations
        while (head) {
            ddb_keyValueIoOperation_t *op = head;
            ddb_keyValueIoOperation_t *next = op->next;

            _keyValueFree (op->keyValue);

            head = next;
        }

        completion_handler (closeRes);
    });
}

static void
_io_operation_append (ddb_keyValueList_t *md, ddb_keyValueIoOperation_t *op) {
    dispatch_sync(md->io_operation_sync_queue, ^{
        if (md->io_operations_tail) {
            md->io_operations_tail->next = op;
        }
        else {
            md->io_operations = op;
        }
        md->io_operations_tail = op;

        if (!md->multiple_io_operations_mode) {
            // execute immediately
            _io_execute_operations(md, ^(int error) {
            });
        }
    });
}

static void
_io_operation_keyvalue_append (ddb_keyValueList_t *md, ddb_keyValue_t *meta, ddb_keyValueOperationType_t operation_type) {
    ddb_keyValueIoOperation_t *op = calloc(1, sizeof (ddb_keyValueIoOperation_t));
    op->type = operation_type;
    op->keyValue = meta;
    _io_operation_append(md, op);
}

#pragma mark - Public functions

#pragma mark Managing

ddb_keyValueList_t *
md_alloc (void) {
    return calloc(1, sizeof(ddb_keyValueList_t));
}

void
md_init_with_filename (ddb_keyValueList_t *md, const char *filename, ddb_keyValueDataStreamInterface_t *dataStreamInterface) {
    md->filename = strdup (filename);
    dataStreamInterface = dataStreamInterface;
    md->data_queue = dispatch_queue_create("MetadataQueue", NULL);
    md->io_operation_sync_queue = dispatch_queue_create("MetadataOpSyncQueue", NULL);
    md->io_operation_execution_queue = dispatch_queue_create("MetadataOpExecuteQueue", NULL);

    // FIXME: load the file
}

void
md_deinit (ddb_keyValueList_t *md) {
    dispatch_sync(md->data_queue, ^{
        md_delete_all_values(md);
    });
    dispatch_release(md->data_queue);
    md->data_queue = NULL;
    dispatch_release(md->io_operation_sync_queue);
    md->io_operation_sync_queue = NULL;
    dispatch_release(md->io_operation_execution_queue);
    md->io_operation_execution_queue = NULL;
    free (md->filename);
    md->filename = NULL;
}

void
md_free (ddb_keyValueList_t *md) {
    free (md);
}

#pragma mark Metadata queries

void
md_add_with_size (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value, size_t valuesize) {
    if (!value || !*value) {
        return;
    }

    __block ddb_keyValue_t *meta_copy;

    dispatch_sync(md->data_queue, ^{
        ddb_keyValue_t *meta = _add_empty_meta_for_key (md, itemName, key);
        if (!meta) {
            return;
        }

        _meta_set_value (meta, value, valuesize);

        meta_copy = _meta_copy (meta);
    });

    _io_operation_keyvalue_append (md, meta_copy, DDB_KV_OPERATION_TYPE_ADD);
}

void
md_append_value (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value) {
    md_append_with_size(md, itemName, key, value, strlen (value)+1);
}

void
md_append_with_size (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value, size_t valuesize) {
    if (!value || valuesize == 0 || *value == 0) {
        return;
    }

    __block ddb_keyValue_t *meta_copy;
    __block ddb_keyValueOperationType_t type;
    dispatch_sync(md->data_queue, ^{
        ddb_keyValue_t *m = _meta_for_key (md, itemName, key);
        if (!m) {
            m = _add_empty_meta_for_key(md, itemName, key);
        }

        if (!m->value) {
            _meta_set_value (m, value, valuesize);
            meta_copy = _meta_copy (m);
            type = DDB_KV_OPERATION_TYPE_ADD;
            return;
        }

        size_t buflen;
        char *buf = _combine_into_unique_multivalue(m->value, m->valuesize, value, valuesize, &buflen);

        if (!buf) {
            return;
        }

        metacache_remove_value (m->value, m->valuesize);
        m->value = metacache_add_value (buf, buflen);
        m->valuesize = (int)buflen;
        free (buf);
        meta_copy = _meta_copy (m);
        type = DDB_KV_OPERATION_TYPE_UPDATE;
    });

    if (meta_copy) {
        _io_operation_keyvalue_append (md, meta_copy, type);
    }
}

const char *
md_find_value (ddb_keyValueList_t *md, const char *itemName, const char *key, char *value, size_t valuesize) {
    __block const char *res = NULL;
    dispatch_sync(md->data_queue, ^{
        ddb_keyValue_t *m = _meta_for_key (md, itemName, key);
        if (!m) {
            return;
        }

        *value = 0;
        size_t size = m->valuesize;
        if (size >= valuesize) {
            size = valuesize-1;
        }
        memcpy (value, m->value, size);
        value[size] = 0;

        res = value;
    });
    return res;
}

int
md_find_value_int (ddb_keyValueList_t *md, const char *itemName, const char *key, int def) {
    __block int res = 0;
    dispatch_sync(md->data_queue, ^{
        ddb_keyValue_t *m = _meta_for_key (md, itemName, key);
        res = (m && m->value) ? atoi (m->value) : def;
    });
    return res;
}

int64_t
md_find_value_int64 (ddb_keyValueList_t *md, const char *itemName, const char *key, int64_t def) {
    __block int64_t res = 0;
    dispatch_sync(md->data_queue, ^{
        ddb_keyValue_t *m = _meta_for_key (md, itemName, key);
        res = (m && m->value) ? atoll (m->value) : def;
    });
    return res;
}

float
md_find_value_float (ddb_keyValueList_t *md, const char *itemName, const char *key, float def) {
    __block float res = 0;
    dispatch_sync(md->data_queue, ^{
        ddb_keyValue_t *m = _meta_for_key (md, itemName, key);
        res = (m && m->value) ? (float)atof (m->value) : def;
    });
    return res;
}

void
md_replace_value (ddb_keyValueList_t *md, const char *itemName, const char *key, const char *value) {
    __block ddb_keyValue_t *meta_copy = NULL;

    dispatch_sync(md->data_queue, ^{
        // check if it's already set
        ddb_keyValue_t *m = _meta_for_key (md, itemName, key);

        if (m) {
            _meta_free_values (m);
            int l = (int)strlen (value) + 1;
            m->value = metacache_add_value(value, l);
            m->valuesize = l;
            meta_copy = _meta_copy (m);
            return;
        }
        else {
            md_add_with_size (md, itemName, key, value, strlen (value) + 1);
        }
    });

    if (meta_copy) {
        _io_operation_keyvalue_append (md, meta_copy, DDB_KV_OPERATION_TYPE_UPDATE);
    }
}

void
md_set_value_int (ddb_keyValueList_t *md, const char *itemName, const char *key, int value) {
    char s[20];
    snprintf (s, sizeof (s), "%d", value);
    md_replace_value (md, itemName, key, s);
}

void
md_set_value_int64 (ddb_keyValueList_t *md, const char *itemName, const char *key, int64_t value) {
    char s[20];
    snprintf (s, sizeof (s), "%lld", value);
    md_replace_value (md, itemName, key, s);
}

void
md_set_value_float (ddb_keyValueList_t *md, const char *itemName, const char *key, float value) {
    char s[20];
    snprintf (s, sizeof (s), "%f", value);
    md_replace_value (md, itemName, key, s);
}

void
md_delete_value (ddb_keyValueList_t *md, const char *itemName, const char *key) {
    __block ddb_keyValue_t *meta_copy = NULL;

    dispatch_sync(md->data_queue, ^{
        ddb_keyValueHashItem_t *item = _get_item_create_if_needed (md, itemName, 0);

        ddb_keyValue_t *prev = NULL;
        ddb_keyValue_t *m = item->keyValues;
        while (m) {
            if (!strcasecmp (key, m->key)) {
                if (prev) {
                    prev->next = m->next;
                }
                else {
                    item->keyValues = m->next;
                }
                m->next = NULL;
                meta_copy = m;
                return;
            }
            prev = m;
            m = m->next;
        }
    });
    if (meta_copy) {
        _io_operation_keyvalue_append (md, meta_copy, DDB_KV_OPERATION_TYPE_DELETE);
    }
}

void
md_delete_all_values (ddb_keyValueList_t *md) {
    dispatch_sync(md->data_queue, ^{
        ddb_keyValueHashItem_t *item = md->itemListHead;

        while (item) {
            ddb_keyValueHashItem_t *nextItem = item->listNext;
            ddb_keyValue_t *m = item->keyValues;
            ddb_keyValue_t *prev = NULL;
            while (m) {
                ddb_keyValue_t *next = m->next;
                if (prev) {
                    prev->next = next;
                }
                else {
                    item->keyValues = next;
                }
                _keyValueFree(m);
                m = next;
            }
            item->keyValues = NULL;
            metacache_remove_string(item->name);
            free (item);
            item = nextItem;
        }
        md->itemListHead = md->itemListTail = NULL;
        memset (md->itemHash, 0, sizeof (md->itemHash));
    });
}

// FIXME: it's questionable whether this is necessary,
// since when querying such flag -- need to immediately follow
// with an operation, to make it transactional
int
md_value_exists (ddb_keyValueList_t *md, const char *itemName, const char *key) {
    __block ddb_keyValue_t *m = NULL;
    dispatch_sync(md->data_queue, ^{
        m = _meta_for_key (md, itemName, key);
    });
    return m ? 1 : 0;
}

