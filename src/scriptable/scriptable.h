#ifndef scriptable_h
#define scriptable_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct scriptableKeyValue_s {
    struct scriptableKeyValue_s *next;
    char *key;
    char *value;
} scriptableKeyValue_t;

typedef struct stringListItem_s {
    struct stringListItem_s *next;
    char *str;
} scriptableStringListItem_t;

struct scriptableItem_s;
typedef struct scriptableItem_s scriptableItem_t;

// FIXME: this is not extensible (needs size property)
typedef struct {
    /// FIXME: convert everything to function pointers

    int isList; // for example, dsp preset, or dsp chain
    int isReorderable; // whether items can be reordered by the user
    int allowRenaming; // whether the names can be changed by the user

    /// for drag drop on mac
    const char *pasteboardItemIdentifier;

    /// A text to display in UI when the item is read-only
    const char *readonlyPrefix;

    scriptableStringListItem_t *(*factoryItemNames)(scriptableItem_t *item);

    scriptableStringListItem_t *(*factoryItemTypes)(scriptableItem_t *item);

    scriptableItem_t *(*createItemOfType)(scriptableItem_t *item, const char *type);

    int (*isSubItemNameAllowed)(scriptableItem_t *item, const char *name);

    // additional update logic, such as save the item data to disk
    int (*updateItem)(scriptableItem_t *item);

    // additional update logic, such as save all subItems data to disk, if it's stored in a single file
    int (*updateItemForSubItem)(scriptableItem_t *item, scriptableItem_t *subItem);

    // additional remove logic, such as delete subItem data from disk
    int (*removeSubItem)(scriptableItem_t *item, scriptableItem_t *subItem);

    void (*free)(scriptableItem_t *item);

    int (*save)(scriptableItem_t *item);

    char * (*saveToString)(scriptableItem_t *item);

    void (*propertyValueWillChangeForKey) (scriptableItem_t *item, const char *key);
    void (*propertyValueDidChangeForKey) (scriptableItem_t *item, const char *key);
} scriptableCallbacks_t;

scriptableItem_t *
scriptableItemAlloc (void);

void
scriptableItemSetCallbacks(scriptableItem_t *item, scriptableCallbacks_t *callbacks);

void
scriptableItemFree (scriptableItem_t *item);

scriptableStringListItem_t *
scriptableStringListItemAlloc (void);

void
scriptableStringListItemFree (scriptableStringListItem_t *item);

void
scriptableStringListFree (scriptableStringListItem_t *list);

int
scriptableItemSave (scriptableItem_t *item);

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

scriptableKeyValue_t *
scriptableItemProperties(scriptableItem_t *item);

int
scriptableItemIsList(scriptableItem_t *item);

int
scriptableItemIsReorderable(scriptableItem_t *item);

int
scriptableItemIsRenamable(scriptableItem_t *item);

int
scriptableItemIsLoading(scriptableItem_t *item);

void
scriptableItemSetIsLoading(scriptableItem_t *item, int isLoading);

int
scriptableItemIsReadOnly(scriptableItem_t *item);

void
scriptableItemSetIsReadOnly(scriptableItem_t *item, int isReadOnly);

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

void
scriptableItemUpdateForSubItem (scriptableItem_t *item, scriptableItem_t *subItem);

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
scriptableInit (void);

void
scriptableFree (void);

scriptableItem_t *
scriptableRoot (void);

#ifdef __cplusplus
}
#endif

#endif /* scriptable_h */
