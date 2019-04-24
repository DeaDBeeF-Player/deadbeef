//
//  SciptableSelectViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "ScriptableSelectViewController.h"

@interface ScriptableSelectViewController ()
@property (weak) IBOutlet NSPopUpButton *nameList;
@property (weak) IBOutlet NSButton *browseButton;

@end

@implementation ScriptableSelectViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}

- (void)setScriptable:(scriptableItem_t *)scriptable {
    [self.nameList removeAllItems];
    for (scriptableItem_t *c = scriptable->children; c; c = c->next) {
        const char *name = scriptableItemPropertyValueForKey(c, "name");
        if (name) {
            [self.nameList addItemWithTitle:[NSString stringWithUTF8String:name]];
        }
    }
}

- (IBAction)nameSelectedAction:(id)sender {
}

- (IBAction)browseButtonAction:(id)sender {
}

@end
