#include <assert.h>
#import "AAPLNSView.h"
#import "AAPLView.h"
#import "ShaderRenderer.h"
#import "SpectrumAnalyzerPreferencesWindowController.h"
#import "SpectrumAnalyzerPreferencesViewController.h"
#import "SpectrumAnalyzerVisualizationViewController.h"
#import "SpectrumAnalyzerLabelsView.h"
#import "VisualizationSettingsUtil.h"
#import "SpectrumShaderTypes.h"
#include "analyzer.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

#define VisXOffset 40
#define VisYOffset 12

#define LOWER_BOUND -80
static NSString * const kWindowIsVisibleKey = @"view.window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

@interface SpectrumAnalyzerVisualizationViewController() <AAPLViewDelegate, ShaderRendererDelegate> {
    ShaderRenderer *_renderer;
    ddb_analyzer_t _analyzer;
    ddb_analyzer_draw_data_t _draw_data;
    ddb_waveformat_t _fmt;
    ddb_audio_data_t _input_data;
}

@property (nonatomic) SpectrumAnalyzerPreferencesWindowController *preferencesWindowController;
@property (nonatomic) NSPopover *preferencesPopover;
@property (nonatomic) SpectrumAnalyzerLabelsView *labelsView;
@property (nonatomic) NSView *visualizationView;

@property (nonatomic) BOOL isListening;


@end

@implementation SpectrumAnalyzerVisualizationViewController

static void vis_callback (void *ctx, const ddb_audio_data_t *data) {
    SpectrumAnalyzerVisualizationViewController *view = (__bridge SpectrumAnalyzerVisualizationViewController *)(ctx);
    [view updateFFTData:data];
}

- (void)updateFFTData:(const ddb_audio_data_t *)data {
    @synchronized (self) {
        // copy the input data for later consumption
        if (_input_data.nframes != data->nframes || _input_data.fmt->channels != data->fmt->channels) {
            free (_input_data.data);
            _input_data.data = malloc (data->nframes * data->fmt->channels * sizeof (float));
            _input_data.nframes = data->nframes;
        }
        memcpy (_input_data.fmt, data->fmt, sizeof (ddb_waveformat_t));
        memcpy (_input_data.data, data->data, data->nframes * data->fmt->channels * sizeof (float));
    }
}

- (void)dealloc {
    if (self.isListening) {
        deadbeef->vis_spectrum_unlisten ((__bridge void *)(self));
        self.isListening = NO;
    }
    ddb_analyzer_dealloc(&_analyzer);
    ddb_analyzer_draw_data_dealloc(&_draw_data);
    free (_input_data.data);
    _input_data.data = NULL;
    [self removeObserver:self forKeyPath:kWindowIsVisibleKey];
}


- (void)updateAnalyzerSettings:(SpectrumAnalyzerSettings * _Nonnull)settings {
    [self.labelsView updateSettings:settings];
    if (_analyzer.mode != settings.mode || _analyzer.octave_bars_step != settings.barGranularity) {
        _analyzer.mode_did_change = 1;
    }
    _analyzer.mode = settings.mode;
    _analyzer.bar_gap_denominator = settings.distanceBetweenBars;
    _analyzer.octave_bars_step = settings.barGranularity;
    _analyzer.enable_bar_index_lookup_table = 1;
}

- (void)loadView {
    self.labelsView = [SpectrumAnalyzerLabelsView new];
    self.visualizationView = [[AAPLNSView alloc] initWithFrame:NSZeroRect];

    self.labelsView.translatesAutoresizingMaskIntoConstraints = NO;
    self.visualizationView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.labelsView addSubview:self.visualizationView];

    [NSLayoutConstraint activateConstraints:@[
        [self.visualizationView.leadingAnchor constraintEqualToAnchor:self.labelsView.leadingAnchor constant:VisXOffset],
        [self.visualizationView.trailingAnchor constraintEqualToAnchor:self.labelsView.trailingAnchor],
        [self.visualizationView.topAnchor constraintEqualToAnchor:self.labelsView.topAnchor constant:VisYOffset],
        [self.visualizationView.bottomAnchor constraintEqualToAnchor:self.labelsView.bottomAnchor],
    ]];

    self.view = self.labelsView;
}

