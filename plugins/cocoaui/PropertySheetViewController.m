#import "PropertySheetViewController.h"
#import "PropertySheetContentView.h"
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
@property (nonatomic) BOOL isStackView;
@property (nonatomic) BOOL noclip;
@property (nonatomic) int itemWidth;

@end

@implementation BoxHandler

@end

#pragma mark - PropertySheetViewController

@interface PropertySheetViewController ()

@property (nonatomic,readwrite) NSSize contentSize;
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
    if ((notification.userInfo)[@"sender"] == self) {
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
                && [binding[@"propname"] isEqualToString:@(_settingsData.props[i].key)]) {
                NSControl *ctl = binding[@"sender"];
                NSString *key = @(_settingsData.props[i].key);
                NSString *def = @(_settingsData.props[i].def);
                NSString *value = [self.dataSource propertySheet:self valueForKey:key def:def item:self.item];
                if ([ctl isKindOfClass:[NSPopUpButton class]]) {
                    NSPopUpButton *pb = (NSPopUpButton *)ctl;
                    pb.title = value;
                }
                else {
                    if (binding[@"isInteger"] && [binding[@"isInteger"] boolValue]) {
                        ctl.integerValue = value.integerValue;
                    }
                    else if (binding[@"isFloat"] && [binding[@"isFloat"] boolValue]) {
                        ctl.floatValue = value.floatValue;
                    }
                    else {
                        ctl.stringValue = value;
                    }
                }
            }
        }
    }
}

