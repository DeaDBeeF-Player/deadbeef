#import <Foundation/Foundation.h>

@interface DSPPresetListDataSource : NSObject <NSComboBoxDataSource>

- (DSPPresetListDataSource *)init;
- (void)initPresetList;

@end
