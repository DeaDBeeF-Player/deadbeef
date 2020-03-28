#ifndef scriptable_h
#define scriptable_h

typedef struct scriptableKeyValue_s {
    struct scriptableKeyValue_s *next;
    char *key;
    char *value;
} scriptableKeyValue_t;

typedef struct stringListItem_s {
    struct stringListItem_s *next;
    char *str;
} scriptableStringListItem_t;

typedef struct scriptableItem_s {
    struct scriptableItem_s *next;
    scriptableKeyValue_t *properties;

    struct scriptableItem_s *parent;
    struct scriptableItem_s *children;
    struct scriptableItem_s *childrenTail;

    int isList; // for example, dsp preset, or dsp chain

    // FIXME: this should be combined in a single global struct and stored as a pointer

    // hooks for subclasses
    scriptableStringListItem_t *(*factoryItemNames)(struct scriptableItem_s *item);

    scriptableStringListItem_t *(*factoryItemTypes)(struct scriptableItem_s *item);

    struct scriptableItem_s *(*createItemOfType)(struct scriptableItem_s *item, const char *type);

    // additional update logic, such as save the item data to disk
    int (*updateItem)(struct scriptableItem_s *item);

    // additional update logic, such as save all subItems data to disk, if it's stored in a single file
    int (*updateItemForSubItem)(struct scriptableItem_s *item, struct scriptableItem_s *subItem);

    // additional remove logic, such as delete subItem data from disk
    int (*removeSubItem)(struct scriptableItem_s *item, struct scriptableItem_s *subItem);

    void (*free)(struct scriptableItem_s *item);
    void (*save)(struct scriptableItem_s *item);
} scriptableItem_t;

scriptableItem_t *
scriptableItemAlloc (void);

void
scriptableItemFree (scriptableItem_t *item);

scriptableStringListItem_t *
scriptableStringListItemAlloc (void);

void
scriptableStringListItemFree (scriptableStringListItem_t *item);

void
scriptableStringListFree (scriptableStringListItem_t *list);

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

scriptableItem_t *
scriptableItemClone (scriptableItem_t *item);

// - CRUD

void
scriptableItemInsertSubItemAtIndex (scriptableItem_t *item, scriptableItem_t *subItem, unsigned int insertPosition);

void
scriptableItemRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

void
scriptableItemUpdate (scriptableItem_t *item);

void
scriptableItemUpdateForSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

// -

const char *
scriptableItemPropertyValueForKey (scriptableItem_t *item, const char *key);

void
scriptableItemSetPropertyValueForKey (scriptableItem_t *item, const char *value, const char *key);

scriptableStringListItem_t *
scriptableItemFactoryItemNames (struct scriptableItem_s *item);

scriptableStringListItem_t *
scriptableItemFactoryItemTypes (struct scriptableItem_s *item);

void
scriptableInit (void);

void
scriptableFree (void);

scriptableItem_t *
scriptableRoot (void);


#endif /* scriptable_h */
