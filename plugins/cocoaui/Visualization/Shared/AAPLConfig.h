/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Header defining preprocessor conditional values that control the configuration of the app
*/

// When enabled, rendering occurs on the main application thread.
// This can make responding to UI events during redraw simpler
// to manage because UI calls usually must occur on the main thread.
// When disabled, rendering occurs on a background thread, allowing
// the UI to respond more quickly in some cases because events can be 
// processed asynchronously from potentially CPU-intensive rendering code.
#define RENDER_ON_MAIN_THREAD 0

// When enabled, the view continually animates and renders
// frames 60 times a second.  When disabled, rendering is event
// based, occurring when a UI event requests a redraw.
#define ANIMATION_RENDERING   0

// When enabled, the drawable's size is updated automatically whenever
// the view is resized. When disabled, you can update the drawable's
// size explicitly outside the view class.
#define AUTOMATICALLY_RESIZE  1
