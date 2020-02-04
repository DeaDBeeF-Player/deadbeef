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

@property (nonatomic,readwrite) NSSize calculatedSize;

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
        [self reload];
    }
}

- (void)reload {
    for (int i = 0; i < _settingsData.nprops; i++) {
        // find binding with the same propname
        for (NSDictionary *binding in _bindings) {
            if (_settingsData.props[i].key
                && [binding[@"propname"] isEqualToString:[NSString stringWithUTF8String:_settingsData.props[i].key]]) {
                NSControl *ctl = binding[@"sender"];
                NSString *key = [NSString stringWithUTF8String:_settingsData.props[i].key];
                NSString *def = [NSString stringWithUTF8String:_settingsData.props[i].def];
                NSString *value = [self.dataSource propertySheet:self valueForKey:key def:def item:self.item];
                if ([ctl isKindOfClass:[NSPopUpButton class]]) {
                    NSPopUpButton *pb = (NSPopUpButton *)ctl;
                    pb.title = value;
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
    NSView *view;
    _bindings = [NSMutableArray new];

    settings_data_free (&_settingsData);

    BOOL have_settings = YES;
    NSString *config = [dataSource propertySheet:self configForItem:self.item];
    if (!config || settings_data_init(&_settingsData, [config UTF8String]) < 0) {
        have_settings = NO;
    }

    CGFloat label_width = _labelFixedWidth;
    int padding = 4;
    CGFloat unit_h = _contentFontSize + 11;
    CGFloat h = _settingsData.nprops * (unit_h + _unitSpacing) + _topMargin;
    NSSize sz;

    self.calculatedSize = NSMakeSize(NSWidth(self.view.frame), h);

    if ([self.view isKindOfClass:[NSScrollView class]]) {
        NSScrollView *scrollView = (NSScrollView *)self.view;
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
        view = self.view;
        sz = view.frame.size;
        h = sz.height;
    }

    while (view.subviews.count > 0) {
        [view.subviews.lastObject removeFromSuperview];
    }

    if (!have_settings) {
        NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(0, sz.height/2 - unit_h/2, sz.width, unit_h)];
        lbl.stringValue = @"No properties available";
        lbl.alignment = NSTextAlignmentCenter;
        lbl.bezeled = NO;
        lbl.drawsBackground = NO;
        lbl.editable = NO;
        lbl.selectable = NO;
        lbl.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin|NSViewMaxYMargin;
        [view addSubview:lbl];
        return;
    }

    NSFont *fontLabel = [NSFont systemFontOfSize:_labelFontSize weight:NSFontWeightRegular];
    NSFont *fontContent = [NSFont systemFontOfSize:_contentFontSize weight:NSFontWeightRegular];

    if (_autoAlignLabels) {
        label_width = 0;
        NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
        textStyle.alignment = NSTextAlignmentLeft;
        textStyle.lineBreakMode = NSLineBreakByTruncatingTail;

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


    NSStackView *sv;
    int svItemCount = 0;
    BOOL vert = NO;

    CGFloat topLevelY = h - (unit_h + _unitSpacing) - _topMargin;
    for (int i = 0; i < _settingsData.nprops; i++) {

        CGFloat calculated_label_w = 0;

        if (_settingsData.props[i].type == PROP_HBOX) {
            // FIXME: disabled h/vboxes inside of scrollviews to prevent breaking plugin configuration
            if ([self.view isKindOfClass:NSScrollView.class]) {
                continue;
            }

            // FIXME: reuse for vbox
            // parse parameters
            char token[MAX_TOKEN];
            const char *script = _settingsData.props[i].select_options;

            // count
            script = gettoken (script, token);
            if (!script) {
                continue;
            }

            svItemCount = atoi (token);

            int hmg = 1;
            int fill = 1;
            int expand = 1;
            int border = 0;
            int spacing = 0;
            int height = (int)unit_h;

            // other args
            while ((script = gettoken (script, token))) {
                if (!strcmp (token, "hmg")) {
                    hmg = 1;
                }
                else if (!strcmp (token, "fill")) {
                    fill = 1;
                }
                else if (!strcmp (token, "expand")) {
                    expand = 1;
                }
                else if (!strncmp (token, "border=", 7)) {
                    border = atoi(token+7);
                }
                else if (!strncmp (token, "spacing=", 8)) {
                    spacing = atoi (token+8);
                }
                else if (!strncmp (token, "height=", 7)) {
                    height = atoi (token+7);
                }
            }


            topLevelY -= (unit_h + _unitSpacing);
            sv = [[NSStackView alloc] initWithFrame:NSMakeRect(padding, topLevelY-height, view.frame.size.width-padding*2, height)];
            sv.orientation = NSUserInterfaceLayoutOrientationHorizontal;
            sv.distribution = NSStackViewDistributionFillEqually;
            sv.autoresizingMask = NSViewMinYMargin|NSViewMaxYMargin|NSViewMaxXMargin|NSViewMinXMargin|NSViewWidthSizable;
            sv.spacing = 0;
            [self.view addSubview:sv];

            svItemCount = atoi (_settingsData.props[i].select_options);
            view = sv;
            h = height-topLevelY;
            self.calculatedSize = NSMakeSize(600, height); // FIXME: hardcoded width, should be calculated from content width
            topLevelY += height;
            vert = YES;
            continue;
        }
        else if (_settingsData.props[i].type == PROP_VBOX) {
            continue;
        }

        CGFloat y = topLevelY;
        if (svItemCount) {
            view = [[NSView alloc] initWithFrame:NSMakeRect(0,0,50,200)];
            [sv addView:view inGravity:NSStackViewGravityLeading];
            svItemCount--;
            y = 0;
        }

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
                lbl.stringValue = title;
                lbl.bezeled = NO;
                lbl.drawsBackground = NO;
                lbl.editable = NO;
                lbl.selectable = NO;
                lbl.toolTip = title;
                lbl.cell.truncatesLastVisibleLine = YES;
                lbl.cell.scrollable = NO;
                lbl.cell.wraps = NO;
                if (_autoAlignLabels) {
                    lbl.cell.lineBreakMode = NSLineBreakByClipping;
                }
                else {
                    lbl.cell.lineBreakMode = NSLineBreakByTruncatingTail;
                }

                lbl.font = fontLabel;

                // resize label to fit content
                calculated_label_w = [[lbl cell] cellSizeForBounds:lbl.bounds].width+1;

                NSRect frame;
                if (!vert) {
                    frame = NSMakeRect(padding+label_width-calculated_label_w, y+3, calculated_label_w, unit_h-6);
                }
                else {
                    frame = NSMakeRect(3, 200, 50, unit_h-6);
                    lbl.frameRotation = -90;
                }

                lbl.frame = frame;
                lbl.autoresizingMask = NSViewMaxXMargin;

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
                CGFloat w = sz.width-label_width - padding*3;
                if (_settingsData.props[i].type == PROP_FILE || _settingsData.props[i].type == PROP_DIR) {
                    w -= 12+padding;
                }
                NSRect frame = NSMakeRect(label_width+padding*2, y+3, w, unit_h-3);
                NSTextField *tf = _settingsData.props[i].type == PROP_PASSWORD ? [[NSSecureTextField alloc] initWithFrame:frame] : [[NSTextField alloc] initWithFrame:frame];
                tf.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin;
                tf.font = fontContent;
                tf.usesSingleLineMode = YES;
                tf.stringValue = value;
                [view addSubview:tf];
                [_bindings addObject:@{@"sender":tf,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];
                tf.target = self;
                tf.delegate = self;

                if (_settingsData.props[i].type == PROP_FILE || _settingsData.props[i].type == PROP_DIR) {
                    BrowseButton *btn = [[BrowseButton alloc] initWithFrame:NSMakeRect(label_width+padding*3+w, y+unit_h/2-5, 12, 10)];
                    btn.autoresizingMask = NSViewMinYMargin|NSViewMinXMargin;
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
                checkbox.buttonType = NSButtonTypeSwitch;
                checkbox.title = [NSString stringWithUTF8String:_settingsData.props[i].title];
                checkbox.state = [value intValue] ? NSControlStateValueOn : NSControlStateValueOff;
                checkbox.font = fontContent;
                checkbox.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin;
                [view addSubview:checkbox];

                [_bindings addObject:@{@"sender":checkbox,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                checkbox.target = self;
                checkbox.action = @selector(valueChanged:);

                break;
            }
            case PROP_SLIDER:
            {
                CGFloat w = sz.width-label_width - padding*3;
                NSRect frame;
                if (!vert) {
                    frame = NSMakeRect(label_width+padding*2, y+3, w - _sliderLabelWidth-8, unit_h-3);
                }
                else {
                    frame = NSMakeRect(0, unit_h-3, 50, 200-40-(unit_h-3));
                }
                NSSlider *slider = [[NSSlider alloc] initWithFrame:frame];
                if (vert) {
                    slider.vertical = YES;
                }
                slider.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin;
                const char *opts = _settingsData.props[i].select_options;
                float min, max, step;
                sscanf (opts, "%f,%f,%f", &min, &max, &step);

                if (min > max) {
                    slider.minValue = max;
                    slider.maxValue = min;
                }
                else {
                    slider.minValue = min;
                    slider.maxValue = max;
                }
                slider.continuous = YES;
                if (step == 1) {
                    slider.intValue = [value intValue];
                }
                else {
                    slider.floatValue = [value floatValue];
                }

                NSRect frame2;
                if (!vert) {
                    frame2 = NSMakeRect(label_width+padding*2 + w - _sliderLabelWidth - 4, y+3, _sliderLabelWidth, unit_h-3);
                }
                else {
                    frame2 = NSMakeRect(0, 0, 50, unit_h-3);
                }

                NSTextField *valueedit = [[NSTextField alloc] initWithFrame:frame2];
                valueedit.font = fontContent;
                valueedit.autoresizingMask = NSViewMinYMargin|NSViewMinXMargin;
                if (!vert) {
                    valueedit.editable = YES;
                }
                else {
                    valueedit.bezeled = NO;
                    valueedit.drawsBackground = NO;
                    valueedit.editable = NO;
                    valueedit.selectable = NO;
                    valueedit.cell.truncatesLastVisibleLine = YES;
                    valueedit.cell.scrollable = NO;
                    valueedit.cell.wraps = NO;
                    valueedit.font = fontLabel;
                    valueedit.alignment = NSTextAlignmentCenter;

                    NSNumberFormatter *f = [[NSNumberFormatter alloc] init];
                    f.allowsFloats = YES;
                    f.alwaysShowsDecimalSeparator = NO;
                    f.numberStyle = NSNumberFormatterDecimalStyle;
                    f.maximumFractionDigits = 2;
                    valueedit.cell.formatter = f;

                    valueedit.autoresizingMask = NSViewMinXMargin|NSViewMaxXMargin|NSViewWidthSizable;
                }
                valueedit.stringValue = value;

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

                slider.target = self;
                slider.action = @selector(valueChanged:);

                valueedit.delegate = (id<NSTextFieldDelegate>)self;

                [view addSubview:slider];
                [view addSubview:valueedit];
                break;
            }
            case PROP_SELECT:
            {
                NSRect frame = NSMakeRect(label_width+padding*2, y, sz.width-label_width - padding*2 - padding, unit_h);
                NSPopUpButton *popUpButton = [[NSPopUpButton alloc] initWithFrame:frame];
                popUpButton.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin;
                popUpButton.font = fontContent;

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

                popUpButton.target = self;
                popUpButton.action = @selector(valueChanged:);

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
                vc.view.frame = frame;
                [view addSubview:vc.view];
                vc.view.autoresizingMask = NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewMinYMargin|NSViewHeightSizable|NSViewMaxYMargin;
                break;
            }
#endif
            case PROP_ITEMSELECT:
            {
                NSRect frame = NSMakeRect(label_width+padding*2, y, sz.width-label_width - padding*2 - padding, unit_h);
                NSPopUpButton *popUpButton = [[NSPopUpButton alloc] initWithFrame:frame];
                popUpButton.font = fontContent;

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

                popUpButton.target = self;
                popUpButton.action = @selector(valueChanged:);
                popUpButton.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin;

                [view addSubview:popUpButton];
                break;            }
        }
        if (!svItemCount && sv) {
            if ([self.view isKindOfClass:[NSScrollView class]]) {
                NSScrollView *scrollView = (NSScrollView *)self.view;
                view = [scrollView documentView];
            }
            else {
                view = self.view;
            }
            sv = nil;
        }
        else {
            topLevelY -= unit_h + _unitSpacing;
        }
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
            NSControl *sender = binding[@"sender"];
            if ([sender isKindOfClass:[NSPopUpButton class]]) {
                [((NSPopUpButton *)sender) selectItemAtIndex:[binding[@"default"] intValue]];
            }
#if 0
            else if ([sender isKindOfClass:[ItemListViewController class]]) {
                // FIXME
            }
#endif
            else {
                sender.stringValue = binding[@"default"];
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
                    ((NSControl *)binding[@"valueview"]).stringValue = [@([sender integerValue]) stringValue];
                }
                else {
                    ((NSControl *)binding[@"valueview"]).stringValue = [sender stringValue];
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
