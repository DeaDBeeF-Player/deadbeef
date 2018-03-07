#import "DSPPresetListDataSource.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DSPPresetListDataSource {
    NSArray *_presetList;
}

- (DSPPresetListDataSource *)init {
    self = [super init];

    [self initPresetList];

    return self;
}

- (void)initPresetList {
    NSString *presetPath = [[NSString stringWithUTF8String:deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG)] stringByAppendingString:@"/presets/dsp"];

    NSArray *dirFiles = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:presetPath error:nil];

    _presetList = [dirFiles filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self ENDSWITH '.txt'"]];
}

- (id)comboBox:(NSComboBox *)comboBox objectValueForItemAtIndex:(NSInteger)index {
    return _presetList[index];
}

- (NSInteger)numberOfItemsInComboBox:(NSComboBox *)comboBox {
    return [_presetList count];
}
@end
