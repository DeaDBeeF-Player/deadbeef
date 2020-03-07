#import <Cocoa/Cocoa.h>

@class PropertySheetContentView;

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

@property (nonatomic) IBOutlet PropertySheetContentView *contentView;
@property (nonatomic,readonly) NSSize contentSize;

@property (nonatomic, weak) IBOutlet id<PropertySheetDataSource> dataSource;
@property (weak) IBOutlet id item;

@property (nonatomic) CGFloat labelFontSize;
@property (nonatomic) CGFloat contentFontSize;
@property (nonatomic) NSInteger topMargin;
@property (nonatomic) BOOL autoAlignLabels;
@property (nonatomic) NSInteger labelFixedWidth;
@property (nonatomic) NSInteger sliderLabelWidth;
@property (nonatomic) NSInteger unitSpacing;

- (void)reset;
- (void)reload;

@end
