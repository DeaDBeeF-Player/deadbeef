/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "LogWindowController.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

static const char conf_autoopen_key[] = "cocoaui.log.autoopen";
static const int conf_autoopen_default = 1;

@interface LogWindowController()

@property (nonatomic) BOOL wasShown;
@property (nonatomic) NSDictionary *attributes;
@property (weak) IBOutlet NSButton *autoOpenButton;

@end

@implementation LogWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    NSFont *font;

    if (@available(macOS 10.15, *)) {
        font = [NSFont monospacedSystemFontOfSize:0 weight:NSFontWeightRegular];
    } else {
        font = [NSFont systemFontOfSize:0];
    }

    self.attributes = @{
        NSForegroundColorAttributeName:NSColor.controlTextColor,
        NSFontAttributeName: font
    };

    self.autoOpenButton.state = deadbeef->conf_get_int(conf_autoopen_key, conf_autoopen_default) ? NSControlStateValueOn : NSControlStateValueOff;

    deadbeef->log_viewer_register (_cocoaui_logger_callback, (__bridge void *)(self));
}

- (void)windowDidChangeOcclusionState:(NSNotification *)notification {
    if (!self.wasShown) {
        [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
        self.wasShown = YES;
    }
}

- (void)dealloc {
    deadbeef->log_viewer_unregister (_cocoaui_logger_callback, (__bridge void *)(self));
}

- (void)appendText:(NSString *)text {
    NSAttributedString* attr = [[NSAttributedString alloc] initWithString:text attributes:self.attributes];

    NSRect visibleRect = _clipView.documentVisibleRect;
    NSRect docRect = _textView.frame;

    BOOL scroll = NO;
    if (visibleRect.origin.y + visibleRect.size.height >= docRect.size.height) {
        scroll = YES;
    }

    [self.textView.textStorage appendAttributedString:attr];
    if (scroll) {
        [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
    }
}

- (IBAction)clearAction:(id)sender {
    self.textView.textStorage.attributedString =  [[NSAttributedString alloc] initWithString:@"" attributes:self.attributes];
}

static void
_cocoaui_logger_callback (DB_plugin_t *plugin, uint32 layers, const char *text, void *ctx) {
    LogWindowController *ctl = (__bridge LogWindowController *)(ctx);
    [ctl appendLoggerText:text forPlugin:plugin onLayers:layers];
}

- (void)appendLoggerText:(const char *)text forPlugin:(DB_plugin_t *)plugin onLayers:(uint32_t)layers {
    NSString *str = @(text);
    if (!str) {
        return; // may happen in case of invalid UTF8 and such
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        [self appendText:str];

        int autoopen = deadbeef->conf_get_int(conf_autoopen_key, conf_autoopen_default);

        if (autoopen && layers == DDB_LOG_LAYER_DEFAULT) {
            if (!(self.window).visible) {
                [self showWindow:self];
            }
        }
    });
}

- (IBAction)autoOpenButtonAction:(NSButton *)sender {
    deadbeef->conf_set_int(conf_autoopen_key, sender.state == NSControlStateValueOn);
    deadbeef->conf_save();
}

@end
