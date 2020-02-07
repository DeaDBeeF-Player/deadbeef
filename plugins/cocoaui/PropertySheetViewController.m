#import "PropertySheetViewController.h"
// FIXME: needed for referencing other object lists
//#import "ItemListViewController.h"
#include "parser.h"
#include "pluginsettings.h"
#import "BrowseButton.h"

#pragma mark - BoxHandler

@interface BoxHandler : NSObject

@property (nonatomic) NSView *view;
@property (nonatomic) NSView *firstLabel;
@property (nonatomic) NSView *previousField;
@property (nonatomic) BOOL isVertical; // FIXME: this is a hack for vertical layout in horz stackview. Should be per item param.
@property (nonatomic) BOOL isStackView;
@end

@implementation BoxHandler

@end

#pragma mark - PropertySheetViewController

@interface PropertySheetViewController ()

@property (nonatomic,readwrite) NSSize calculatedSize;
@property (nonatomic) NSMutableArray *bindings;
@property (nonatomic) settings_data_t settingsData;

@property (nonatomic) NSFont *fontLabel;
@property (nonatomic) NSFont *fontContent;

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
                    if (binding[@"isInteger"] && [binding[@"isInteger"] boolValue]) {
                        ctl.integerValue = [value integerValue];
                    }
                    else if (binding[@"isFloat"] && [binding[@"isFloat"] boolValue]) {
                        ctl.floatValue = [value floatValue];
                    }
                    else {
                        ctl.stringValue = value;
                    }
                }
            }
        }
    }
}

- (void)constrainCurrentField:(NSView *)currentField boxHandler:(BoxHandler *)box currLabel:(NSView *)currLabel unalignedFields:(NSMutableArray<NSView *> *)unalignedFields view:(NSView *)view {
    if (currentField) {
        currentField.translatesAutoresizingMaskIntoConstraints = NO;

        if (box.firstLabel) {
            [currentField.leadingAnchor constraintEqualToAnchor:box.firstLabel.trailingAnchor constant:8].active=YES;
        }
        else {
            [unalignedFields addObject:currentField];
        }

        CGFloat leadingOffs = 20;

        if ([currentField isKindOfClass:NSStackView.class]) {
            leadingOffs = 0;
        }
        [currentField.leadingAnchor constraintGreaterThanOrEqualToAnchor:view.leadingAnchor constant:leadingOffs].active=YES;

        if (currLabel) {
            [currentField.firstBaselineAnchor constraintEqualToAnchor:currLabel.firstBaselineAnchor].active=YES;
        }
        else if (box.previousField) {
            [currentField.topAnchor constraintEqualToAnchor:box.previousField.bottomAnchor constant:8].active=YES;
        }
        else {
            CGFloat topOffs = 8;
            if ([currentField isKindOfClass:NSStackView.class]) {
                topOffs = 0;
            }
            [currentField.topAnchor constraintEqualToAnchor:view.topAnchor constant:topOffs].active=YES;
        }
        box.previousField = currentField;
    }
}

