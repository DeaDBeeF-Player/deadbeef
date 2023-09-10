#ifndef scriptable_h
#define scriptable_h

#ifdef __cplusplus
extern "C" {
#endif

// TODO: this seems to be only used by factories,
// could be refactored to return string arrays.
typedef struct stringListItem_s {
    struct stringListItem_s *next;
    char *str;
} scriptableStringListItem_t;

// Please note that the "name" property key is used internally by a lot of systems,
// including UI.
// Any other keys are allowed for any use (so far).
struct scriptableItem_s;
typedef struct scriptableItem_s scriptableItem_t;

enum scriptableFlags_t {
    /// prevent calling hooks while loading data
    SCRIPTABLE_FLAG_IS_LOADING = 1<<0,
    SCRIPTABLE_FLAG_IS_READONLY = 1<<1,
    /// for example, dsp preset, or dsp chain
    SCRIPTABLE_FLAG_IS_LIST = 1<<2,
    /// whether items can be reordered by the user
    SCRIPTABLE_FLAG_IS_REORDABLE = 1<<3,
    /// whether the names can be changed by the user
    SCRIPTABLE_FLAG_CAN_RENAME = 1<<4,
    SCRIPTABLE_FLAG_CAN_RESET = 1<<5,
};

struct scriptableOverrides_t {
    /// Must be set to @c sizeof(scriptableOverrides_t)
    /// If 0 will assume the v1 size.
    int _size;

    /// for drag drop on mac
    const char *(*pasteboardItemIdentifier)(scriptableItem_t *item);

    /// A text to display in UI when the item is read-only
    const char *(*readonlyPrefix)(scriptableItem_t *item);

    scriptableStringListItem_t *(*factoryItemNames)(scriptableItem_t *item);

    scriptableStringListItem_t *(*factoryItemTypes)(scriptableItem_t *item);

    scriptableItem_t *(*createItemOfType)(scriptableItem_t *item, const char *type);

    int (*isSubItemNameAllowed)(scriptableItem_t *item, const char *name);

    /// Called after the child list or properties are modified
    /// (add/remove/insert child or property)
    int (*didUpdateItem)(scriptableItem_t *item);

    /// Called right after @c didUpdateItem, but for the parent
    int (*didUpdateChildItem)(scriptableItem_t *item, scriptableItem_t *subItem);

    /// Called right before the child item is removed
    int (*willRemoveChildItem)(scriptableItem_t *item, scriptableItem_t *subItem);

    /// Called before the item is destroyed to perform additional cleanup
    void (*willDestroyItem)(scriptableItem_t *item);

    int (*save)(scriptableItem_t *item);

    int (*reset)(scriptableItem_t *item);

    char * (*saveToString)(scriptableItem_t *item);

    void (*propertyValueWillChangeForKey) (scriptableItem_t *item, const char *key);
    void (*propertyValueDidChangeForKey) (scriptableItem_t *item, const char *key);
};

typedef struct scriptableOverrides_t scriptableOverrides_t;

scriptableItem_t *
scriptableItemAlloc (void);

void
scriptableItemSetOverrides(scriptableItem_t *item, scriptableOverrides_t *overrides);

void
scriptableItemFree (scriptableItem_t *item);

scriptableStringListItem_t *
scriptableStringListItemAlloc (void);

void
scriptableStringListItemFree (scriptableStringListItem_t *item);

uint64_t scriptableItemFlags(scriptableItem_t *item);
void scriptableItemFlagsSet(scriptableItem_t *item, uint64_t flags);
void scriptableItemFlagsAdd(scriptableItem_t *item, uint64_t flags);
void scriptableItemFlagsRemove(scriptableItem_t *item, uint64_t flags);

void
scriptableStringListFree (scriptableStringListItem_t *list);

int
scriptableItemSave (scriptableItem_t *item);

int
scriptableItemReset (scriptableItem_t *item);

char *
scriptableItemSaveToString (scriptableItem_t *item);

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

scriptableItem_t *
scriptableItemParent(scriptableItem_t *item);

void scriptableItemPropertiesForEach(scriptableItem_t *item, int(^block)(const char *key, const char *value));

const char *
scriptableItemConfigDialog(scriptableItem_t *item);

void
scriptableItemSetConfigDialog(scriptableItem_t *item, const char *configDialog);

const char *
scriptableItemPasteboardIdentifier(scriptableItem_t *item);

scriptableItem_t *
scriptableItemChildren(scriptableItem_t *item);

scriptableItem_t *
scriptableItemNext(scriptableItem_t *item);

// - CRUD

void
scriptableItemInsertSubItemAtIndex (scriptableItem_t *item, scriptableItem_t *subItem, unsigned int insertPosition);

void
scriptableItemRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

void
scriptableItemUpdate (scriptableItem_t *item);

// -

const char *
scriptableItemPropertyValueForKey (scriptableItem_t *item, const char *key);

void
scriptableItemSetPropertyValueForKey (scriptableItem_t *item, const char *value, const char *key);

void
scriptableItemSetUniqueNameUsingPrefixAndRoot (scriptableItem_t *item, const char *prefix, scriptableItem_t *root);

int
scriptableItemContainsSubItemWithName (scriptableItem_t *item, const char *name);

int
scriptableItemIsSubItemNameAllowed (scriptableItem_t *item, const char *name);

scriptableStringListItem_t *
scriptableItemFactoryItemNames (struct scriptableItem_s *item);

scriptableStringListItem_t *
scriptableItemFactoryItemTypes (struct scriptableItem_s *item);

char *
scriptableItemFormattedName (scriptableItem_t *item);

void
scriptableInitShared (void);

void
scriptableDeinitShared (void);

scriptableItem_t *
scriptableRootShared(void);

#ifdef __cplusplus
}
#endif

#endif /* scriptable_h */
