//
//  SciptableTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 4/22/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "conf.h"
#include <deadbeef/common.h>
#include "logger.h"
#include "scriptable.h"
#include "scriptable_dsp.h"
#include "scriptable_encoder.h"
#include <gtest/gtest.h>

@interface ScriptableTestsDummyClass: NSObject
@end
@implementation ScriptableTestsDummyClass
@end

class ScriptableTests: public ::testing::Test {
protected:

    scriptableItem_t *root;

    void SetUp() override {
        // FIXME: convert to C++ / make cross-platform
        NSString *path = [[NSBundle bundleForClass:ScriptableTestsDummyClass.class].resourcePath stringByAppendingString:@"/PresetManagerData"];
        strcpy (dbconfdir, path.UTF8String);
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);
        root = scriptableItemAlloc();
    }

    void TearDown() override {
        scriptableItemFree(root);
    }
};

TEST_F(ScriptableTests, test_LoadDSPPreset_ReturnsExpectedData) {
    scriptableDspLoadPresets (root);
    scriptableItem_t *dspRoot = scriptableDspRoot (root);
    EXPECT_EQ(2, scriptableItemNumChildren (dspRoot));
}

TEST_F(ScriptableTests, test_DSPPreset_Has3Plugins) {
    scriptableDspLoadPresets (root);
    scriptableItem_t *dspRoot = scriptableDspRoot (root);

    scriptableItem_t *preset = scriptableItemNext(scriptableItemChildren(dspRoot));
    EXPECT_EQ(3, scriptableItemNumChildren (preset));
}

TEST_F(ScriptableTests, test_DSPPreset_HasExpectedPluginIds) {
    scriptableDspLoadPresets (root);
    scriptableItem_t *dspRoot = scriptableDspRoot (root);

    scriptableItem_t *preset = scriptableItemNext(scriptableItemChildren(dspRoot));
    scriptableItem_t *plugin = scriptableItemChildren(preset);

    const char *pluginId = scriptableItemPropertyValueForKey(plugin, "pluginId");
    EXPECT_TRUE(pluginId);
    EXPECT_STREQ(pluginId, "supereq");

    plugin = scriptableItemNext(plugin);
    pluginId = scriptableItemPropertyValueForKey(plugin, "pluginId");
    EXPECT_TRUE(pluginId);
    EXPECT_STREQ(pluginId, "SRC");

    plugin = scriptableItemNext(plugin);
    pluginId = scriptableItemPropertyValueForKey(plugin, "pluginId");
    EXPECT_TRUE(pluginId);
    EXPECT_STREQ(pluginId, "m2s");
}

TEST_F(ScriptableTests, test_LoadEncoderPreset_ReturnsExpectedData) {
    scriptableEncoderLoadPresets (root);
    scriptableItem_t *encoderRoot = scriptableEncoderRoot (root);
    EXPECT_EQ(1, scriptableItemNumChildren (encoderRoot));
}

TEST_F(ScriptableTests, test_EncoderPreset_HasNoChildren) {
    scriptableEncoderLoadPresets (root);
    scriptableItem_t *encoderRoot = scriptableEncoderRoot (root);

    scriptableItem_t *preset = scriptableItemChildren(encoderRoot);
    EXPECT_EQ(scriptableItemChildren(preset), nullptr);
}

TEST_F(ScriptableTests, test_EncoderPreset_HasEncoderProperty) {
    scriptableEncoderLoadPresets (root);
    scriptableItem_t *encoderRoot = scriptableEncoderRoot (root);

    scriptableItem_t *preset = scriptableItemChildren(encoderRoot);

    const char *val = scriptableItemPropertyValueForKey(preset, "encoder");
    EXPECT_TRUE(val);
    EXPECT_STREQ(val, "cp %i %o");
}

TEST_F(ScriptableTests, test_ScriptableToConverterEncPreset_EmptyData_CreatesDefault) {
    ddb_encoder_preset_t preset;
    scriptableItem_t *item = scriptableItemAlloc();
    scriptableEncoderPresetToConverterEncoderPreset (item, &preset);
    EXPECT_TRUE(preset.ext);
    EXPECT_TRUE(preset.encoder);
    EXPECT_TRUE(preset.method==0);
    EXPECT_TRUE(preset.tag_id3v2==0);
    EXPECT_TRUE(preset.tag_id3v1==0);
    EXPECT_TRUE(preset.tag_apev2==0);
    EXPECT_TRUE(preset.tag_flac==0);
    EXPECT_TRUE(preset.tag_oggvorbis==0);
    EXPECT_TRUE(preset.tag_mp3xing==0);
    EXPECT_TRUE(preset.tag_mp4==0);
    EXPECT_TRUE(preset.id3v2_version==0);
    free (preset.ext);
    free (preset.encoder);
}

TEST_F(ScriptableTests, test_DSPPreset_HasPassThrough) {
    scriptableDspLoadPresets (root);
    scriptableItem_t *dspRoot = scriptableDspRoot (root);
    int numPresets = scriptableItemNumChildren (dspRoot);
    EXPECT_EQ(numPresets, 2);

    scriptableItem_t *preset = scriptableItemChildren(dspRoot);
    EXPECT_EQ(0, scriptableItemNumChildren (preset));

    const char *name = scriptableItemPropertyValueForKey(preset, "name");
    EXPECT_TRUE(!strcmp (name, "Pass-through"));
}
