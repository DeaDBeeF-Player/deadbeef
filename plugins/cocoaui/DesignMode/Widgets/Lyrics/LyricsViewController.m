//
//  LyricsViewController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "LyricsViewController.h"

extern DB_functions_t *deadbeef;

@interface LyricsViewController ()

@property (unsafe_unretained) IBOutlet NSTextView *textView;

@end

@implementation LyricsViewController

- (void)dealloc {
    if (_track != NULL) {
        deadbeef->pl_item_unref (_track);
        _track = NULL;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self update];
}

- (void)update {
    NSString *lyrics = @"Lyrics Not Available";
    NSString *artist;
    NSString *title;
    if (self.track != NULL) {
        // get lyrics from tags
        deadbeef->pl_lock();

        // A bit of a hack since deadbeef stores multiline values as 0-separated lines.
        DB_metaInfo_t *meta = deadbeef->pl_meta_for_key(self.track, "lyrics");
        if (meta != NULL) {
            size_t buffer_size = 100000;
            char *lyrics_buffer = malloc(100000);
            *lyrics_buffer = 0;

            size_t value_size = meta->valuesize;
            if (value_size > buffer_size - 1) {
                value_size = buffer_size - 1;
            }
            memcpy(lyrics_buffer, meta->value, value_size);
            char *p = lyrics_buffer;
            for (size_t i = 0; i <= value_size; i++, p++) {
                if (*p == 0) {
                    *p = '\n';
                }
            }
            lyrics_buffer[value_size] = 0;
            lyrics = @(lyrics_buffer);
            free(lyrics_buffer);
        }

        const char *str = deadbeef->pl_find_meta (self.track, "artist");
        if (str != NULL) {
            artist = @(str);
        }
        str = deadbeef->pl_find_meta (self.track, "title");
        if (str != NULL) {
            title = @(str);
        }

        deadbeef->pl_unlock();
    }

    NSMutableParagraphStyle *paragraphStyle = [NSParagraphStyle.defaultParagraphStyle mutableCopy];
    paragraphStyle.alignment = NSTextAlignmentCenter;

    NSAttributedString *headingAttributedString;
    if (artist != nil && title != nil) {
        NSString *headingString = [NSString stringWithFormat:@"%@ - %@\n\n", artist, title];
        NSFont *headingFont = [NSFont systemFontOfSize:16 weight:NSFontWeightBold];

        headingAttributedString = [[NSAttributedString alloc] initWithString:headingString attributes:@{
            NSForegroundColorAttributeName: NSColor.controlTextColor,
            NSFontAttributeName: headingFont,
            NSParagraphStyleAttributeName: paragraphStyle
        }];

    }

    NSAttributedString *contentAttributedString = [[NSAttributedString alloc] initWithString:lyrics attributes:@{
        NSForegroundColorAttributeName: NSColor.controlTextColor,
        NSFontAttributeName: [NSFont systemFontOfSize:NSFont.systemFontSize],
        NSParagraphStyleAttributeName: paragraphStyle
    }];

    NSMutableAttributedString *wholeAttributedString = [NSMutableAttributedString new];
    if (headingAttributedString != nil) {
        [wholeAttributedString appendAttributedString:headingAttributedString];
    }
    [wholeAttributedString appendAttributedString:contentAttributedString];

    [self.textView.textStorage setAttributedString:wholeAttributedString];

    self.textView.selectedRange = NSMakeRange(0, 0);
}

- (void)setTrack:(ddb_playItem_t *)track {
    if (_track != NULL) {
        deadbeef->pl_item_unref (_track);
    }

    _track = track;

    if (_track != NULL) {
        deadbeef->pl_item_ref (_track);
    }

    [self update];
}

@end