- (void)updateVisListening {
    if (self.isListening && !self.view.window.isVisible) {
        deadbeef->vis_spectrum_unlisten ((__bridge void *)(self));
        self.isListening = NO;
    }
    else if (!self.isListening && self.view.window.isVisible) {
        deadbeef->vis_spectrum_listen2((__bridge void *)(self), vis_callback);
        self.isListening = YES;
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if (context == kIsVisibleContext) {
        [self updateVisListening];
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)setupMetalRenderer {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();

    AAPLView *view = (AAPLView *)self.visualizationView;

    // Set the device for the layer so the layer can create drawable textures that can be rendered to
    // on this device.
    view.metalLayer.device = device;

    // Set this class as the delegate to receive resize and render callbacks.
    view.delegate = self;

    view.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

    _renderer = [[ShaderRenderer alloc] initWithMetalDevice:device
                                        drawablePixelFormat:view.metalLayer.pixelFormat
                                         fragmentShaderName:@"spectrumFragmentShader"
    ];
    _renderer.delegate = self;
}

- (void)viewDidLoad {
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

    // Setup analyzer
    _input_data.fmt = &_fmt;

    ddb_analyzer_init(&_analyzer);
    _analyzer.db_lower_bound = LOWER_BOUND;
    _analyzer.peak_hold = 10;
    _analyzer.view_width = 1000;
    _analyzer.fractional_bars = 1;
    _analyzer.octave_bars_step = 2;
    _analyzer.max_of_stereo_data = 1;
    _analyzer.mode = DDB_ANALYZER_MODE_FREQUENCIES;
    _analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
    _isListening = NO;

    [self addObserver:self forKeyPath:kWindowIsVisibleKey options:NSKeyValueObservingOptionInitial context:kIsVisibleContext];

    [self setupMetalRenderer];
}

- (BOOL)updateDrawData {
    // for some reason KVO is not triggered when the window becomes hidden
    [self updateVisListening];

    if (!self.isListening || _input_data.nframes == 0) {
        return NO;
    }
    CGFloat scale = self.view.window.backingScaleFactor;

    @synchronized (self) {
        ddb_analyzer_process(&_analyzer, _input_data.fmt->samplerate, _input_data.fmt->channels, _input_data.data, _input_data.nframes);
        ddb_analyzer_tick(&_analyzer);
        ddb_analyzer_get_draw_data(&_analyzer, self.visualizationView.bounds.size.width * scale, self.visualizationView.bounds.size.height * scale, &_draw_data);
    }

    return YES;
}

// Called by the timer in superclass
- (void)draw {
    if ([self updateDrawData]) {
        [self.labelsView updateDrawData:&_draw_data];
    }
    
    self.visualizationView.needsDisplay = YES;
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
        dispatch_async(dispatch_get_main_queue(), ^{
            self.labelsView.needsDisplay = YES;
        });
    }
}

- (NSColor *)backgroundColor {
    if (self.settings.useCustomBackgroundColor) {
        NSColor *color = self.settings.customBackgroundColor;
        if (color != nil) {
            return color;
        }
    }

    return VisualizationSettingsUtil.shared.backgroundColor;
}

- (NSColor *)barColor {
    if (self.settings.useCustomBarColor) {
        NSColor *color = self.settings.customBarColor;
        if (color != nil) {
            return color;
        }
    }
    NSColor *tempColor = [VisualizationSettingsUtil.shared.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat h, s, b, a;
    [tempColor getHue:&h saturation:&s brightness:&b alpha:&a];
    return [NSColor colorWithHue:h saturation:s brightness:b*0.7 alpha:1];
}

- (NSColor *)peakColor {
    if (self.settings.useCustomPeakColor) {
        NSColor *color = self.settings.customPeakColor;
        if (color != nil) {
            return color;
        }
    }
    NSColor *tempColor = [VisualizationSettingsUtil.shared.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat h, s, b, a;
    [tempColor getHue:&h saturation:&s brightness:&b alpha:&a];

    return [NSColor colorWithHue:h saturation:s*0.7 brightness:b*1.1 alpha:1];
}


- (NSColor *)lineColor {
    return [NSColor.whiteColor colorWithAlphaComponent:0.4];
}

static inline vector_float4 vec4color (NSColor *color) {
    CGFloat components[4];
    [[color colorUsingColorSpace:NSColorSpace.sRGBColorSpace] getComponents:components];
    return (vector_float4){ (float)components[0], (float)components[1], (float)components[2], (float)components[3] };
}

#pragma mark - AAPLViewDelegate

- (void)drawableResize:(CGSize)size {
    [_renderer drawableResize:size];
}

- (void)renderToMetalLayer:(nonnull CAMetalLayer *)layer viewParams:(AAPLViewParams)params {
    [_renderer renderToMetalLayer:layer viewParams:params];
}

#pragma mark - ShaderRendererDelegate

- (void)applyFragParamsWithViewport:(vector_uint2)viewport device:(id<MTLDevice>)device encoder:(id<MTLRenderCommandEncoder>)encoder viewParams:(AAPLViewParams)viewParams {

    struct SpectrumFragParams params;

    params.backgroundColor = vec4color([self.backgroundColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace]);

    params.barColor = vec4color(self.barColor);
    params.peakColor = vec4color(self.peakColor);
    params.lineColor = vec4color(self.lineColor);
    params.size.x = viewport.x;
    params.size.y = viewport.y;
    params.barCount = _draw_data.bar_count;
    params.gridLineCount = -LOWER_BOUND / 10;
    params.backingScaleFactor = viewParams.backingScaleFactor;
    params.barWidth = _draw_data.bar_width;
    params.discreteFrequencies = _draw_data.mode == DDB_ANALYZER_MODE_FREQUENCIES;
    [encoder setFragmentBytes:&params length:sizeof (params) atIndex:0];

    assert(sizeof(struct SpectrumFragBar) == sizeof (ddb_analyzer_draw_bar_t));

    // bar data

    if (_draw_data.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
        // In this scenario, the buffer is too large, need to use MTLBuffer.
        id<MTLBuffer> buffer = [device newBufferWithBytes:_draw_data.bars length:_draw_data.bar_count * sizeof (struct SpectrumFragBar) options:0];

        [encoder setFragmentBuffer:buffer offset:0 atIndex:1];

        if (_draw_data.bar_index_for_x_coordinate_table != NULL) {
            id<MTLBuffer> lookupBuffer = [device newBufferWithBytes:_draw_data.bar_index_for_x_coordinate_table length:_draw_data.bar_index_for_x_coordinate_table_size * sizeof (int) options:0];
            [encoder setFragmentBuffer:lookupBuffer offset:0 atIndex:2];
        }
    }
    else {
        // The buffer is not bigger than ~2.5KB (211 bars * 12 bytes),
        // therefore it should be safe to use setFragmentBytes.
        [encoder setFragmentBytes:_draw_data.bars length:_draw_data.bar_count * sizeof (struct SpectrumFragBar) atIndex:1];
    }

}
@end
