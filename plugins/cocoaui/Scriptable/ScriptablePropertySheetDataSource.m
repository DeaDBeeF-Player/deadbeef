//
//  ScriptablePropertySheetDataSource.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 4/24/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
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
    const char *config = _scriptable->configDialog;
    return config ? @(config) : nil;
}

- (BOOL)propertySheet:(PropertySheetViewController *)vc itemIsReadonly:(id)item {
    return _scriptable->isReadonly;
}


- (NSString *)propertySheet:(PropertySheetViewController *)vc valueForKey:(NSString *)key def:(NSString *)def item:(id)item {
    const char *value = scriptableItemPropertyValueForKey(_scriptable, key.UTF8String);
    return value ? @(value) : def;
}

- (void)propertySheet:(PropertySheetViewController *)vc setValue:(NSString *)value forKey:(NSString *)key item:(id)item {
    scriptableItemSetPropertyValueForKey(_scriptable, value.UTF8String, key.UTF8String);
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
