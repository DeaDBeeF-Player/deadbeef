//
//  QuickLookViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 10/26/26.
//  Copyright © 2026 Oleksiy Yakovenko. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "CoverManager.h"
#import "QuickLookViewController.h"

extern DB_functions_t *deadbeef;

static const CGFloat kArtworkPadding = 40;
static const CGFloat kTextMargin = 16;
static const CGFloat kZoomStartScale = 0.02;
static const CGFloat kCornerRadius = 16;
static const NSSize kPanelSize = {700, 350};
static const CGFloat kLineSpacing = 16;
static const NSTimeInterval kZoomDuration = 0.18;

@interface QuickLookViewController () {
    DB_playItem_t *_track;
}

@property (nonatomic) NSPanel *panel;
@property (nonatomic) NSVisualEffectView *backgroundView;
@property (nonatomic) NSImageView *artworkView;
@property (nonatomic) NSStackView *infoStack;
@property (nonatomic) NSArray<NSTextField *> *infoLabels; // title, artist, album, time
@property (nonatomic) BOOL animating;
@property (nonatomic) BOOL suspended; // hidden because the app became inactive, to be shown again on activation
@property (nonatomic) NSPoint zoomOrigin; // screen coordinates

@end

@implementation QuickLookViewController

+ (QuickLookViewController *)shared {
    static QuickLookViewController *instance;
    static dispatch_once_t onceToken;
    dispatch_once (&onceToken, ^{
        instance = [QuickLookViewController new];
    });
    return instance;
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [self setTrack:NULL];
}

- (BOOL)isVisible {
    return _panel.visible && !self.animating;
}

#pragma mark - UI

- (void)loadView {
    NSView *content = [[NSView alloc] initWithFrame:NSMakeRect (0, 0, 640, 320)];

    // Translucent blurred background, behind everything else
    self.backgroundView = [self newBackgroundViewWithFrame:content.bounds];
    [content addSubview:self.backgroundView];

    self.artworkView = [NSImageView new];
    self.artworkView.imageScaling = NSImageScaleProportionallyUpOrDown;
    self.artworkView.imageAlignment = NSImageAlignCenter;
    self.artworkView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.artworkView setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
                                                forOrientation:NSLayoutConstraintOrientationHorizontal];
    [self.artworkView setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
                                                forOrientation:NSLayoutConstraintOrientationVertical];
    [content addSubview:self.artworkView];

    // One wrapping label per row, so that long values wrap instead of widening the window
    NSMutableArray<NSTextField *> *labels = [NSMutableArray new];
    for (int i = 0; i < 4; i++) {
        NSTextField *label = [NSTextField wrappingLabelWithString:@""];
        label.alignment = NSTextAlignmentLeft;
        label.lineBreakMode = NSLineBreakByWordWrapping;
        label.maximumNumberOfLines = 0;
        label.translatesAutoresizingMaskIntoConstraints = NO;
        [label setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
                                        forOrientation:NSLayoutConstraintOrientationHorizontal];
        [labels addObject:label];
    }
    self.infoLabels = labels;

    self.infoStack = [NSStackView stackViewWithViews:labels];
    self.infoStack.orientation = NSUserInterfaceLayoutOrientationVertical;
    self.infoStack.alignment = NSLayoutAttributeLeading;
    self.infoStack.spacing = kLineSpacing;
    self.infoStack.translatesAutoresizingMaskIntoConstraints = NO;
    [content addSubview:self.infoStack];
    for (NSTextField *label in labels) {
        [label.widthAnchor constraintEqualToAnchor:self.infoStack.widthAnchor].active = YES;
    }

    [NSLayoutConstraint activateConstraints:@[
        [self.artworkView.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:kArtworkPadding],
        [self.artworkView.topAnchor constraintEqualToAnchor:content.topAnchor constant:kArtworkPadding],
        [self.artworkView.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-kArtworkPadding],
        [self.artworkView.widthAnchor constraintEqualToAnchor:content.widthAnchor multiplier:0.45],

        [self.infoStack.leadingAnchor constraintEqualToAnchor:self.artworkView.trailingAnchor constant:kArtworkPadding],
        [self.infoStack.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-kTextMargin],
        [self.infoStack.centerYAnchor constraintEqualToAnchor:content.centerYAnchor],
    ]];

    self.view = content;
}