- (NSUInteger)addFieldsToBox:(BoxHandler *)box fromIndex:(NSUInteger)index count:(NSUInteger)count {
    NSView *view = box.view;
    CGFloat unit_h = _contentFontSize + 11;

    NSMutableArray<NSView *> *unalignedFields = [NSMutableArray new];


    for (NSUInteger i = index; i < index+count; i++) {

        if (_settingsData.props[i].type == PROP_HBOX) {
            NSStackView *sv;

            // FIXME: reuse for vbox
            // parse parameters
            char token[MAX_TOKEN];
            const char *script = _settingsData.props[i].select_options;

            // count
            script = gettoken (script, token);
            if (!script) {
                continue;
            }

            int nestedCount = atoi (token);

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


            sv = [NSStackView new];

            sv.orientation = NSUserInterfaceLayoutOrientationHorizontal;
            sv.distribution = NSStackViewDistributionFillEqually;
            sv.spacing = 0;
            sv.translatesAutoresizingMaskIntoConstraints = NO;
            [view addSubview:sv];

            BoxHandler *nestedBox = [BoxHandler new];
            nestedBox.view = sv;
            nestedBox.isStackView = YES;
            nestedBox.isVertical = YES;

            [nestedBox.view.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:0].active=YES;

            [self constrainCurrentField:sv boxHandler:box currLabel:nil unalignedFields:unalignedFields view:view];

            [sv.heightAnchor constraintEqualToConstant:height].active = YES;

            NSUInteger added = [self addFieldsToBox:nestedBox fromIndex:i+1 count:nestedCount];

            self.calculatedSize = NSMakeSize(600, height); // FIXME: hardcoded width, should be calculated from content width
            i += added;
            continue;
        }
        else if (_settingsData.props[i].type == PROP_VBOX) {
            continue;
        }

        if (box.isStackView) {
            view = [[NSView alloc] initWithFrame:NSMakeRect(0,0,50,200)];
            [(NSStackView *)box.view addView:view inGravity:NSStackViewGravityLeading];
        }

        NSView *currLabel;

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
                NSTextField *lbl = [NSTextField new];
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

                lbl.font = self.fontLabel;

                lbl.translatesAutoresizingMaskIntoConstraints = NO;
                [view addSubview:lbl];

                if (!box.isVertical) {
                    // horz alignment, same size labels
                    if (box.firstLabel) {
                        [lbl.leadingAnchor constraintEqualToAnchor:box.firstLabel.leadingAnchor].active = YES;
                        [lbl.trailingAnchor constraintEqualToAnchor:box.firstLabel.trailingAnchor].active = YES;
                    }
                    else {
                        [lbl.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:20].active = YES;
                        box.firstLabel = lbl;
                    }

                    // vert spacing
                    if (box.previousField) {
                        [lbl.topAnchor constraintEqualToAnchor:box.previousField.bottomAnchor constant:8].active = YES;
                    }
                    else {
                        [lbl.topAnchor constraintEqualToAnchor:view.topAnchor constant:8].active = YES;
                    }
                }
                else {
                    [lbl.topAnchor constraintEqualToAnchor:view.topAnchor constant:0].active = YES;
                    [lbl.centerXAnchor constraintEqualToAnchor:view.centerXAnchor constant:0].active = YES;
                    [lbl.leftAnchor constraintGreaterThanOrEqualToAnchor:view.leftAnchor constant:0].active = YES;
                    [lbl.rightAnchor constraintLessThanOrEqualToAnchor:view.rightAnchor constant:0].active = YES;
                }
                currLabel = lbl;
//                continue;
            }
        }

        // set entry
        NSString *propname = [NSString stringWithUTF8String:_settingsData.props[i].key];
        NSString *def = [NSString stringWithUTF8String:_settingsData.props[i].def];
        NSString *value = [self.dataSource propertySheet:self valueForKey:propname def:def item:self.item];

        NSView *currentField;

        switch (_settingsData.props[i].type) {
        case PROP_ENTRY:
        case PROP_PASSWORD:
        case PROP_FILE:
        case PROP_DIR:
            {
                NSTextField *tf = _settingsData.props[i].type == PROP_PASSWORD ? [NSSecureTextField new] : [NSTextField new];
                tf.font = self.fontContent;
                tf.usesSingleLineMode = YES;
                tf.stringValue = value;
                [view addSubview:tf];
                [_bindings addObject:@{@"sender":tf,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                }];
                tf.target = self;
                tf.delegate = self;

                currentField = tf;

                if (_settingsData.props[i].type == PROP_FILE || _settingsData.props[i].type == PROP_DIR) {
                    BrowseButton *btn = [BrowseButton new];
                    btn.isDir = _settingsData.props[i].type == PROP_DIR;
                    btn.initialPath = value;
                    btn.fileSelectedBlock = ^(NSString * _Nonnull path) {
                        tf.stringValue = path;
                        [self valueChanged:tf];
                    };
                    btn.translatesAutoresizingMaskIntoConstraints = NO;
                    [view addSubview:btn];

                    [btn.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20].active=YES;
                    [btn.leadingAnchor constraintEqualToAnchor:currentField.trailingAnchor constant:8].active=YES;
                    [btn.topAnchor constraintEqualToAnchor:currentField.topAnchor].active=YES;
                    [btn.bottomAnchor constraintEqualToAnchor:currentField.bottomAnchor].active=YES;
                }
                else {
                    [currentField.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20].active=YES;
                }

                break;
            }
        case PROP_CHECKBOX:
            {
                NSButton *checkbox = [NSButton new];
                checkbox.buttonType = NSButtonTypeSwitch;
                checkbox.title = [NSString stringWithUTF8String:_settingsData.props[i].title];
                checkbox.state = [value intValue] ? NSControlStateValueOn : NSControlStateValueOff;
                checkbox.font = self.fontContent;

                [view addSubview:checkbox];
                currentField = checkbox;

                [_bindings addObject:@{@"sender":checkbox,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                }];

                checkbox.target = self;
                checkbox.action = @selector(valueChanged:);

                [currentField.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20].active=YES;

                break;
            }
        case PROP_SLIDER:
            {
                NSSlider *slider = [NSSlider new];
                if (box.isVertical) {
                    slider.vertical = YES;
                }
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

                NSTextField *valueedit = [NSTextField new];
                valueedit.font = self.fontContent;
                if (!box.isVertical) {
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
                    valueedit.font = self.fontLabel;
                    valueedit.alignment = NSTextAlignmentCenter;

                    NSNumberFormatter *f = [[NSNumberFormatter alloc] init];
                    f.allowsFloats = YES;
                    f.alwaysShowsDecimalSeparator = NO;
                    f.numberStyle = NSNumberFormatterDecimalStyle;
                    f.maximumFractionDigits = 2;
                    valueedit.cell.formatter = f;

                }
                valueedit.floatValue = value.floatValue;

                [_bindings addObject:@{@"sender":slider,
                                       @"propname":propname,
                                       @"valueview":valueedit,
                                       @"isInteger":[NSNumber numberWithBool:step == 1 ? YES:NO],
                                       @"isFloat":[NSNumber numberWithBool:step != 1 ? YES:NO],
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                }];

                [_bindings addObject:@{@"sender":valueedit,
                                       @"propname":propname,
                                       @"valueview":slider,
                                       @"isInteger":[NSNumber numberWithBool:step == 1 ? YES:NO],
                                       @"isFloat":[NSNumber numberWithBool:step != 1 ? YES:NO],
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                }];

                slider.target = self;
                slider.action = @selector(valueChanged:);
                currentField = slider;

                valueedit.delegate = (id<NSTextFieldDelegate>)self;

                [view addSubview:slider];

                valueedit.translatesAutoresizingMaskIntoConstraints = NO;
                [view addSubview:valueedit];

                if (!box.isVertical) {
                    [valueedit.firstBaselineAnchor constraintEqualToAnchor:currentField.firstBaselineAnchor].active = YES;
                    [valueedit.widthAnchor constraintEqualToConstant:100].active = YES;
                    [valueedit.leadingAnchor constraintEqualToAnchor:currentField.trailingAnchor constant:8].active = YES;
                    [valueedit.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20].active = YES;
                }
                else {
                    slider.translatesAutoresizingMaskIntoConstraints = NO;
                    [slider.topAnchor constraintEqualToAnchor:currLabel.bottomAnchor constant:0].active = YES;
                    [slider.centerXAnchor constraintEqualToAnchor:view.centerXAnchor constant:0].active = YES;
                    [slider.leadingAnchor constraintGreaterThanOrEqualToAnchor:view.leadingAnchor constant:0].active = YES;
                    [slider.trailingAnchor constraintLessThanOrEqualToAnchor:view.trailingAnchor constant:0].active = YES;
                    [valueedit.topAnchor constraintEqualToAnchor:slider.bottomAnchor constant:0].active = YES;
                    [valueedit.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:0].active = YES;
                    [valueedit.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:0].active = YES;
                    [valueedit.bottomAnchor constraintEqualToAnchor:view.bottomAnchor constant:0].active = YES;
                }
                break;
            }
        case PROP_SELECT:
            {
                NSPopUpButton *popUpButton = [NSPopUpButton new];
                popUpButton.font = self.fontContent;

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


                popUpButton.translatesAutoresizingMaskIntoConstraints = NO;
                [view addSubview:popUpButton];
                currentField = popUpButton;
                [currentField.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20].active = YES;
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
                break;
            }
#endif
        case PROP_ITEMSELECT:
            {
                NSPopUpButton *popUpButton = [NSPopUpButton new];
                popUpButton.font = self.fontContent;

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

                popUpButton.translatesAutoresizingMaskIntoConstraints = NO;
                [view addSubview:popUpButton];
                currentField = popUpButton;
                [currentField.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20].active = YES;
                break;
            }
        }

        if (!box.isStackView) {
            [self constrainCurrentField:currentField boxHandler:box currLabel:currLabel unalignedFields:unalignedFields view:view];
        }
    }

    if (!box.isStackView) {
        for (NSView *field in unalignedFields) {
            if (box.firstLabel) {
                [field.leadingAnchor constraintEqualToAnchor:box.firstLabel.trailingAnchor constant:8].active=YES;
            }
            else {
                CGFloat leadingOffs = 20;
                if ([field isKindOfClass:NSStackView.class]) {
                    leadingOffs = 0;
                }
                [field.leadingAnchor constraintEqualToAnchor:field.superview.leadingAnchor constant:leadingOffs].active=YES;
            }
        }
    }

    return count;
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