- (void)constrainCurrentField:(NSView *)currentField isVert:(BOOL)isVert boxHandler:(BoxHandler *)box currLabel:(NSView *)currLabel unalignedFields:(NSMutableArray<NSView *> *)unalignedFields view:(NSView *)view {
    if (currentField) {
        currentField.translatesAutoresizingMaskIntoConstraints = NO;

        if ([currentField isKindOfClass:NSStackView.class]) {
            [currentField.leadingAnchor constraintEqualToAnchor:currentField.superview.leadingAnchor constant:0].active=YES;
        }
        else if (box.firstLabel) {
            if ([currentField isKindOfClass:NSButton.class]) {
                [currentField.leadingAnchor constraintLessThanOrEqualToAnchor:box.firstLabel.trailingAnchor constant:8].active=YES;
            }
            else {
                [currentField.leadingAnchor constraintEqualToAnchor:box.firstLabel.trailingAnchor constant:8].active=YES;
            }
        }
        else {
            [unalignedFields addObject:currentField];
        }

        CGFloat leadingOffs = 8;

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

- (NSUInteger)addFieldsToBox:(BoxHandler *)box fromIndex:(NSUInteger)index count:(NSUInteger)count isReadonly:(BOOL)isReadonly {
    NSView *view = box.view;
    CGFloat unit_h = _contentFontSize + 13;

    NSMutableArray<NSView *> *unalignedFields = [NSMutableArray new];

    CGFloat contentWidth = self.contentSize.width;
    CGFloat contentHeight = self.contentSize.height;

    NSUInteger i = index;
    NSUInteger remaining = count;

    if (box.isStackView && box.noclip) {
        NSStackView *stackview = (NSStackView *)box.view;
        NSView *spacingView = [NSView new];
        [stackview addArrangedSubview:spacingView];
    }

    while (remaining-- && i < (NSUInteger)_settingsData.nprops) {

        if (box.isStackView) {
            view = [NSView new];

            if (box.noclip) {
                view.wantsLayer = YES;
                view.layer = [CALayer new];
                view.layer.masksToBounds = NO;
            }

            NSStackView *stackview = (NSStackView *)box.view;
            view.autoresizingMask = NSViewMinXMargin | NSViewMaxXMargin;
            [stackview addArrangedSubview:view];
        }

        if (_settingsData.props[i].type == PROP_HBOX || _settingsData.props[i].type == PROP_VBOX) {
            NSStackView *sv;

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
            __unused int expand = 1;
            int border = 0;
            int spacing = 0;
            int width = -1;
            int height = -1;
            int noclip = 0;
            int itemwidth = -1;

            // other args
            while ((script = gettoken (script, token)) && strcmp (token, ";")) {
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
                else if (!strncmp (token, "width=", 6)) {
                    width = atoi (token+6);
                }
                else if (!strncmp (token, "height=", 7)) {
                    height = atoi (token+7);
                }
                else if (!strncmp (token, "noclip", 6)) {
                    noclip = 1;
                }
                else if (!strncmp (token, "itemwidth=", 10)) {
                    itemwidth = atoi (token+10);
                }
            }


            sv = [NSStackView new];

            sv.orientation = _settingsData.props[i].type == PROP_VBOX
            ? NSUserInterfaceLayoutOrientationVertical
            : NSUserInterfaceLayoutOrientationHorizontal;

            if (fill) {
                if (hmg) {
                    sv.distribution = NSStackViewDistributionFillEqually;
                }
                else {
                    sv.distribution = NSStackViewDistributionFill;
                }
            }
            sv.spacing = spacing;
            sv.edgeInsets = NSEdgeInsetsMake(border, border, border, border);

            sv.translatesAutoresizingMaskIntoConstraints = NO;
            [view addSubview:sv];

            BoxHandler *nestedBox = [BoxHandler new];
            nestedBox.view = sv;
            nestedBox.isStackView = YES;
            nestedBox.noclip = noclip;
            nestedBox.itemWidth = itemwidth;

            [nestedBox.view.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:0].active=YES;

            [self constrainCurrentField:sv isVert:NO boxHandler:box currLabel:nil unalignedFields:unalignedFields view:view];

            if (height != -1) {
                [sv.heightAnchor constraintEqualToConstant:height].active = YES;
            }

            i = [self addFieldsToBox:nestedBox fromIndex:i+1 count:nestedCount isReadonly:isReadonly];

            if (width != -1) {
                contentWidth = MAX(width, contentWidth);
            }
            if (height != -1) {
                contentHeight += height;
            }
            continue;
        }

        NSView *currLabel;

        BOOL isVert = NO;

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
                if (_settingsData.props[i].type == PROP_SLIDER) {
                    const char *opts = _settingsData.props[i].select_options;
                    if (opts) {
                        char token[MAX_TOKEN];
                        const char *script = opts;

                        // count
                        while ((script = gettoken (script, token)) && strcmp (token, ";")) {
                            if (!strcmp (token, "vert")) {
                                isVert = YES;
                            }
                        }
                    }
                }

                NSTextField *lbl = [NSTextField new];
                NSString *title = @(_settingsData.props[i].title);
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

                if (!isVert) {
                    // horz alignment, same size labels
                    if (box.firstLabel) {
                        [lbl.leadingAnchor constraintEqualToAnchor:box.firstLabel.leadingAnchor].active = YES;
                        [lbl.trailingAnchor constraintEqualToAnchor:box.firstLabel.trailingAnchor].active = YES;
                    }
                    else {
                        CGFloat offs = 8;
                        if (box.isStackView) {
                            offs = 0;
                        }
                        [lbl.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:offs].active = YES;
                        box.firstLabel = lbl;
                    }

                    // vert spacing
                    if (box.previousField) {
                        [lbl.topAnchor constraintEqualToAnchor:box.previousField.bottomAnchor constant:8].active = YES;
                    }
                    else {
                        [lbl.topAnchor constraintEqualToAnchor:view.topAnchor constant:8].active = YES;
                    }
                    [lbl setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationHorizontal];

                    [lbl setContentHuggingPriority:NSLayoutPriorityDefaultHigh-1 forOrientation:NSLayoutConstraintOrientationHorizontal];
                }
                else {
                    [lbl.topAnchor constraintEqualToAnchor:view.topAnchor constant:0].active = YES;
                    [lbl.centerXAnchor constraintEqualToAnchor:view.centerXAnchor constant:0].active = YES;
                    if (!box.noclip) {
                        [lbl.leftAnchor constraintGreaterThanOrEqualToAnchor:view.leftAnchor constant:0].active = YES;
                        [lbl.rightAnchor constraintLessThanOrEqualToAnchor:view.rightAnchor constant:0].active = YES;
                    }
                }
                currLabel = lbl;
            }
        }

        // set entry
        NSString *propname = @(_settingsData.props[i].key);
        NSString *def = @(_settingsData.props[i].def);
        NSString *value = [self.dataSource propertySheet:self valueForKey:propname def:def item:self.item];

        NSControl *currentField;

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
                                       @"default":@(_settingsData.props[i].def)
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
                checkbox.title = @(_settingsData.props[i].title);
                checkbox.state = value.intValue ? NSControlStateValueOn : NSControlStateValueOff;
                checkbox.font = self.fontContent;

                [view addSubview:checkbox];
                currentField = checkbox;

                [_bindings addObject:@{@"sender":checkbox,
                                       @"propname":propname,
                                       @"default":@(_settingsData.props[i].def)
                }];

                checkbox.target = self;
                checkbox.action = @selector(valueChanged:);

                [currentField.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20].active=YES;
                break;
            }
        case PROP_SLIDER:
            {
                NSSlider *slider = [NSSlider new];
                if (isVert) {
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
                    slider.intValue = value.intValue;
                }
                else {
                    slider.floatValue = value.floatValue;
                }

                NSTextField *valueedit = [NSTextField new];
                valueedit.font = self.fontContent;
                if (!isVert) {
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
                                       @"default":@(_settingsData.props[i].def)
                }];

                [_bindings addObject:@{@"sender":valueedit,
                                       @"propname":propname,
                                       @"valueview":slider,
                                       @"isInteger":[NSNumber numberWithBool:step == 1 ? YES:NO],
                                       @"isFloat":[NSNumber numberWithBool:step != 1 ? YES:NO],
                                       @"default":@(_settingsData.props[i].def)
                }];

                slider.target = self;
                slider.action = @selector(valueChanged:);
                currentField = slider;

                valueedit.delegate = (id<NSTextFieldDelegate>)self;

                [view addSubview:slider];

                valueedit.translatesAutoresizingMaskIntoConstraints = NO;
                [view addSubview:valueedit];

                if (!isVert) {
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
                    if (box.itemWidth >= 0) {
                        [slider.widthAnchor constraintEqualToConstant:box.itemWidth].active = YES;
                    }
                    [valueedit.topAnchor constraintEqualToAnchor:slider.bottomAnchor constant:0].active = YES;
                    if (box.noclip) {
                        [valueedit.centerXAnchor constraintEqualToAnchor:view.centerXAnchor constant:0].active = YES;
                    }
                    else {
                        [valueedit.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:0].active = YES;
                        [valueedit.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:0].active = YES;
                    }
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

                int selectedIdx = value.intValue;

                while ((script = gettoken (script, token)) && strcmp (token, ";")) {
                    [popUpButton addItemWithTitle:@(token)];
                    if (selectedIdx == popUpButton.numberOfItems-1) {
                        [popUpButton selectItemAtIndex:popUpButton.numberOfItems-1];
                    }
                }

                [_bindings addObject:@{@"sender":popUpButton,
                                       @"propname":propname,
                                       @"default":@(_settingsData.props[i].def)
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

                [self.dataSource propertySheet:self enumerateItemsOfType:@(_settingsData.props[i].itemlist_type) block:^BOOL(id item) {
                    NSString *name = [self.dataSource propertySheet:self valueForKey:@"name" def:@"Undefined" item:item];
                    [popUpButton addItemWithTitle:name];
                    if ([value isEqualToString:name]) {
                        [popUpButton selectItemAtIndex:popUpButton.numberOfItems-1];
                    }
                    return NO;
                }];

                [_bindings addObject:@{@"sender":popUpButton,
                                       @"propname":propname,
                                       @"default":@(_settingsData.props[i].def)
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

        if (currentField && !isVert) {
            [self constrainCurrentField:currentField isVert:isVert boxHandler:box currLabel:currLabel unalignedFields:unalignedFields view:view];
            contentHeight += unit_h;
        }

        if (isReadonly) {
            if ([currentField isKindOfClass:NSTextField.class]) {
                NSTextField *tf = (NSTextField *)currentField;
                tf.enabled = YES;
                tf.editable = NO;
                tf.selectable = YES;
            }
            else {
                currentField.enabled = NO;
            }
        }

        i++;
    }

    if (box.isStackView && box.noclip) {
        NSStackView *stackview = (NSStackView *)box.view;
        NSView *spacingView = [NSView new];
        [stackview addArrangedSubview:spacingView];
    }

    if (!box.isStackView) {
        for (NSView *field in unalignedFields) {
            if ([field isKindOfClass:NSStackView.class]) {
                [field.leadingAnchor constraintEqualToAnchor:field.superview.leadingAnchor constant:8].active=YES;
            }
            else if (box.firstLabel) {
                if ([field isKindOfClass:NSButton.class]) {
                    [field.leadingAnchor constraintLessThanOrEqualToAnchor:box.firstLabel.trailingAnchor constant:8].active=YES;
                }
                else {
                    [field.leadingAnchor constraintEqualToAnchor:box.firstLabel.trailingAnchor constant:8].active=YES;
                }
            }
            else {
                CGFloat leadingOffs = 8;
                if ([field isKindOfClass:NSStackView.class]) {
                    leadingOffs = 0;
                }
                [field.leadingAnchor constraintEqualToAnchor:field.superview.leadingAnchor constant:leadingOffs].active=YES;
            }
        }
    }

    // constrain to bottom
    if (box.previousField) {
        [box.previousField.bottomAnchor constraintEqualToAnchor:view.bottomAnchor constant:-8].active = YES;
    }

    self.contentSize = NSMakeSize(contentWidth, contentHeight);

    return i;
}

- (void)setDataSource:(NSObject<PropertySheetDataSource> *)dataSource {
    _dataSource = dataSource;

    self.contentView.translatesAutoresizingMaskIntoConstraints = NO;

    NSView *view;
    _bindings = [NSMutableArray new];

    settings_data_free (&_settingsData);

    BOOL have_settings = YES;
    NSString *config = [dataSource propertySheet:self configForItem:self.item];
    BOOL isReadonly = NO;
    if ([dataSource respondsToSelector:@selector(propertySheet:itemIsReadonly:)]) {
        isReadonly = [dataSource propertySheet:self itemIsReadonly:self.item];
    }
    if (!config || settings_data_init(&_settingsData, config.UTF8String) < 0) {
        have_settings = NO;
    }

    self.contentSize = NSMakeSize(NSWidth(self.view.frame)-2, 0);

    view = self.contentView;

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
        self.contentView.contentSize = NSMakeSize(NSWidth(self.view.frame)-6, NSHeight(self.view.frame)-6);
        return;
    }

    self.contentSize = NSMakeSize(NSWidth(self.contentView.frame), 0);

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

    [self addFieldsToBox:box fromIndex:0 count:self.settingsData.nprops isReadonly:isReadonly];

//    self.contentView.frame = NSMakeRect(0, 0, self.contentSize.width, self.contentSize.height);
    self.contentView.contentSize = self.contentSize;
}

- (void)save {
    [self.dataSource propertySheetBeginChanges];
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] && binding[@"propname"]) {
            id sender = binding[@"sender"];
            NSString *value;
            if (binding[@"isInteger"] && [binding[@"isInteger"] boolValue]) {
                value = (@([sender integerValue])).stringValue;
            }
            else if (binding[@"isFloat"] && [binding[@"isFloat"] boolValue]) {
                value = (@([sender floatValue])).stringValue;
            }
            else if ([sender isKindOfClass:[NSPopUpButton class]]) {
                value = (@([sender indexOfSelectedItem])).stringValue;
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
                value = (@([sender integerValue])).stringValue;
            }
            else if (binding[@"isFloat"] && [binding[@"isFloat"] boolValue]) {
                value = (@([sender floatValue])).stringValue;
            }
            else if ([sender isKindOfClass:[NSPopUpButton class]]) {
                NSUInteger idx = [sender indexOfSelectedItem];
                value = @(idx).stringValue;
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