- (NSVisualEffectView *)newBackgroundViewWithFrame:(NSRect)frame {
    NSVisualEffectView *view = [[NSVisualEffectView alloc] initWithFrame:frame];
    view.material = NSVisualEffectMaterialHUDWindow;
    view.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    view.state = NSVisualEffectStateActive;
    view.wantsLayer = YES;
    view.layer.cornerRadius = kCornerRadius;
    view.layer.masksToBounds = YES;
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    return view;
}

- (NSPanel *)panel {
    if (_panel == nil) {
        NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
            | NSWindowStyleMaskUtilityWindow | NSWindowStyleMaskNonactivatingPanel | NSWindowStyleMaskFullSizeContentView;
        NSPanel *panel = [[NSPanel alloc] initWithContentRect:self.view.frame
                                                    styleMask:style
                                                      backing:NSBackingStoreBuffered
                                                        defer:YES];
        panel.floatingPanel = YES;
        panel.becomesKeyOnlyIfNeeded = YES;
        panel.hidesOnDeactivate = NO;
        panel.releasedWhenClosed = NO;
        panel.minSize = NSMakeSize (400, 240);
        panel.titlebarAppearsTransparent = YES;
        panel.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
        panel.opaque = NO;
        panel.backgroundColor = NSColor.clearColor;
        panel.contentViewController = self;
        // Route the close button through the animated hide
        NSButton *closeButton = [panel standardWindowButton:NSWindowCloseButton];
        closeButton.target = self;
        closeButton.action = @selector (closeButtonClicked:);
        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector (appDidResignActive:)
                                                   name:NSApplicationDidResignActiveNotification
                                                 object:nil];
        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector (appDidBecomeActive:)
                                                   name:NSApplicationDidBecomeActiveNotification
                                                 object:nil];
        _panel = panel;
    }
    return _panel;
}

- (void)closeButtonClicked:(id)sender {
    [self hidePanelReleasingTrack:YES];
}

- (void)appDidResignActive:(NSNotification *)notification {
    if (!self.isVisible) {
        return;
    }
    self.suspended = YES;
    [self hidePanelReleasingTrack:NO];
}

- (void)appDidBecomeActive:(NSNotification *)notification {
    [self resumeIfSuspended];
}

- (void)resumeIfSuspended {
    if (!self.suspended) {
        return;
    }
    if (self.animating) {
        // still zooming out, try again when it's done
        dispatch_after (dispatch_time (DISPATCH_TIME_NOW, (int64_t)((kZoomDuration + 0.05) * NSEC_PER_SEC)), dispatch_get_main_queue (), ^{
            if (NSApp.active) {
                [self resumeIfSuspended];
            }
        });
        return;
    }
    self.suspended = NO;
    [self showPanel];
}

/// Tiny frame centered at the zoom origin, used as the start/end state of the zoom animation
- (NSRect)zoomedOutFrameForFrame:(NSRect)frame {
    CGFloat w = MAX (1, frame.size.width * kZoomStartScale);
    CGFloat h = MAX (1, frame.size.height * kZoomStartScale);
    return NSMakeRect (self.zoomOrigin.x - w / 2, self.zoomOrigin.y - h / 2, w, h);
}