//    CGFloat label_width = _labelFixedWidth;
    CGFloat unit_h = _contentFontSize + 11;

    CGFloat h = _settingsData.nprops * (unit_h + _unitSpacing) + _topMargin;
    NSSize sz;

    self.calculatedSize = NSMakeSize(NSWidth(self.view.frame), h);

    if ([self.view isKindOfClass:[NSScrollView class]]) {
        NSScrollView *scrollView = (NSScrollView *)self.view;
        view = [scrollView documentView];
        NSRect rc = view.frame;
        rc.size.width = scrollView.contentView.frame.size.width;
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
        NSTextField *lbl = [NSTextField new];
        lbl.stringValue = @"No properties available";
        lbl.alignment = NSTextAlignmentCenter;
        lbl.bezeled = NO;
        lbl.drawsBackground = NO;
        lbl.editable = NO;
        lbl.selectable = NO;
        lbl.translatesAutoresizingMaskIntoConstraints = NO;
        [view addSubview:lbl];
        [lbl.centerXAnchor constraintEqualToAnchor:view.centerXAnchor].active = YES;
        [lbl.centerYAnchor constraintEqualToAnchor:view.centerYAnchor].active = YES;
        return;
    }

    self.fontLabel = [NSFont systemFontOfSize:_labelFontSize weight:NSFontWeightRegular];
    self.fontContent = [NSFont systemFontOfSize:_contentFontSize weight:NSFontWeightRegular];

