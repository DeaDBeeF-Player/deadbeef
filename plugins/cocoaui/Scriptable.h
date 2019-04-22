//
//  Scriptable.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/22/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Scriptable : NSObject

+ (Scriptable *)sharedRootNode;

- (id)initWithParent:(Scriptable *)parent;
- (id)initWithName:(NSString *)name type:(NSString *)type parent:(Scriptable * _Nullable)parent;
- (NSInteger)indexOfChild:(Scriptable *)child;
- (Scriptable *)createSubItemOfType:(NSString *)type;

@property (weak, nonatomic) Scriptable *parent;
@property (nonatomic) NSMutableDictionary *properties;
@property (nonatomic) NSMutableArray <Scriptable *> *children;

@property (nonatomic,readonly) BOOL hasSubItems;
@property (nonatomic,readonly) NSArray <NSString *> *subItemTypes;
@property (nonatomic) NSString *configDialog;

@end

NS_ASSUME_NONNULL_END