/// Borderless window showing a snapshot of the panel. Resizing it makes the whole panel look like it's zooming,
/// without relayouting the real panel's contents.
- (NSWindow *)zoomWindowWithSnapshotOfPanel:(NSPanel *)panel frame:(NSRect)frame {
    NSView *frameView = panel.contentView.superview;
    [frameView layoutSubtreeIfNeeded];
    // The blur can't be captured, so snapshot only the contents, and let the zoom window do its own blur
    self.backgroundView.hidden = YES;
    NSBitmapImageRep *rep = [frameView bitmapImageRepForCachingDisplayInRect:frameView.bounds];
    [frameView cacheDisplayInRect:frameView.bounds toBitmapImageRep:rep];
    self.backgroundView.hidden = NO;
    NSImage *snapshot = [[NSImage alloc] initWithSize:frameView.bounds.size];
    [snapshot addRepresentation:rep];

    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:NSWindowStyleMaskBorderless
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.releasedWhenClosed = NO;
    window.opaque = NO;
    window.backgroundColor = NSColor.clearColor;
    window.hasShadow = NO;
    window.ignoresMouseEvents = YES;
    window.level = panel.level;

    NSView *windowContent = [self newBackgroundViewWithFrame:((NSView *)window.contentView).bounds];
    window.contentView = windowContent;

    NSImageView *imageView = [[NSImageView alloc] initWithFrame:windowContent.bounds];
    imageView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    imageView.imageScaling = NSImageScaleAxesIndependently;
    imageView.image = snapshot;
    [windowContent addSubview:imageView];
    return window;
}

- (void)showPanel {
    NSPanel *panel = self.panel;
    [panel setFrame:NSMakeRect (0, 0, kPanelSize.width, kPanelSize.height) display:NO];
    [panel center];
    NSRect fullFrame = panel.frame;

    // Render the panel while it's invisible, then zoom its snapshot out of the origin point
    panel.alphaValue = 0;
    [panel orderFrontRegardless];
    NSWindow *zoomWindow = [self zoomWindowWithSnapshotOfPanel:panel frame:[self zoomedOutFrameForFrame:fullFrame]];
    [zoomWindow orderFrontRegardless];

    self.animating = YES;
    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
        context.duration = kZoomDuration;
        context.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
        [zoomWindow.animator setFrame:fullFrame display:YES];
    } completionHandler:^{
        panel.alphaValue = 1;
        [zoomWindow orderOut:nil];
        self.animating = NO;
    }];
}

- (void)hidePanelReleasingTrack:(BOOL)releaseTrack {
    if (!self.isVisible) {
        return;
    }
    self.animating = YES;
    if (releaseTrack) {
        [self setTrack:NULL];
    }

    NSPanel *panel = self.panel;
    NSRect fullFrame = panel.frame;
    NSWindow *zoomWindow = [self zoomWindowWithSnapshotOfPanel:panel frame:fullFrame];
    [zoomWindow orderFrontRegardless];
    [panel orderOut:nil];

    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
        context.duration = kZoomDuration;
        context.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
        [zoomWindow.animator setFrame:[self zoomedOutFrameForFrame:fullFrame] display:YES];
    } completionHandler:^{
        [zoomWindow orderOut:nil];
        self.animating = NO;
    }];
}

#pragma mark - Track handling

- (void)setTrack:(nullable DB_playItem_t *)track {
    if (track == _track) {
        return;
    }
    if (track != NULL) {
        deadbeef->pl_item_ref (track);
    }
    if (_track != NULL) {
        deadbeef->pl_item_unref (_track);
    }
    _track = track;
}

- (void)toggleForTrack:(nullable DB_playItem_t *)track zoomOrigin:(NSPoint)origin {
    self.zoomOrigin = origin;
    if (self.animating) {
        return;
    }
    if (self.isVisible) {
        [self hidePanelReleasingTrack:YES];
        return;
    }
    if (track == NULL) {
        return;
    }
    [self setTrack:track];
    [self refresh];
    [self showPanel];
}

- (void)updateWithTrack:(nullable DB_playItem_t *)track zoomOrigin:(NSPoint)origin {
    self.zoomOrigin = origin;
    if (!self.isVisible || track == NULL || track == _track) {
        return;
    }
    [self setTrack:track];
    [self refresh];
}

