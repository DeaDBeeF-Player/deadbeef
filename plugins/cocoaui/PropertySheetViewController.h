#import <Cocoa/Cocoa.h>

@class PropertySheetViewController;

@protocol PropertySheetDataSource
@required
- (NSString *)propertySheet:(PropertySheetViewController *)vc configForItem:(id)item;
- (NSString *)propertySheet:(PropertySheetViewController *)vc valueForKey:(NSString *)key def:(NSString *)def item:(id)item;
- (void)propertySheet:(PropertySheetViewController *)vc setValue:(NSString *)value forKey:(NSString *)key item:(id)item;
@optional
- (id)propertySheet:(PropertySheetViewController *)vc itemForName:(NSString *)name;
- (id)propertySheet:(PropertySheetViewController *)vc subItemWithIndex:(NSInteger)index item:(id)item;
- (NSInteger)propertySheet:(PropertySheetViewController *)vc subItemCount:(id)item;
- (void)propertySheet:(PropertySheetViewController *)vc enumerateItemsOfType:(NSString *)type block:(BOOL (^)(id item))block;
- (void)propertySheetBeginChanges;
- (void)propertySheetCommitChanges;
@end

@interface PropertySheetViewController : NSViewController<NSTextFieldDelegate>

@property (weak,nonatomic) IBOutlet id<PropertySheetDataSource> dataSource;
@property (weak) IBOutlet id item;

- (void)save;
- (void)reset;

@end