//    if (_autoAlignLabels) {
//        label_width = 0;
//        NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
//        textStyle.alignment = NSTextAlignmentLeft;
//        textStyle.lineBreakMode = NSLineBreakByTruncatingTail;
//
//        NSDictionary *attrs = @{
//                             NSParagraphStyleAttributeName: textStyle,
//                             NSFontAttributeName:self.fontLabel
//                             };
//        for (int i = 0; i < _settingsData.nprops; i++) {
//            // set label
//            switch (_settingsData.props[i].type) {
//                case PROP_ENTRY:
//                case PROP_PASSWORD:
//                case PROP_SELECT:
//                case PROP_SLIDER:
//                case PROP_FILE:
//                case PROP_DIR:
//                case PROP_ITEMLIST:
//                case PROP_ITEMSELECT: {
//                    NSString *title = [NSString stringWithUTF8String:_settingsData.props[i].title];
//                    NSSize size = [title sizeWithAttributes:attrs];
//                    if (size.width > label_width) {
//                        label_width = size.width;
//                    }
//                    break;
//                }
//            }
//        }
//        label_width += 10;
//    }


    BoxHandler *box = [BoxHandler new];
    box.view = view;

    [self addFieldsToBox:box fromIndex:0 count:self.settingsData.nprops];

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
            else if (binding[@"isFloat"] && [binding[@"isFloat"] boolValue]) {
                value = [@([sender floatValue]) stringValue];
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
                    ((NSControl *)binding[@"valueview"]).integerValue = [sender integerValue];
                }
                else if (binding[@"isFloat"] && [binding[@"isFloat"] boolValue]) {
                    ((NSControl *)binding[@"valueview"]).floatValue = [sender floatValue];
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
            else if (binding[@"isFloat"] && [binding[@"isFloat"] boolValue]) {
                value = [@([sender floatValue]) stringValue];
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
