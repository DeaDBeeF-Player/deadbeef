#import "PluginConfigurationViewController.h"
#include "pluginsettings.h"
#include "parser.h"

extern DB_functions_t *deadbeef;

@interface PluginConfigurationViewController () {
    NSMutableArray *_bindings;
    ddb_dsp_context_t *_curr_dsp;
    settings_data_t _settingsData;
}

@end

@implementation PluginConfigurationViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}

- (void)initPluginConfiguration:(const char *)config dsp:(ddb_dsp_context_t *)dsp {
    NSView *view = [self view];
    _bindings = [[NSMutableArray alloc] init];
    _curr_dsp = dsp;

    NSScrollView *scrollView = (NSScrollView *)view;
    view = [scrollView documentView];

    while ([[view subviews] count] > 0) {
        [[[view subviews] lastObject] removeFromSuperview];
    }

    settings_data_free (&_settingsData);

    BOOL have_settings = YES;
    if (!config || settings_data_init(&_settingsData, config) < 0) {
        have_settings = NO;
    }

    int label_padding = 8;
    int unit_spacing = 4;
    int unit_h = 22;
    int h = _settingsData.nprops * (unit_h + unit_spacing);

    NSSize sz = [scrollView contentSize];

    if (h < sz.height) {
        h = sz.height;
    }
    NSRect frame = [view frame];
    [view setFrame:NSMakeRect(0, 0, frame.size.width, h)];

    NSPoint pt = NSMakePoint(0.0, [[scrollView documentView]
                                   bounds].size.height);
    [[scrollView documentView] scrollPoint:pt];

    sz = [scrollView contentSize];

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

    deadbeef->conf_lock ();
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
        char value[1000];
        if (dsp) {
            int param = atoi (_settingsData.props[i].key);
            dsp->plugin->get_param (dsp, param, value, sizeof (value));
        }
        else {
            deadbeef->conf_get_str (_settingsData.props[i].key, _settingsData.props[i].def, value, sizeof (value));
        }
        NSString *propname = [NSString stringWithUTF8String:_settingsData.props[i].key];

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
                [tf setStringValue:[NSString stringWithUTF8String:value]];
                [view addSubview:tf];
                [_bindings addObject:@{@"sender":tf,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];
                tf.delegate = self;
                break;
            }
            case PROP_CHECKBOX:
            {
                NSRect frame = NSMakeRect(4, y, sz.width - 4, unit_h);
                NSButton *checkbox = [[NSButton alloc] initWithFrame:frame];
                [checkbox setButtonType:NSSwitchButton];
                [checkbox setTitle:[NSString stringWithUTF8String:_settingsData.props[i].title]];
                [checkbox setState:atoi(value) ? NSOnState : NSOffState];
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
                    [slider setIntValue:atoi(value)];
                }
                else {
                    [slider setFloatValue:atof(value)];
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
                [valueedit setStringValue:[NSString stringWithUTF8String: value]];
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

                valueedit.delegate = self;

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

                int selectedIdx = atoi (value);

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
        }
    }
    deadbeef->conf_unlock ();
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
            const char *propname = [binding[@"propname"] UTF8String];
            const char *svalue = [value UTF8String];
            if (_curr_dsp) {
                _curr_dsp->plugin->set_param (_curr_dsp, atoi (propname), svalue);
            }
            else {
                deadbeef->conf_set_str (propname, svalue);
            }
        }
    }
    deadbeef->streamer_set_dsp_chain (_curr_dsp);
}

- (void)resetPluginConfigToDefaults {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] && binding[@"default"]) {
            id sender = binding[@"sender"];
            if ([sender isKindOfClass:[NSPopUpButton class]]) {
                [sender selectItemAtIndex:[binding[@"default"] intValue]];
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
            const char *propname = [binding[@"propname"] UTF8String];
            const char *svalue = [value UTF8String];
            if (_curr_dsp) {
                _curr_dsp->plugin->set_param (_curr_dsp, atoi (propname), svalue);
                deadbeef->streamer_set_dsp_chain (_curr_dsp);
            }
            else {
                deadbeef->conf_set_str (propname, svalue);
                deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
                deadbeef->conf_save ();
            }

            break;
        }
    }
}

@end
