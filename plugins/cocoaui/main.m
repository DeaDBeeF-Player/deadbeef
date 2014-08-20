//
//  main.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 16/11/13.
//  Copyright (c) 2013 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"
#include "deadbeef.h"

DB_functions_t *deadbeef;

int cocoaui_start(void)
{
	char *argv[1];
	argv[0] = "FIXME";
	return NSApplicationMain(1, (const char **)argv);
}

int cocoaui_message (uint32_t _id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return [AppDelegate ddb_message:_id ctx:ctx p1:p1 p2:p2];
}

DB_gui_t plugin = {
	.plugin.type = DB_PLUGIN_GUI,
	.plugin.api_vmajor = 1,
	.plugin.api_vminor = 5,
	.plugin.version_major = 1,
	.plugin.version_minor = 0,
	.plugin.id = "cocoaui",
	.plugin.name = "Cocoa UI",
	.plugin.start = cocoaui_start,
    .plugin.message = cocoaui_message,
	// NSApplicationMain doesn't return, so it doesn't seem it's possible to cleanup
};

DB_plugin_t *cocoaui_load (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
	return (DB_plugin_t *)&plugin;
}