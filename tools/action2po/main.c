#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "../../deadbeef.h"

#ifdef __APPLE__
#define SOEXT ".dylib"
#else
#define SOEXT ".so"
#endif


void
pl_lock (void) {}

void
pl_unlock (void) {}

int
main (int argc, char *argv[]) {
    DB_functions_t deadbeef = {
        .pl_lock = pl_lock,
        .pl_unlock = pl_unlock,
    };

    for (int i = 1; i < argc; i++) {
        char d_name[256];
        void *handle;
        size_t l;


        char *slash = strrchr (argv[i], '/');
        if (!slash) {
            slash = argv[i];
        }
        else {
            slash++;
        }
        l = strlen (slash);
        if (l < strlen(SOEXT) || strcasecmp (&slash[l-strlen(SOEXT)], SOEXT)) {
            fprintf (stderr, "pluginfo: invalid fname %s\n", argv[i]);
            continue;
        }

        strcpy (d_name, slash);

        handle = dlopen (argv[i], RTLD_NOW);
        if (!handle) {
            fprintf (stderr, "dlopen error: %s\n", dlerror ());
            continue;
        }
        printf ("loading actions from %s\n", d_name);
        d_name[l-strlen(SOEXT)] = 0;
        strcat (d_name, "_load");
        DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name);
        if (!plug_load) {
            fprintf (stderr, "pluginfo: dlsym error: %s\n", dlerror ());
            dlclose (handle);
            continue;
        }

        DB_plugin_t *plug = plug_load ((DB_functions_t *)&deadbeef);

        if (plug->get_actions) {
            DB_plugin_action_t *act = plug->get_actions (NULL);
            while (act) {
                printf ("%s\n", act->title);
                act = act->next;
            }
        }

        dlclose (handle);
    }
    return 0;
}
