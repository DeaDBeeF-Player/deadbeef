#include "scriptable.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static scriptableItem_t *rootNode;

keyValuePair_t *
keyValuePairAlloc (void) {
    return calloc (1, sizeof (keyValuePair_t));
}

void
keyValuePairFree (keyValuePair_t *p) {
    free (p->key);
    free (p->value);
    free (p);
}

scriptableItem_t *
scriptableItemAlloc (void) {
    return calloc (1, sizeof (scriptableItem_t));
}

void
scriptableItemSave (scriptableItem_t *item) {
    if (item->save) {
        item->save (item);
    }
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
    if (item->free) {
        item->free (item);
    }

    keyValuePair_t *p = item->properties;
    while (p) {
        keyValuePair_t *next = p->next;
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
    if (!item->createItemOfType) {
        return NULL;
    }
    return item->createItemOfType (item, type);
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
}

void
scriptableItemRemoveSubItem (scriptableItem_t *item, scriptableItem_t *subItem) {
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

const char *
scriptableItemPropertyValueForKey (scriptableItem_t *item, const char *key) {
    for (keyValuePair_t *p = item->properties; p; p = p->next) {
        if (!strcasecmp (p->key, key)) {
            return p->value;
        }
    }
    return NULL;
}

void
scriptableItemSetPropertyValueForKey (scriptableItem_t *item, const char *value, const char *key) {
    keyValuePair_t *prev = NULL;
    for (keyValuePair_t *p = item->properties; p; prev = p, p = p->next) {
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
                return;
            }
        }
    }
    // new prop
    if (value) {
        keyValuePair_t *p = keyValuePairAlloc ();
        p->key = strdup (key);
        p->value = strdup (value);
        p->next = item->properties;
        item->properties = p;
    }
}

scriptableStringListItem_t *
scriptableItemFactoryItemNames (struct scriptableItem_s *item) {
    if (!item->factoryItemNames) {
        return NULL;
    }
    return item->factoryItemNames (item);
}

scriptableStringListItem_t *
scriptableItemFactoryItemTypes (struct scriptableItem_s *item) {
    if (!item->factoryItemTypes) {
        return NULL;
    }
    return item->factoryItemTypes (item);
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
