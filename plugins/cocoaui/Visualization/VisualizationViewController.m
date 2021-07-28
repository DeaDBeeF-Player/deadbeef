#import "VisualizationViewController.h"
#include "analyzer.h"

static NSString * const kWindowIsVisibleKey = @"view.window.isVisible";
static void *kHiddenOrHasHiddenAncestorContext = &kHiddenOrHasHiddenAncestorContext;

@interface VisualizationViewController ()

@property (nonatomic) NSTimer *tickTimer;

@end

@implementation VisualizationViewController

- (void)dealloc {
    [self removeObserver:self forKeyPath:kWindowIsVisibleKey];
    [self.tickTimer invalidate];
}

- (void)awakeFromNib {
    [self addObserver:self forKeyPath:kWindowIsVisibleKey options:NSKeyValueObservingOptionInitial context:kHiddenOrHasHiddenAncestorContext];

    NSMenu *menu = [NSMenu new];
    NSMenuItem *modeMenuItem = [menu addItemWithTitle:@"Mode" action:nil keyEquivalent:@""];
    NSMenuItem *gapSizeMenuItem = [menu addItemWithTitle:@"Gap Size" action:nil keyEquivalent:@""];

    modeMenuItem.submenu = [NSMenu new];
    gapSizeMenuItem.submenu = [NSMenu new];

    [modeMenuItem.submenu addItemWithTitle:@"Discrete Frequencies" action:@selector(setDescreteFrequenciesMode:) keyEquivalent:@""];
    [modeMenuItem.submenu addItemWithTitle:@"1/12 Octave Bands" action:@selector(set12BarsPerOctaveMode:) keyEquivalent:@""];
    [modeMenuItem.submenu addItemWithTitle:@"1/24 Octave Bands" action:@selector(set24BarsPerOctaveMode:) keyEquivalent:@""];

    [gapSizeMenuItem.submenu addItemWithTitle:@"None" action:@selector(setNoGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/2 Bar" action:@selector(setHalfGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/3 Bar" action:@selector(setThirdGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/4 Bar" action:@selector(setFourthGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/5 Bar" action:@selector(setFifthGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/6 Bar" action:@selector(setSixthGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/7 Bar" action:@selector(setSeventhGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/8 Bar" action:@selector(setEighthGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/9 Bar" action:@selector(setNinthGap:) keyEquivalent:@""];
    [gapSizeMenuItem.submenu addItemWithTitle:@"1/10 Bar" action:@selector(setTenthGap:) keyEquivalent:@""];

    self.view.menu = menu;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if (context == kHiddenOrHasHiddenAncestorContext) {
        if (self.view.window.isVisible) {
            __weak VisualizationViewController *weakSelf = self;
            if (self.tickTimer == nil) {
                self.tickTimer = [NSTimer timerWithTimeInterval:1/30.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
                    VisualizationViewController *strongSelf = weakSelf;
                    if (!strongSelf.view.window.isVisible) {
                        [strongSelf.tickTimer invalidate];
                        strongSelf.tickTimer = nil;
                        return;
                    }

                    strongSelf.view.needsDisplay = YES;
                }];

                [[NSRunLoop currentRunLoop] addTimer:self.tickTimer forMode:NSRunLoopCommonModes];
            }
        }
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark - Actions

- (void)setDescreteFrequenciesMode:(NSMenuItem *)sender {
    self.settings.mode = DDB_ANALYZER_MODE_FREQUENCIES;
}

- (void)set12BarsPerOctaveMode:(NSMenuItem *)sender {
    self.settings.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
    self.settings.barGranularity = 2;
}

- (void)set24BarsPerOctaveMode:(NSMenuItem *)sender {
    self.settings.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
    self.settings.barGranularity = 1;
}

- (void)setNoGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 0;
}

- (void)setHalfGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 2;
}

- (void)setThirdGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 3;
}

- (void)setFourthGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 4;
}

- (void)setFifthGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 5;
}

- (void)setSixthGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 6;
}

- (void)setSeventhGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 7;
}

- (void)setEighthGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 8;
}

- (void)setNinthGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 9;
}

- (void)setTenthGap:(NSMenuItem *)sender {
    self.settings.distanceBetweenBars = 10;
}

@end
