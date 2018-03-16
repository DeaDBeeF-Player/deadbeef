#import "TextViewerWindowController.h"
#include <stdio.h>

@interface TextViewerWindowController () {
    NSString *_content;
}
@property (unsafe_unretained) IBOutlet NSTextView *textView;



@end

@implementation TextViewerWindowController

- (void)loadFromFile:(NSString *)fileName {
    FILE *fp = fopen ([fileName UTF8String], "rb");
    if (!fp)
        return;

    fseek (fp, 0, SEEK_END);
    size_t size = ftell (fp);
    rewind (fp);

    char *buf = malloc (size+1);
    ssize_t rb = fread (buf, 1, size, fp);
    fclose (fp);

    if (rb != size) {
        free (buf);
        return;
    }

    _content = [NSString stringWithUTF8String:buf];
    free (buf);
}

- (void)windowDidLoad {
    [super windowDidLoad];
     
    [[_textView textStorage] setAttributedString:[[NSAttributedString alloc] initWithString:_content]];
}

@end
