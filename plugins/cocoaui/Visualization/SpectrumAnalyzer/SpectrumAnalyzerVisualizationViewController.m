#import "SpectrumAnalyzerVisualizationView.h"
#import "SpectrumAnalyzerPreferencesWindowController.h"
#import "SpectrumAnalyzerPreferencesViewController.h"
#import "SpectrumAnalyzerVisualizationViewController.h"
#import "VisualizationSettingsUtil.h"
#include "analyzer.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface SpectrumAnalyzerVisualizationViewController()

@property (nonatomic) SpectrumAnalyzerPreferencesWindowController *preferencesWindowController;
@property (nonatomic) NSPopover *preferencesPopover;

@end

@implementation SpectrumAnalyzerVisualizationViewController

- (void)updateAnalyzerSettings:(SpectrumAnalyzerSettings * _Nonnull)settings {
    SpectrumAnalyzerVisualizationView *view = (SpectrumAnalyzerVisualizationView *)self.view;
    [view updateAnalyzerSettings:settings];
}

- (void)loadView {
    SpectrumAnalyzerVisualizationView *view = [[SpectrumAnalyzerVisualizationView alloc] initWithFrame:NSZeroRect];
    self.view = view;
    self.view.translatesAutoresizingMaskIntoConstraints = NO;
}

- (void)awakeFromNib {
    [super awakeFromNib];

    NSMenu *menu = [NSMenu new];
    NSMenuItem *modeMenuItem = [menu addItemWithTitle:@"Mode" action:nil keyEquivalent:@""];
    NSMenuItem *gapSizeMenuItem = [menu addItemWithTitle:@"Gap Size" action:nil keyEquivalent:@""];

    modeMenuItem.submenu = [NSMenu new];
    gapSizeMenuItem.submenu = [NSMenu new];

    [modeMenuItem.submenu addItemWithTitle:@"Discrete Frequencies" action:@selector(setDiscreteFrequenciesMode:) keyEquivalent:@""];
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

    [menu addItem:NSMenuItem.separatorItem];
    [menu addItemWithTitle:@"Preferences" action:@selector(preferences:) keyEquivalent:@""];

    self.view.menu = menu;

    SpectrumAnalyzerVisualizationView *view = (SpectrumAnalyzerVisualizationView *)self.view;

    view.baseColor = VisualizationSettingsUtil.shared.baseColor;
    view.backgroundColor = VisualizationSettingsUtil.shared.backgroundColor;
}

#pragma mark - Actions

- (void)setDiscreteFrequenciesMode:(NSMenuItem *)sender {
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

- (void)preferences:(NSMenuItem *)sender {
    if (self.preferencesPopover != nil) {
        [self.preferencesPopover close];
        self.preferencesPopover = nil;
    }

    self.preferencesPopover = [NSPopover new];
    self.preferencesPopover.behavior = NSPopoverBehaviorTransient;

    SpectrumAnalyzerPreferencesViewController *preferencesViewController = [[SpectrumAnalyzerPreferencesViewController alloc] initWithNibName:@"SpectrumAnalyzerPreferencesViewController" bundle:nil];
    preferencesViewController.settings = self.settings;
    preferencesViewController.popover = self.preferencesPopover;

    self.preferencesPopover.contentViewController = preferencesViewController;

    [self.preferencesPopover showRelativeToRect:NSZeroRect ofView:self.view preferredEdge:NSRectEdgeMaxY];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    if (menuItem.action == @selector(setDiscreteFrequenciesMode:) && self.settings.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(set12BarsPerOctaveMode:) && self.settings.mode == DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS && self.settings.barGranularity == 2) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(set24BarsPerOctaveMode:) && self.settings.mode == DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS && self.settings.barGranularity == 1) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setNoGap:) && self.settings.distanceBetweenBars == 0) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setHalfGap:) && self.settings.distanceBetweenBars == 2) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setThirdGap:) && self.settings.distanceBetweenBars == 3) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setFourthGap:) && self.settings.distanceBetweenBars == 4) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setFifthGap:) && self.settings.distanceBetweenBars == 5) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setSixthGap:) && self.settings.distanceBetweenBars == 6) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setSeventhGap:) && self.settings.distanceBetweenBars == 7) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setEighthGap:) && self.settings.distanceBetweenBars == 8) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setNinthGap:) && self.settings.distanceBetweenBars == 9) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setTenthGap:) && self.settings.distanceBetweenBars == 10) {
        menuItem.state = NSControlStateValueOn;
    }
    else {
        menuItem.state = NSControlStateValueOff;
    }

    return YES;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_CONFIGCHANGED) {
        SpectrumAnalyzerVisualizationView *view = (SpectrumAnalyzerVisualizationView *)self.view;

        view.baseColor = VisualizationSettingsUtil.shared.baseColor;
        view.backgroundColor = VisualizationSettingsUtil.shared.backgroundColor;
    }
}

@end
