// example misc plugin which receives events, and prints them
#include <deadbeef/deadbeef.h>

static DB_functions_t *deadbeef;

static int
eventhandler_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SEEKED:
        printf("DB_EV_SEEKED\n");
        break;
    case DB_EV_TRACKINFOCHANGED:
        printf("DB_EV_TRACKINFOCHANGED\n");
        break;
    case DB_EV_SONGSTARTED:
        printf("DB_EV_SONGSTARTED\n");
        break;
    case DB_EV_PAUSED:
        printf("DB_EV_PAUSED\n");
        break;
    case DB_EV_STOP:
        printf("DB_EV_STOP\n");
        break;
    case DB_EV_VOLUMECHANGED:
        printf("DB_EV_VOLUMECHANGED\n");
        break;
    case DB_EV_CONFIGCHANGED:
        printf("DB_EV_CONFIGCHANGED\n");
        break;
    }

    return 0;
}

DB_misc_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.id = "example",
    .plugin.name ="Event Handler Example",
    .plugin.descr = "Example event handler plugin",
    .plugin.copyright = "copyright message - author(s), license, etc",
    .plugin.message = eventhandler_message,
};

DB_plugin_t *
eventhandler_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return DB_PLUGIN(&plugin);
}
