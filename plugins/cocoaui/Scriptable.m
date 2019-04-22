//
//  Scriptable.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/22/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "Scriptable.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface Scriptable ()
@end

@implementation Scriptable

+ (Scriptable *)sharedRootNode {
    static Scriptable *rootNode = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        rootNode = [[self alloc] init];
    });
    return rootNode;
}

- (id)init {
    self = [super init];
    self.properties = [[NSMutableDictionary alloc] init];
    self.children = [[NSMutableArray alloc] init];
    return self;
}

- (id)initWithParent:(Scriptable *)parent {
    self = [self init];
    self.parent = parent;
    return self;
}

- (id)initWithName:(NSString *)name type:(NSString *)type parent:(Scriptable *)parent {
    self = [self initWithParent:parent];

    _properties[@"name"] = name;
    _properties[@"type"] = type;

    return self;
}

- (NSInteger)indexOfChild:(Scriptable *)child {
    return [self.children indexOfObject:child];
}

- (Scriptable *)createSubItemOfType:(NSString *)type {
    return nil;
}

@end

@interface ScriptableDSPPreset : Scriptable
@end

@implementation ScriptableDSPPreset

- (NSArray *)subItemTypes {
    NSMutableArray *types = [[NSMutableArray alloc] init];
    DB_dsp_t **dsps = deadbeef->plug_get_dsp_list ();
    for (int i = 0; dsps[i]; i++) {
        [types addObject:[NSString stringWithUTF8String: dsps[i]->plugin.name]];
    }
    return types;
}

- (Scriptable *)createSubItemOfType:(NSString *)type {
    Scriptable *s;
    DB_dsp_t **dsps = deadbeef->plug_get_dsp_list ();
    for (int i = 0; dsps[i]; i++) {
        if ([type isEqualToString:[NSString stringWithUTF8String: dsps[i]->plugin.name]]) {
            s = [[Scriptable alloc] initWithParent:self];
            s.configDialog = [NSString stringWithUTF8String: dsps[i]->configdialog];
            [self.children addObject:s];
            break;
        }
    }
    return s;
}


@end
