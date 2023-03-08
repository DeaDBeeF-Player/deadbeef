#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scriptable/scriptable.h"

static scriptableItem_t *rootNode;

scriptableKeyValue_t *
keyValuePairAlloc (void) {
    return calloc (1, sizeof (scriptableKeyValue_t));
}

void
keyValuePairFree (scriptableKeyValue_t *p) {
    free (p->key);
    free (p->value);
    free (p);
}

scriptableItem_t *
scriptableItemAlloc (void) {
    return calloc (1, sizeof (scriptableItem_t));
}

int
scriptableItemSave (scriptableItem_t *item) {
    if (!item->isReadonly && item->callbacks && item->callbacks->save) {
        return item->callbacks->save (item);
    }
    return 0;
}

char *
scriptableItemSaveToString (scriptableItem_t *item) {
    if (item->callbacks && item->callbacks->saveToString) {
        return item->callbacks->saveToString (item);
    }
    return NULL;
}


scriptableStringListItem_t *
scriptableStringListItemAlloc (void) {
    return calloc (1, sizeof (scriptableStringListItem_t));
}

void
scriptableStringListItemFree (scriptableStringListItem_t *item) {
    free (item->str);
    free (item);
}

void
scriptableStringListFree (scriptableStringListItem_t *list) {
    while (list) {
        scriptableStringListItem_t *next = list->next;
        scriptableStringListItemFree (list);
        list = next;
    }
}

void
scriptableItemFree (scriptableItem_t *item) {
    if (item->callbacks && item->callbacks->free) {
        item->callbacks->free (item);
    }

    scriptableKeyValue_t *p = item->properties;
    while (p) {
        scriptableKeyValue_t *next = p->next;
        keyValuePairFree(p);
        p = next;
    }
    item->properties = NULL;

    scriptableItem_t *i = item->children;
    while (i) {
        scriptableItem_t *next = i->next;
        scriptableItemFree (i);
        i = next;
    }
    item->children = NULL;

    free (item);
}

unsigned int
scriptableItemNumChildren (scriptableItem_t *item) {
    unsigned int i = 0;
    for (scriptableItem_t *c = item->children; c; i++, c = c->next);
    return i;
}

scriptableItem_t *
scriptableItemChildAtIndex (scriptableItem_t *item, unsigned int index) {
    int i = 0;
    for (scriptableItem_t *c = item->children; c; i++, c = c->next) {
        if (i == index) {
            return c;
        }
    }
    return NULL;
}

int
scriptableItemIndexOfChild (scriptableItem_t *item, scriptableItem_t *child) {
    int i = 0;
    for (scriptableItem_t *c = item->children; c; i++, c = c->next) {
        if (c == child) {
            return i;
        }
    }
    return -1;
}

scriptableItem_t *
scriptableItemSubItemForName (scriptableItem_t *item, const char *name) {
    for (scriptableItem_t *c = item->children; c; c = c->next) {
        const char *childName = scriptableItemPropertyValueForKey(c, "name");
        if (childName && !strcmp (name, childName)) {
            return c;
        }

    }
    return NULL;
}


scriptableItem_t *
scriptableItemCreateItemOfType (scriptableItem_t *item, const char *type) {
    if (item->callbacks && item->callbacks->createItemOfType) {
        return item->callbacks->createItemOfType (item, type);
    }
    return NULL;
}

void
scriptableItemAddSubItem (scriptableItem_t *item, scriptableItem_t *subItem) {
    if (item->childrenTail) {
        item->childrenTail->next = subItem;
    }
    else {
        item->children = subItem;
    }
    item->childrenTail = subItem;

    subItem->parent = item;

    scriptableItemUpdate(item);
}

scriptableItem_t *
scriptableItemClone (scriptableItem_t *item) {
    scriptableItem_t *cloned = scriptableItemAlloc ();
    for (scriptableKeyValue_t *property = item->properties; property; property = property->next) {
        scriptableItemSetPropertyValueForKey(cloned, property->value, property->key);
    }
    for (scriptableItem_t *child = item->children; child; child = child->next) {
        scriptableItem_t *clonedChild = scriptableItemClone(child);
        scriptableItemAddSubItem(cloned, clonedChild);
    }
    cloned->callbacks = item->callbacks;
    cloned->configDialog = item->configDialog;

    return cloned;
}


void
scriptableItemInsertSubItemAtIndex (scriptableItem_t *item, scriptableItem_t *subItem, unsigned int insertPosition) {
    unsigned int pos = 0;
    scriptableItem_t *prev = NULL;
    for (scriptableItem_t *c = item->children; c; pos++, prev = c, c = c->next) {
        if (pos == insertPosition) {
            break;
        }
    }

    assert (pos == insertPosition && "Invalid insertPosition");

    scriptableItem_t *next = prev ? prev->next : item->children;
    if (prev) {
        prev->next = subItem;
    }
    else {
        item->children = subItem;
    }
    subItem->next = next;
    if (item->childrenTail == prev) {
        item->childrenTail = subItem;
    }

    subItem->parent = item;

    scriptableItemUpdate(item);
}

void
scriptableItemRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem) {
    if (item->callbacks && item->callbacks->removeSubItem) {
        item->callbacks->removeSubItem (item, subItem);
    }

    scriptableItem_t *prev = NULL;
    for (scriptableItem_t *c = item->children; c; prev = c, c = c->next) {
        if (c == subItem) {
            if (prev) {
                prev->next = c->next;
            }
            else {
                item->children = c->next;
            }
            if (item->childrenTail == subItem) {
                item->childrenTail = prev;
            }
            break;
        }
    }
}

