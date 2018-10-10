#import "PluginConfigurationViewController.h"
#import "ItemListViewController.h"
#import "deadbeef-Swift.h"
#include "pluginsettings.h"
#include "parser.h"
#include "conf.h"
#include "messagepump.h"

@implementation PluginConfigurationValueAccessorConfig
- (NSString *)getValueForKey:(NSString *)key def:(NSString *)def {
    char val[1000];
    conf_get_str ([key UTF8String], [def UTF8String], val, sizeof (val));
    return [NSString stringWithUTF8String:val];
}

- (void)setValueForKey:(NSString *)key value:(NSString *)value {
    conf_set_str([key UTF8String], [value UTF8String]);
    messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
    conf_save();
}
- (int)count {
    return -1;
}

- (NSString *)keyForIndex:(int)index {
    return nil;
}
@end

@implementation PluginConfigurationValueAccessorPreset {
    PresetManager *_presetMgr;
    int _presetIndex;
}

- (id)initWithPresetManager:(PresetManager*)presetMgr presetIndex:(int)presetIndex {
    self = [super init];
    _presetMgr = presetMgr;
    _presetIndex = presetIndex;
    return self;
}

- (NSString *)getValueForKey:(NSString *)key def:(NSString *)def {
    return [_presetMgr savePresetWithIndex:_presetIndex itemIndex:[key integerValue]];
}

- (void)setValueForKey:(NSString *)key value:(NSString *)value {
    [_presetMgr loadPresetWithIndex:[key intValue] fromString:value];
}

- (int)count {
    return (int)[_presetMgr presetItemCountWithPresetIndex:_presetIndex];
}

- (NSString *)keyForIndex:(int)index {
    return [NSString stringWithFormat:@"%d", index];
}
@end

@interface PluginConfigurationViewController () {
    NSMutableArray *_bindings;
    settings_data_t _settingsData;
    NSObject<PluginConfigurationValueAccessor> *_accessor;
}

@end

@implementation PluginConfigurationViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}

