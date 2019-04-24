#ifndef scriptable_h
#define scriptable_h

typedef struct keyValue_s {
    struct keyValue_s *next;
    char *key;
    char *value;
} keyValuePair_t;

typedef struct stringListItem_s {
    struct stringListItem_s *next;
    const char *str;
} stringListItem_t;

typedef struct scriptableItem_s {
    struct scriptableItem_s *next;
    keyValuePair_t *properties;

    struct scriptableItem_s *parent;
    struct scriptableItem_s *children;
    struct scriptableItem_s *childrenTail;

    stringListItem_t (*subItemTypes)(struct scriptableItem_s *item);
    struct scriptableItem_s *(*createItemOfType)(const char *type);
    void (*free)(struct scriptableItem_s *item);
    void (*save)(struct scriptableItem_s *item);
} scriptableItem_t;

scriptableItem_t *
scriptableItemAlloc (void);

void
scriptableItemFree (scriptableItem_t *item);

void
scriptableItemSave (scriptableItem_t *item);

unsigned int
scriptableItemNumChildren (scriptableItem_t *item);

scriptableItem_t *
scriptableItemChildAtIndex (scriptableItem_t *item, unsigned int index);

int
scriptableItemIndexOfChild (scriptableItem_t *item, scriptableItem_t *child);

scriptableItem_t *
scriptableItemSubItemForName (scriptableItem_t *item, const char *name);

scriptableItem_t *
scriptableItemCreateItemOfType (scriptableItem_t *item, const char *type);

void
scriptableItemAddSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

void
scriptableItemInsertSubItemAtIndex (scriptableItem_t *item, scriptableItem_t *subItem, unsigned int insertPosition);

void
scriptableItemRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

const char *
scriptableItemPropertyValueForKey (scriptableItem_t *item, const char *key);

void
scriptableItemSetPropertyValueForKey (scriptableItem_t *item, const char *value, const char *key);

void
scriptableInit (void);

void
scriptableFree (void);

scriptableItem_t *
scriptableRoot (void);


#endif /* scriptable_h */
