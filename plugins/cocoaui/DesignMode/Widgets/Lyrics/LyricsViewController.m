//
//  LyricsViewController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "LyricsViewController.h"
#import "Weakify.h"

extern DB_functions_t *deadbeef;

static NSString * const lyricsNotAvailableString = @"Lyrics Not Available";

@interface LyricsViewController ()

@property (nonatomic) NSCache<NSString *, NSString *> *lyricsCache;

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
    self.lyricsCache = [NSCache new];
    [self update];
}

- (NSString *)parseLyricsFromData:(NSData *)data response:(NSURLResponse *)response {
    NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;

    if (data == nil || httpResponse.statusCode != 200 || ![response.MIMEType isEqualToString:@"application/json"]) {
        return nil;
    }

    id object = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![object isKindOfClass:NSDictionary.class]) {
        return nil;
    }

    NSDictionary *dict = object;
    NSString *lyrics = dict[@"lyrics"];
    if (![lyrics isKindOfClass:NSString.class]) {
        return nil;
    }

    return lyrics;
}

- (void)fetchFromLyricsOvh {
    NSString *artist;
    NSString *title;

    ddb_playItem_t *track = self.track;
    deadbeef->pl_item_ref (track);

    deadbeef->pl_lock();

    const char *str = deadbeef->pl_find_meta (self.track, "artist");
    if (str != NULL) {
        artist = @(str);
    }
    str = deadbeef->pl_find_meta (self.track, "title");
    if (str != NULL) {
        title = @(str);
    }

    deadbeef->pl_unlock();

    if (artist != nil && title != nil) {
        NSURL *url = [[[NSURL URLWithString:@"https://api.lyrics.ovh/v1"] URLByAppendingPathComponent:artist] URLByAppendingPathComponent:title];
        NSURLRequest *request = [NSURLRequest requestWithURL:url];

        weakify(self);
        NSURLSessionDataTask *task = [NSURLSession.sharedSession dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
            strongify(self);

            NSString *lyrics = [self parseLyricsFromData:data response:response];

            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateWithLyrics:lyrics track:track];
            });
        }];
        [task resume];
    }
}

- (NSString *)currentArtist {
    NSString *result;

    if (self.track == nil) {
        return nil;
    }

    deadbeef->pl_lock();

    const char *str = deadbeef->pl_find_meta (self.track, "artist");
    if (str != NULL) {
        result = @(str);
    }

    deadbeef->pl_unlock();
    return result;
}

- (NSString *)currentTitle {
    if (self.track == nil) {
        return nil;
    }

    NSString *result;
    deadbeef->pl_lock();

    const char *str = deadbeef->pl_find_meta (self.track, "title");
    if (str != NULL) {
        result = @(str);
    }

    deadbeef->pl_unlock();
    return result;
}

- (NSString *)currentKey {
    NSString *artist = [self currentArtist];
    NSString *title = [self currentTitle];

    return [NSString stringWithFormat:@"artist:%@;title:%@", artist ?: @"null", title ?: @"null"];
}

- (void)updateWithLyrics:(NSString * _Nullable)lyrics track:(ddb_playItem_t * _Nullable)track {
    if (track != self.track) {
        deadbeef->pl_item_unref(track);
        return;
    }

    if (track != NULL) {
        deadbeef->pl_item_unref(track);
        track = NULL;
    }

    if (lyrics == nil) {
        lyrics = lyricsNotAvailableString;
    }

    NSString *key = [self currentKey];
    [self.lyricsCache setObject:lyrics forKey:key];

    NSString *artist = [self currentArtist];
    NSString *title = [self currentTitle];

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

- (void)update {
    if (self.track == NULL) {
        return [self updateWithLyrics:nil track:nil];
    }

    NSString *key = [self currentKey];
    NSString *cachedLyrics = [self.lyricsCache objectForKey:key];
    if (cachedLyrics != nil) {
        if (self.track != nil) {
            deadbeef->pl_item_ref(self.track);
        }
        return [self updateWithLyrics:cachedLyrics track:self.track];
    }

    deadbeef->pl_lock();

    // A bit of a hack since deadbeef stores multiline values as 0-separated lines.
    DB_metaInfo_t *meta = deadbeef->pl_meta_for_key(self.track, "lyrics");
    if (meta == NULL) {
        deadbeef->pl_unlock();
        [self fetchFromLyricsOvh];
        return;
    }

    // get lyrics from tags
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
    NSString *lyrics = @(lyrics_buffer);
    free(lyrics_buffer);
    lyrics_buffer = NULL;

    deadbeef->pl_unlock();

    deadbeef->pl_item_ref (self.track);
    [self updateWithLyrics:lyrics track:self.track];
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
