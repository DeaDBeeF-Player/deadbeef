//
//  SciptableTests.m
//  Tests
//
//  Created by Alexey Yakovenko on 4/22/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "conf.h"
#include "../../common.h"
#include "../../logger.h"
#include "scriptable.h"
#include "scriptable_dsp.h"
#include "scriptable_encoder.h"

@interface SciptableTests : XCTestCase

@end

@implementation SciptableTests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
    NSString *path = [[[NSBundle bundleForClass:[self class]] resourcePath] stringByAppendingString:@"/PresetManagerData"];
    strcpy (dbconfdir, [path UTF8String]);
    ddb_logger_init ();
    conf_init ();
    conf_enable_saving (0);
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)test_LoadDSPPreset_ReturnsExpectedData {
    scriptableDspLoadPresets ();
    scriptableItem_t *dspRoot = scriptableDspRoot ();
    XCTAssertEqual(2, scriptableItemNumChildren (dspRoot));
    scriptableFree();
}

- (void)test_DSPPreset_Has3Plugins {
    scriptableDspLoadPresets ();
    scriptableItem_t *dspRoot = scriptableDspRoot ();

    scriptableItem_t *preset = dspRoot->children->next;
    XCTAssertEqual(3, scriptableItemNumChildren (preset));
    scriptableFree();
}

- (void)test_DSPPreset_HasExpectedPluginIds {
    scriptableDspLoadPresets ();
    scriptableItem_t *dspRoot = scriptableDspRoot ();

    scriptableItem_t *preset = dspRoot->children->next;
    scriptableItem_t *plugin = preset->children;

    const char *pluginId = scriptableItemPropertyValueForKey(plugin, "pluginId");
    XCTAssert(pluginId);
    XCTAssertEqualObjects([NSString stringWithUTF8String:pluginId], @"supereq");

    plugin = plugin->next;
    pluginId = scriptableItemPropertyValueForKey(plugin, "pluginId");
    XCTAssert(pluginId);
    XCTAssertEqualObjects([NSString stringWithUTF8String:pluginId], @"SRC");

    plugin = plugin->next;
    pluginId = scriptableItemPropertyValueForKey(plugin, "pluginId");
    XCTAssert(pluginId);
    XCTAssertEqualObjects([NSString stringWithUTF8String:pluginId], @"m2s");

    scriptableFree();
}

- (void)test_LoadEncoderPreset_ReturnsExpectedData {
    scriptableEncoderLoadPresets ();
    scriptableItem_t *encoderRoot = scriptableEncoderRoot ();
    XCTAssertEqual(1, scriptableItemNumChildren (encoderRoot));
    scriptableFree();
}

- (void)test_EncoderPreset_HasNoChildren {
    scriptableEncoderLoadPresets ();
    scriptableItem_t *encoderRoot = scriptableEncoderRoot ();

    scriptableItem_t *preset = encoderRoot->children;
    XCTAssertTrue(preset->children == NULL);

    scriptableFree();
}

- (void)test_EncoderPreset_HasEncoderProperty {
    scriptableEncoderLoadPresets ();
    scriptableItem_t *encoderRoot = scriptableEncoderRoot ();

    scriptableItem_t *preset = encoderRoot->children;

    const char *val = scriptableItemPropertyValueForKey(preset, "encoder");
    XCTAssert(val);
    XCTAssertEqualObjects([NSString stringWithUTF8String:val], @"cp %i %o");

    scriptableFree();
}

- (void)test_ScriptableToConverterEncPreset_EmptyData_CreatesDefault {
    ddb_encoder_preset_t preset;
    scriptableItem_t *item = scriptableItemAlloc();
    scriptableEncoderPresetToConverterEncoderPreset (item, &preset);
    XCTAssert(preset.ext);
    XCTAssert(preset.encoder);
    XCTAssert(preset.method==0);
    XCTAssert(preset.tag_id3v2==0);
    XCTAssert(preset.tag_id3v1==0);
    XCTAssert(preset.tag_apev2==0);
    XCTAssert(preset.tag_flac==0);
    XCTAssert(preset.tag_oggvorbis==0);
    XCTAssert(preset.tag_mp3xing==0);
    XCTAssert(preset.tag_mp4==0);
    XCTAssert(preset.id3v2_version==0);
    free (preset.ext);
    free (preset.encoder);
}

- (void)test_DSPPreset_HasPassThrough {
    scriptableDspLoadPresets ();
    scriptableItem_t *dspRoot = scriptableDspRoot ();
    int numPresets = scriptableItemNumChildren (dspRoot);
    XCTAssertEqual(numPresets, 2);

    scriptableItem_t *preset = dspRoot->children;
    XCTAssertEqual(0, scriptableItemNumChildren (preset));

    const char *name = scriptableItemPropertyValueForKey(preset, "name");
    XCTAssert(!strcmp (name, "Pass-through"));

    scriptableFree();
}

@end
