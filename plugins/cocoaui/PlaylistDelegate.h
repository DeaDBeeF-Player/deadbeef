//
//  PlaylistDelegate.h
//  deadbeef
//
//  Created by waker on 14/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

#define PLT_MAX_COLUMNS 100

typedef struct {
    char *title;
    int _id; // predefined col type
    char *format;
    int size;
    char *bytecode;
    int bytecode_len;
} plt_col_info_t;

// implementation of DdbListviewDelegate
@interface PlaylistDelegate : NSObject {
    plt_col_info_t columns[PLT_MAX_COLUMNS];
    int ncolumns;
    NSImage *playTpl;
    NSImage *pauseTpl;
    NSImage *bufTpl;
    NSDictionary *_colTextAttrsDictionary;
    NSDictionary *_cellTextAttrsDictionary;
    NSDictionary *_cellSelectedTextAttrsDictionary;
}

- (PlaylistDelegate *)init;

@end