- (void)initPluginConfiguration:(const char *)config accessor:(NSObject<PluginConfigurationValueAccessor> *)accessor {
    _accessor = accessor;
    NSView *view = [self view];
    _bindings = [[NSMutableArray alloc] init];

    settings_data_free (&_settingsData);

    BOOL have_settings = YES;
    if (!config || settings_data_init(&_settingsData, config) < 0) {
        have_settings = NO;
    }

    int label_padding = 8;
    int unit_spacing = 4;
    int unit_h = 22;
    int h = _settingsData.nprops * (unit_h + unit_spacing);
    NSSize sz;

    if ([view isKindOfClass:[NSScrollView class]]) {
        NSScrollView *scrollView = (NSScrollView *)view;
        view = [scrollView documentView];
        sz = [scrollView contentSize];
        if (h < sz.height) {
            h = sz.height;
        }
        NSRect frame = [view frame];
        [view setFrame:NSMakeRect(0, 0, frame.size.width, h)];
        NSPoint pt = NSMakePoint(0.0, [[scrollView documentView]
                                       bounds].size.height);
        [[scrollView documentView] scrollPoint:pt];

        sz = [scrollView contentSize];
    }
    else {
        sz = [view frame].size;
        h = sz.height;
    }

    while ([[view subviews] count] > 0) {
        [[[view subviews] lastObject] removeFromSuperview];
    }

    if (!have_settings) {
        NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(0, sz.height/2 - unit_h/2, sz.width, unit_h)];
        [lbl setStringValue:@"This plugin doesn't have settings"];
        [lbl setAlignment:NSCenterTextAlignment];
        [lbl setBezeled:NO];
        [lbl setDrawsBackground:NO];
        [lbl setEditable:NO];
        [lbl setSelectable:NO];
        [view addSubview:lbl];
        return;
    }

    // FIXME
    conf_lock ();
    for (int i = 0; i < _settingsData.nprops; i++) {
        int y = h - (unit_h + unit_spacing) * i - unit_h - 4;

        int label_w = 0;

        // set label
        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_SELECT:
            case PROP_SLIDER:
            case PROP_FILE:
            {
                NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(4, y-2, 500, unit_h)];
                [lbl setStringValue:[NSString stringWithUTF8String:_settingsData.props[i].title]];
                [lbl setBezeled:NO];
                [lbl setDrawsBackground:NO];
                [lbl setEditable:NO];
                [lbl setSelectable:NO];

                // resize label to fit content
                [[lbl cell] setLineBreakMode:NSLineBreakByClipping];
                label_w = [[lbl cell] cellSizeForBounds:lbl.bounds].width;
                [lbl setFrameSize:NSMakeSize(label_w, unit_h)];
                label_w += label_padding;

                [view addSubview:lbl];
            }
        }

        // set entry
        NSString *propname = [NSString stringWithUTF8String:_settingsData.props[i].key];
        NSString *def = [NSString stringWithUTF8String:_settingsData.props[i].def];
        NSString *value = [accessor getValueForKey:propname def:def];

        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_FILE:
            {
                int right_offs = 0;
                if (_settingsData.props[i].type == PROP_FILE) {
                    right_offs = 24;
                }
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width-label_w - label_padding - 4 - right_offs, unit_h);
                NSTextField *tf = _settingsData.props[i].type == PROP_PASSWORD ? [[NSSecureTextField alloc] initWithFrame:frame] : [[NSTextField alloc] initWithFrame:frame];
                [tf setUsesSingleLineMode:YES];
                [tf setStringValue:value];
                [view addSubview:tf];
                [_bindings addObject:@{@"sender":tf,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];
                [tf setDelegate:(id<NSTextFieldDelegate>)self];
                break;
            }
            case PROP_CHECKBOX:
            {
                NSRect frame = NSMakeRect(4, y, sz.width - 4, unit_h);
                NSButton *checkbox = [[NSButton alloc] initWithFrame:frame];
                [checkbox setButtonType:NSSwitchButton];
                [checkbox setTitle:[NSString stringWithUTF8String:_settingsData.props[i].title]];
                [checkbox setState:[value intValue] ? NSOnState : NSOffState];
                [view addSubview:checkbox];

                [_bindings addObject:@{@"sender":checkbox,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [checkbox setTarget:self];
                [checkbox setAction:@selector(valueChanged:)];

                break;
            }
            case PROP_SLIDER:
            {
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width - (label_w + label_padding) - 72, unit_h);
                NSSlider *slider = [[NSSlider alloc] initWithFrame:frame];
                const char *opts = _settingsData.props[i].select_options;
                float min, max, step;
                sscanf (opts, "%f,%f,%f", &min, &max, &step);

                if (min > max) {
                    [slider setMinValue:max];
                    [slider setMaxValue:min];
                }
                else {
                    [slider setMinValue:min];
                    [slider setMaxValue:max];
                }
                [slider setContinuous:YES];
                if (step == 1) {
                    [slider setIntValue:[value intValue]];
                }
                else {
                    [slider setFloatValue:[value floatValue]];
                }

                frame = NSMakeRect(label_w + sz.width-label_w - label_padding - 64, y, 68, unit_h);
                NSTextField *valueedit = [[NSTextField alloc] initWithFrame:frame];
#if 0
                NSNumberFormatter *fmt = [[NSNumberFormatter alloc] init];
                if (step == 1) {
                    fmt.allowsFloats = NO;
                }
                else {
                    fmt.allowsFloats = YES;
                }
                fmt.minimum = [NSNumber numberWithFloat:min];
                fmt.maximum = [NSNumber numberWithFloat:max];
                [valueedit setFormatter:fmt];
#endif
                [valueedit setStringValue:value];
                [valueedit setEditable:YES];

                [_bindings addObject:@{@"sender":slider,
                                       @"propname":propname,
                                       @"valueview":valueedit,
                                       @"isInteger":[NSNumber numberWithBool:step == 1 ? YES:NO],
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [_bindings addObject:@{@"sender":valueedit,
                                       @"propname":propname,
                                       @"valueview":slider,
                                       @"isInteger":[NSNumber numberWithBool:YES],
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [slider setTarget:self];
                [slider setAction:@selector(valueChanged:)];

                [valueedit setDelegate:(id<NSTextFieldDelegate>)self];

                [view addSubview:slider];
                [view addSubview:valueedit];
                break;
            }
            case PROP_SELECT:
            {
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width-label_w - label_padding - 4, unit_h);
                NSPopUpButton *popUpButton = [[NSPopUpButton alloc] initWithFrame:frame];

                char token[MAX_TOKEN];
                const char *script = _settingsData.props[i].select_options;

                int selectedIdx = [value intValue];

                while ((script = gettoken (script, token)) && strcmp (token, ";")) {
                    [popUpButton addItemWithTitle:[NSString stringWithUTF8String:token]];
                    if (selectedIdx == [popUpButton numberOfItems]-1) {
                        [popUpButton selectItemAtIndex:[popUpButton numberOfItems]-1];
                    }
                }

                [_bindings addObject:@{@"sender":popUpButton,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [popUpButton setTarget:self];
                [popUpButton setAction:@selector(valueChanged:)];

                [view addSubview:popUpButton];
                break;
            }
            case PROP_ITEMLIST:
            {
                // This should add a proper list view with a backing VC, with add/remove/edit buttons attached to it
                ItemListViewController *vc = [[ItemListViewController alloc] initWithProp:&_settingsData.props[i] accessor:accessor];
                [_bindings addObject:@{@"sender":vc,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];
                NSRect frame = NSMakeRect(0, 0, sz.width, sz.height);
                [[vc view] setFrame:frame];
                [view addSubview:[vc view]];
                [[vc view] setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewMinYMargin|NSViewHeightSizable|NSViewMaxYMargin];
                break;
            }
        }
    }
    // FIXME
    conf_unlock ();
}

- (void)savePluginConfiguration {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] && binding[@"propname"]) {
            id sender = binding[@"sender"];
            NSString *value;
            if (binding[@"isInteger"] && [binding[@"isInteger"] boolValue]) {
                value = [@([sender integerValue]) stringValue];
            }
            else if ([sender isKindOfClass:[NSPopUpButton class]]) {
                value = [@([sender indexOfSelectedItem]) stringValue];
            }
            else {
                value = [sender stringValue];
            }
            [_accessor setValueForKey:binding[@"propname"] value:value];
        }
    }
    // FIXME: apply current preset (ask streamer to reload it)
    // deadbeef->streamer_set_dsp_chain (_curr_dsp);
}

- (void)resetPluginConfigToDefaults {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] && binding[@"default"]) {
            id sender = binding[@"sender"];
            if ([sender isKindOfClass:[NSPopUpButton class]]) {
                [sender selectItemAtIndex:[binding[@"default"] intValue]];
            }
            else if ([sender isKindOfClass:[ItemListViewController class]]) {
                // FIXME
            }
            else {
                [sender setStringValue:binding[@"default"]];
            }
        }
    }

    [self savePluginConfiguration];
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    [self valueChanged:textField];
}

- (void)valueChanged:(id)sender {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] == sender) {
            // synchronize dependent widgets, e.g. slider with its textfield
            if (binding[@"valueview"]) {
                if (binding[@"isInteger"] && [binding[@"isInteger"] boolValue]) {
                    [binding[@"valueview"] setStringValue:[@([sender integerValue]) stringValue]];
                }
                else {
                    [binding[@"valueview"] setStringValue:[sender stringValue]];
                }
            }

            // update config
            NSString *value;
            if (binding[@"isInteger"] && [binding[@"isInteger"] boolValue]) {
                value = [@([sender integerValue]) stringValue];
            }
            else if ([sender isKindOfClass:[NSPopUpButton class]]) {
                value = [@([sender indexOfSelectedItem]) stringValue];
            }
            else {
                value = [sender stringValue];
            }
            [_accessor setValueForKey:binding[@"propname"] value:value];

            break;
        }
    }
}

@end
