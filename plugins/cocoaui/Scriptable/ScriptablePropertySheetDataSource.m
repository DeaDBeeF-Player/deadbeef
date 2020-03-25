//
//  ScriptablePropertySheetDataSource.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/24/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "ScriptablePropertySheetDataSource.h"

@interface ScriptablePropertySheetDataSource () {
    scriptableItem_t *_scriptable;
    BOOL _multipleChanges;
}
@end

@implementation ScriptablePropertySheetDataSource
- (instancetype)initWithScriptable:(scriptableItem_t *)scriptable {
    self = [super init];
    _scriptable = scriptable;
    return self;
}

- (NSString *)propertySheet:(PropertySheetViewController *)vc configForItem:(id)item {
    const char *config = scriptableItemPropertyValueForKey(_scriptable, "configDialog");
    return config ? [NSString stringWithUTF8String:config] : nil;
}

- (NSString *)propertySheet:(PropertySheetViewController *)vc valueForKey:(NSString *)key def:(NSString *)def item:(id)item {
    const char *value = scriptableItemPropertyValueForKey(_scriptable, [key UTF8String]);
    return value ? [NSString stringWithUTF8String:value] : def;
}

- (void)propertySheet:(PropertySheetViewController *)vc setValue:(NSString *)value forKey:(NSString *)key item:(id)item {
    scriptableItemSetPropertyValueForKey(_scriptable, [value UTF8String], [key UTF8String]);
    if (!_multipleChanges) {
        [self.delegate scriptableItemChanged:_scriptable change:ScriptableItemChangeUpdate];
    }
}

- (void)propertySheetBeginChanges {
    _multipleChanges = YES;
}

- (void)propertySheetCommitChanges {
    [self.delegate scriptableItemChanged:_scriptable change:ScriptableItemChangeUpdate];
    _multipleChanges = NO;
}
@end
