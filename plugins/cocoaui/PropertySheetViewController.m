#import "PropertySheetViewController.h"
// FIXME: needed for referencing other object lists
//#import "ItemListViewController.h"
#include "parser.h"
#include "pluginsettings.h"
#import "BrowseButton.h"

@interface PropertySheetViewController () {
    NSMutableArray *_bindings;
    settings_data_t _settingsData;
}
@end

@implementation PropertySheetViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do view setup here.
    _labelFontSize = [NSFont systemFontSize];
    _contentFontSize = [NSFont systemFontSize];
    _topMargin = 0;
    _autoAlignLabels = YES;
    _labelFixedWidth = 76;
    _sliderLabelWidth = 76;
    _unitSpacing = 8;

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(projectDataItemChanged:) name:@"ProjectDataItemChanged" object:nil];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)projectDataItemChanged:(NSNotification *)notification {
    if ([notification.userInfo objectForKey:@"sender"] == self) {
        return;
    }
    id obj = notification.object;
    if (obj == self.item) {
        // reload properties
        [self reloadProperties];
    }
}

- (void)reloadProperties {
    for (int i = 0; i < _settingsData.nprops; i++) {
        // find binding with the same propname
        for (NSDictionary *binding in _bindings) {
            if ([binding[@"propname"] isEqualToString:[NSString stringWithUTF8String:_settingsData.props[i].key]]) {
                NSControl *ctl = binding[@"sender"];
                NSString *key = [NSString stringWithUTF8String:_settingsData.props[i].key];
                NSString *def = [NSString stringWithUTF8String:_settingsData.props[i].def];
                NSString *value = [self.dataSource propertySheet:self valueForKey:key def:def item:self.item];
                if ([ctl isKindOfClass:[NSPopUpButton class]]) {
                    NSPopUpButton *pb = (NSPopUpButton *)ctl;
                    [pb setTitle:value];
                }
                else {
                    ctl.stringValue = value;
                }
            }
        }
    }
}