static NSString *
string_or_empty (const char *s) {
    NSString *str = s ? [NSString stringWithUTF8String:s] : nil;
    return str ?: @"";
}

static NSString *
format_duration (float duration) {
    int total = duration > 0 ? (int)duration : 0;
    int h = total / 3600;
    int m = (total / 60) % 60;
    int s = total % 60;
    if (h > 0) {
        return [NSString stringWithFormat:@"%d:%02d:%02d", h, m, s];
    }
    return [NSString stringWithFormat:@"%d:%02d", m, s];
}

- (void)refresh {
    [self view]; // make sure the UI is loaded
    DB_playItem_t *track = _track;
    if (track == NULL) {
        return;
    }

    NSString *filename;
    NSString *title;
    NSString *artist;
    NSString *album;
    NSString *duration;

    deadbeef->pl_lock ();
    filename = string_or_empty (deadbeef->pl_find_meta (track, ":URI")).lastPathComponent;
    title = string_or_empty (deadbeef->pl_find_meta (track, "title"));
    artist = string_or_empty (deadbeef->pl_find_meta (track, "artist"));
    album = string_or_empty (deadbeef->pl_find_meta (track, "album"));
    deadbeef->pl_unlock ();
    duration = format_duration (deadbeef->pl_get_item_duration (track));

    if (title.length == 0) {
        title = filename;
    }

    self.panel.title = filename;

    // System typography: title style for the heading, body style for the rest
    NSFont *headingFont = [NSFont boldSystemFontOfSize:[NSFont preferredFontForTextStyle:NSFontTextStyleTitle1 options:@{}].pointSize];
    NSFont *regularFont = [NSFont preferredFontForTextStyle:NSFontTextStyleBody options:@{}];
    NSFont *boldFont = [NSFont boldSystemFontOfSize:regularFont.pointSize];
    NSDictionary *titleAttrs = @{NSFontAttributeName: headingFont, NSForegroundColorAttributeName: NSColor.labelColor};
    NSDictionary *regularAttrs = @{NSFontAttributeName: regularFont, NSForegroundColorAttributeName: NSColor.secondaryLabelColor};
    NSDictionary *boldAttrs = @{NSFontAttributeName: boldFont, NSForegroundColorAttributeName: NSColor.secondaryLabelColor};
    NSMutableParagraphStyle *para = [NSMutableParagraphStyle new];
    para.alignment = NSTextAlignmentLeft;
    para.lineBreakMode = NSLineBreakByWordWrapping;

    NSAttributedString * (^line) (NSString *, NSString *) = ^(NSString *label, NSString *value) {
        NSMutableAttributedString *text = [NSMutableAttributedString new];
        if (label != nil) {
            [text appendAttributedString:[[NSAttributedString alloc] initWithString:label attributes:regularAttrs]];
        }
        [text appendAttributedString:[[NSAttributedString alloc] initWithString:value attributes:label ? boldAttrs : titleAttrs]];
        [text addAttribute:NSParagraphStyleAttributeName value:para range:NSMakeRange (0, text.length)];
        return text;
    };
    self.infoLabels[0].attributedStringValue = line (nil, title);
    self.infoLabels[1].attributedStringValue = line (@"Artist: ", artist);
    self.infoLabels[2].attributedStringValue = line (@"Album: ", album);
    self.infoLabels[3].attributedStringValue = line (@"Time: ", duration);

    // The cover manager returns the "no album art" image the same way the playlist view gets it
    self.artworkView.image = nil;
    deadbeef->pl_item_ref (track);
    NSImage *image = [CoverManager.shared coverForTrack:track completionBlock:^(NSImage *img) {
        if (track == self->_track) {
            self.artworkView.image = img;
        }
        deadbeef->pl_item_unref (track);
    }];
    if (image != nil) {
        self.artworkView.image = image;
        deadbeef->pl_item_unref (track);
    }
}

@end
