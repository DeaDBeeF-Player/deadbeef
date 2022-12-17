PLUG(stdio)
#ifdef HAVE_COCOAUI
PLUG(cocoaui)
#endif
#ifdef HAVE_COREAUDIO
PLUG(coreaudio)
PLUG(nullout)
#endif
#ifdef HAVE_XGUI
PLUG(xgui)
#endif
#ifdef GOOGLETEST_STATIC
PLUG(mp3)
#endif
