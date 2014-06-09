//
//  cocoautil.m
//  
//
//  Created by Oleksiy Yakovenko on 09/06/2014.
//
//

#import "cocoautil.h"
#import <Foundation/Foundation.h>

int
cocoautil_get_resources_path (char *s, int size) {
    NSBundle *main =[NSBundle mainBundle];
	NSURL *url = [main resourceURL];
    const char *str = [url fileSystemRepresentation];
    strcpy (s, str);
	return 0;
}