void
scriptableItemUpdate (scriptableItem_t *item) {
    if (item->isLoading) {
        return;
    }
    if (item->callbacks && item->callbacks->updateItem) {
        item->callbacks->updateItem (item);
    }
    if (item->parent) {
        scriptableItemUpdateForSubItem(item->parent, item);
    }
}

void
scriptableItemUpdateForSubItem (scriptableItem_t *item, scriptableItem_t *subItem) {
    if (item->isLoading) {
        return;
    }
    if (item->callbacks && item->callbacks->updateItemForSubItem) {
        item->callbacks->updateItemForSubItem (item, subItem);
    }
}


const char *
scriptableItemPropertyValueForKey (scriptableItem_t *item, const char *key) {
    for (scriptableKeyValue_t *p = item->properties; p; p = p->next) {
        if (!strcasecmp (p->key, key)) {
            return p->value;
        }
    }
    return NULL;
}

static void
scriptableItemPropertyValueWillChangeForKey (scriptableItem_t *item, const char *key) {
    if (!item->isLoading && item->callbacks && item->callbacks->propertyValueWillChangeForKey) {
        item->callbacks->propertyValueWillChangeForKey (item, key);
    }
}

static void
scriptableItemPropertyValueDidChangeForKey (scriptableItem_t *item, const char *key) {
    if (!item->isLoading && item->callbacks && item->callbacks->propertyValueDidChangeForKey) {
        item->callbacks->propertyValueDidChangeForKey (item, key);
    }
}

void
scriptableItemSetPropertyValueForKey (scriptableItem_t *item, const char *value, const char *key) {
    scriptableKeyValue_t *prev = NULL;
    scriptableItemPropertyValueWillChangeForKey (item, key);
    for (scriptableKeyValue_t *p = item->properties; p; prev = p, p = p->next) {
        if (!strcasecmp (p->key, key)) {
            if (p->value) {
                free (p->value);
                p->value = NULL;
            }
            if (value) {
                p->value = strdup(value);
            }
            else {
                // remove property
                if (prev) {
                    prev->next = p->next;
                    keyValuePairFree (p);
                }
            }
            scriptableItemPropertyValueDidChangeForKey (item, key);
            scriptableItemUpdate(item);
            return;
        }
    }
    // new prop
    if (value) {
        scriptableKeyValue_t *p = keyValuePairAlloc ();
        p->key = strdup (key);
        p->value = strdup (value);
        p->next = item->properties;
        item->properties = p;
    }

    scriptableItemPropertyValueDidChangeForKey (item, key);
    scriptableItemUpdate(item);
}

void
scriptableItemSetUniqueNameUsingPrefixAndRoot (scriptableItem_t *item, const char *prefix, scriptableItem_t *root) {
    int i;
    const int MAX_ATTEMPTS = 100;
    char name[100];
    for (i = 0; i < MAX_ATTEMPTS; i++) {
        if (i == 0) {
            snprintf (name, sizeof (name), "%s", prefix);
        }
        else {
            snprintf (name, sizeof (name), "%s %02d", prefix, i);
        }

        if (!scriptableItemContainsSubItemWithName(root, name)) {
            scriptableItemSetPropertyValueForKey(item, name, "name");
            return;
        }
    }
}

int
scriptableItemContainsSubItemWithName (scriptableItem_t *item, const char *name) {
    scriptableItem_t *c = NULL;
    for (c = item->children; c; c = c->next) {
        const char *cname = scriptableItemPropertyValueForKey(c, "name");
        if (!strcasecmp (name, cname)) {
            return 1;
        }
    }
    return 0;
}

int
scriptableItemIsSubItemNameAllowed (scriptableItem_t *item, const char *name) {
    if (item->callbacks && item->callbacks->isSubItemNameAllowed) {
        return item->callbacks->isSubItemNameAllowed (item, name);
    }
    return 1;
}

scriptableStringListItem_t *
scriptableItemFactoryItemNames (struct scriptableItem_s *item) {
    if (item->callbacks && item->callbacks->factoryItemNames) {
        return item->callbacks->factoryItemNames (item);
    }
    return NULL;
}

scriptableStringListItem_t *
scriptableItemFactoryItemTypes (struct scriptableItem_s *item) {
    if (item->callbacks && item->callbacks->factoryItemTypes) {
        return item->callbacks->factoryItemTypes (item);
    }
    return NULL;
}

char *
scriptableItemFormattedName (scriptableItem_t *item) {
    const char *name = scriptableItemPropertyValueForKey(item, "name");
    if (!name) {
        return NULL;
    }

    if (!item->isReadonly || !item->callbacks || !item->callbacks->readonlyPrefix) {
        return strdup (name);
    }

    size_t len = strlen (name) + strlen (item->callbacks->readonlyPrefix) + 1;
    char *buffer = calloc (1, len);
    snprintf (buffer, len, "%s%s", item->callbacks->readonlyPrefix, name);
    return buffer;
}

void
scriptableInit (void) {
    rootNode = scriptableItemAlloc ();
}

void
scriptableFree (void) {
    if (rootNode) {
        scriptableItemFree (rootNode);
        rootNode = NULL;
    }
}

scriptableItem_t *
scriptableRoot (void) {
    if (!rootNode) {
        rootNode = scriptableItemAlloc();
    }
    return rootNode;
}
