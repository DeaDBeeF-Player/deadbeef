#import "HelpWindowController.h"
#include <stdio.h>

@interface HelpWindowController ()

@property (unsafe_unretained) IBOutlet NSTextView *textView;

@end

@implementation HelpWindowController

- (NSString *)loadFromFile:(NSString *)fileName {
    FILE *fp = fopen ([fileName UTF8String], "rb");
    if (!fp)
        return @"";

    fseek (fp, 0, SEEK_END);
    size_t size = ftell (fp);
    rewind (fp);

    char *buf = malloc (size+1);
    ssize_t rb = fread (buf, 1, size, fp);
    fclose (fp);

    if (rb != size) {
        free (buf);
        return @"";
    }

    buf[size] = 0;

    NSString *content = [NSString stringWithUTF8String:buf];
    free (buf);
    assert (content);
    return content;
}

- (void)windowDidLoad {
    [super windowDidLoad];
    [self window].title = @"Help";
    NSString *content = [self loadFromFile:[[NSBundle mainBundle] pathForResource:@"help-cocoa" ofType:@"txt"]];

    [[_textView textStorage] setAttributedString:[[NSAttributedString alloc] initWithString:content]];

    [_textView setSelectedRange:NSMakeRange(0, 0)];
}

@end