- (void)setDataSource:(id<PropertySheetDataSource>)dataSource {
    _dataSource = dataSource;
    NSView *view = [self view];
    _bindings = [[NSMutableArray alloc] init];

    settings_data_free (&_settingsData);

    BOOL have_settings = YES;
    NSString *config = [dataSource propertySheet:self configForItem:self.item];
    if (!config || settings_data_init(&_settingsData, [config UTF8String]) < 0) {
        have_settings = NO;
    }

    NSInteger label_width = _labelFixedWidth;
    int padding = 4;
    NSInteger unit_h = _contentFontSize + 11;
    NSInteger h = _settingsData.nprops * (unit_h + _unitSpacing) + _topMargin;
    NSSize sz;

    if ([view isKindOfClass:[NSScrollView class]]) {
        NSScrollView *scrollView = (NSScrollView *)view;
        view = [scrollView documentView];
        NSRect rc = view.frame;
        if (_settingsData.nprops) {
            rc.size.height = h;
        }
        else {
            rc.size.height = [scrollView contentView].frame.size.height;
        }
        view.frame = rc;

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
        [lbl setStringValue:@"No properties available"];
        [lbl setAlignment:NSTextAlignmentCenter];
        [lbl setBezeled:NO];
        [lbl setDrawsBackground:NO];
        [lbl setEditable:NO];
        [lbl setSelectable:NO];
        [lbl setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin|NSViewMaxYMargin];
        [view addSubview:lbl];
        return;
    }

    NSFont *fontLabel = [NSFont systemFontOfSize:_labelFontSize weight:NSFontWeightRegular];
    NSFont *fontContent = [NSFont systemFontOfSize:_contentFontSize weight:NSFontWeightRegular];

    if (_autoAlignLabels) {
        label_width = 0;
        NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
        [textStyle setAlignment:NSLeftTextAlignment];
        [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];

        NSDictionary *attrs = @{
                             NSParagraphStyleAttributeName: textStyle,
                             NSFontAttributeName:fontLabel
                             };
        for (int i = 0; i < _settingsData.nprops; i++) {
            // set label
            switch (_settingsData.props[i].type) {
                case PROP_ENTRY:
                case PROP_PASSWORD:
                case PROP_SELECT:
                case PROP_SLIDER:
                case PROP_FILE:
                case PROP_DIR:
                case PROP_ITEMLIST:
                case PROP_ITEMSELECT: {
                    NSString *title = [NSString stringWithUTF8String:_settingsData.props[i].title];
                    NSSize size = [title sizeWithAttributes:attrs];
                    if (size.width > label_width) {
                        label_width = size.width;
                    }
                    break;
                }
            }
        }
        label_width += 10;
    }

    NSInteger y = h - (unit_h + _unitSpacing) - _topMargin;
    for (int i = 0; i < _settingsData.nprops; i++) {

        int calculated_label_w = 0;

        // set label
        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_SELECT:
            case PROP_SLIDER:
            case PROP_FILE:
            case PROP_DIR:
            case PROP_ITEMLIST:
            case PROP_ITEMSELECT:
            {
                NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(padding, y+3, label_width, unit_h-6)];
                NSString *title = [NSString stringWithUTF8String:_settingsData.props[i].title];
                [lbl setStringValue:title];
                [lbl setBezeled:NO];
                [lbl setDrawsBackground:NO];
                [lbl setEditable:NO];
                [lbl setSelectable:NO];
                [lbl setToolTip:title];
                lbl.cell.truncatesLastVisibleLine = YES;
                lbl.cell.scrollable = NO;
                lbl.cell.wraps = NO;
                if (_autoAlignLabels) {
                    lbl.cell.lineBreakMode = NSLineBreakByClipping;
                }
                else {
                    lbl.cell.lineBreakMode = NSLineBreakByTruncatingTail;
                }

                [lbl setFont:fontLabel];

                // resize label to fit content
                calculated_label_w = [[lbl cell] cellSizeForBounds:lbl.bounds].width+1;
                [lbl setFrame:NSMakeRect(padding+label_width-calculated_label_w, y+3, calculated_label_w, unit_h-6)];
                [lbl setAutoresizingMask:NSViewMinYMargin];

                [view addSubview:lbl];
            }
        }

        // set entry
        NSString *propname = [NSString stringWithUTF8String:_settingsData.props[i].key];
        NSString *def = [NSString stringWithUTF8String:_settingsData.props[i].def];
        NSString *value = [self.dataSource propertySheet:self valueForKey:propname def:def item:self.item];

        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_FILE:
            case PROP_DIR:
            {
                int w = sz.width-label_width - padding*3;
                if (_settingsData.props[i].type == PROP_FILE || _settingsData.props[i].type == PROP_DIR) {
                    w -= 12+padding;
                }
                NSRect frame = NSMakeRect(label_width+padding*2, y+3, w, unit_h-3);
                NSTextField *tf = _settingsData.props[i].type == PROP_PASSWORD ? [[NSSecureTextField alloc] initWithFrame:frame] : [[NSTextField alloc] initWithFrame:frame];
                [tf setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin];
                [tf setFont:fontContent];
                [tf setUsesSingleLineMode:YES];
                [tf setStringValue:value];
                [view addSubview:tf];
                [_bindings addObject:@{@"sender":tf,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];
                tf.target = self;
                tf.delegate = self;

                if (_settingsData.props[i].type == PROP_FILE || _settingsData.props[i].type == PROP_DIR) {
                    BrowseButton *btn = [[BrowseButton alloc] initWithFrame:NSMakeRect(label_width+padding*3+w, y+unit_h/2-5, 12, 10)];
                    [btn setAutoresizingMask:NSViewMinYMargin|NSViewMinXMargin];
                    btn.isDir = _settingsData.props[i].type == PROP_DIR;
                    btn.initialPath = value;
                    btn.fileSelectedBlock = ^(NSString * _Nonnull path) {
                        tf.stringValue = path;
                        [self valueChanged:tf];
                    };
                    [view addSubview:btn];
                }

                break;
            }
            case PROP_CHECKBOX:
            {
                NSRect frame = NSMakeRect(label_width+padding*2, y+3, sz.width-label_width - padding*3, unit_h-3);
                NSButton *checkbox = [[NSButton alloc] initWithFrame:frame];
                [checkbox setButtonType:NSButtonTypeSwitch];
                [checkbox setTitle:[NSString stringWithUTF8String:_settingsData.props[i].title]];
                [checkbox setState:[value intValue] ? NSControlStateValueOn : NSControlStateValueOff];
                [checkbox setFont:fontContent];
                [checkbox setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin];
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
                int w = sz.width-label_width - padding*3;
                NSRect frame = NSMakeRect(label_width+padding*2, y+3, w - _sliderLabelWidth-8, unit_h-3);
                NSSlider *slider = [[NSSlider alloc] initWithFrame:frame];
                [slider setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin];
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

                NSRect frame2 = NSMakeRect(label_width+padding*2 + w - _sliderLabelWidth - 4, y+3, _sliderLabelWidth, unit_h-3);
                NSTextField *valueedit = [[NSTextField alloc] initWithFrame:frame2];
                [valueedit setFont:fontContent];
                [valueedit setAutoresizingMask:NSViewMinYMargin|NSViewMinXMargin];
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
                NSRect frame = NSMakeRect(label_width+padding*2, y, sz.width-label_width - padding*2 - padding, unit_h);
                NSPopUpButton *popUpButton = [[NSPopUpButton alloc] initWithFrame:frame];
                [popUpButton setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin];
                [popUpButton setFont:fontContent];

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
#if 0
                // FIXME: needed for referencing other object lists
            case PROP_ITEMLIST:
            {
                // This should add a proper list view with a backing VC, with add/remove/edit buttons attached to it
                ItemListViewController *vc = [[ItemListViewController alloc] initWithProp:&_settingsData.props[i] scriptable:[accessor getScriptable]];
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
#endif
            case PROP_ITEMSELECT:
            {
                NSRect frame = NSMakeRect(label_width+padding*2, y, sz.width-label_width - padding*2 - padding, unit_h);
                NSPopUpButton *popUpButton = [[NSPopUpButton alloc] initWithFrame:frame];
                [popUpButton setFont:fontContent];

                [popUpButton addItemWithTitle:@""];

                [self.dataSource propertySheet:self enumerateItemsOfType:[NSString stringWithUTF8String:_settingsData.props[i].itemlist_type] block:^BOOL(id item) {
                    NSString *name = [self.dataSource propertySheet:self valueForKey:@"name" def:@"Undefined" item:item];
                    [popUpButton addItemWithTitle:name];
                    if ([value isEqualToString:name]) {
                        [popUpButton selectItemAtIndex:[popUpButton numberOfItems]-1];
                    }
                    return NO;
                }];

                [_bindings addObject:@{@"sender":popUpButton,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [popUpButton setTarget:self];
                [popUpButton setAction:@selector(valueChanged:)];
                [popUpButton setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin];

                [view addSubview:popUpButton];
                break;            }
        }
        y -= unit_h + _unitSpacing;
    }
}

- (void)save {
    [self.dataSource propertySheetBeginChanges];
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
            [self.dataSource propertySheet:self setValue:value forKey:binding[@"propname"] item:self.item];
        }
    }
    [self.dataSource propertySheetCommitChanges];
}

- (void)reset {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] && binding[@"default"]) {
            id sender = binding[@"sender"];
            if ([sender isKindOfClass:[NSPopUpButton class]]) {
                [sender selectItemAtIndex:[binding[@"default"] intValue]];
            }
#if 0
            else if ([sender isKindOfClass:[ItemListViewController class]]) {
                // FIXME
            }
#endif
            else {
                [sender setStringValue:binding[@"default"]];
            }
        }
    }

    [self save];
}

- (void)controlTextDidEndEditing:(NSNotification *)obj {
    NSTextField *tf = obj.object;
    [self valueChanged:tf];
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
                value = [sender titleOfSelectedItem];
            }
            else {
                value = [sender stringValue];
            }
            [self.dataSource propertySheet:self setValue:value forKey:binding[@"propname"] item:self.item];

            break;
        }
    }
}

@end
