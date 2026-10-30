/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <gtest/gtest.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>
#include "../plugins/gtkui/layoutmanager.h"

static const char *kCurrentVersion = "1.9.0";

class GtkLayoutManagerTests : public ::testing::Test {
protected:
    std::string tempDir;

    void SetUp() override {
        char tmpl[] = "/tmp/ddb_layout_tests_XXXXXX";
        ASSERT_NE(mkdtemp (tmpl), nullptr);
        tempDir = tmpl;
    }

    void TearDown() override {
        std::string cmd = "rm -rf '" + tempDir + "'";
        (void)system (cmd.c_str ());
    }

    std::string writeFile (const std::string &dir, const std::string &name, const std::string &contents) {
        std::string path = dir + "/" + name;
        FILE *fp = fopen (path.c_str (), "wb");
        EXPECT_NE (fp, nullptr);
        if (fp != nullptr) {
            fwrite (contents.c_str (), 1, contents.size (), fp);
            fclose (fp);
        }
        return path;
    }

    std::string writeEnvelope (const std::string &magic, const std::string &version, const std::string &layout) {
        std::string json = "{\"magic\":\"" + magic + "\",\"version\":\"" + version + "\",\"layout_gtkui\":" + layout + "}";
        return writeFile (tempDir, "test.ddblayout", json);
    }

    std::vector<std::string> scanNames (const std::string &dir) {
        std::vector<std::string> names;
        layoutmanager_entry_t *entries = layoutmanager_scan_directory (dir.c_str ());
        for (layoutmanager_entry_t *e = entries; e != nullptr; e = e->next) {
            names.push_back (e->name);
        }
        layoutmanager_entries_free (entries);
        return names;
    }
};

// MARK: - Reading and writing

TEST_F (GtkLayoutManagerTests, test_saveThenLoad_roundTripsLayout) {
    json_t *layout = json_loads ("{\"type\":\"vbox\",\"children\":[{\"type\":\"tabbed_playlist\"}]}", 0, nullptr);
    std::string path = tempDir + "/roundtrip.ddblayout";

    EXPECT_EQ (layoutmanager_save_file (path.c_str (), kCurrentVersion, layout), LAYOUTMANAGER_OK);

    layoutmanager_error_t error;
    json_t *loaded = layoutmanager_load_file (path.c_str (), kCurrentVersion, &error);
    ASSERT_NE (loaded, nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_OK);
    EXPECT_TRUE (json_equal (loaded, layout));

    json_decref (loaded);
    json_decref (layout);
}

TEST_F (GtkLayoutManagerTests, test_save_writesMagicVersionAndLayout) {
    json_t *layout = json_loads ("{\"type\":\"tabbed_playlist\"}", 0, nullptr);
    std::string path = tempDir + "/envelope.ddblayout";
    layoutmanager_save_file (path.c_str (), kCurrentVersion, layout);

    json_t *root = json_load_file (path.c_str (), 0, nullptr);
    ASSERT_NE (root, nullptr);

    EXPECT_STREQ (json_string_value (json_object_get (root, "magic")), "ddblayout");
    EXPECT_STREQ (json_string_value (json_object_get (root, "version")), "1.9.0");
    EXPECT_TRUE (json_equal (json_object_get (root, "layout_gtkui"), layout));

    json_decref (root);
    json_decref (layout);
}

TEST_F (GtkLayoutManagerTests, test_save_unwritablePath_fails) {
    json_t *layout = json_object ();

    EXPECT_EQ (
        layoutmanager_save_file ((tempDir + "/missing/dir/file.ddblayout").c_str (), kCurrentVersion, layout),
        LAYOUTMANAGER_ERROR_IO);

    json_decref (layout);
}

TEST_F (GtkLayoutManagerTests, test_load_currentVersion_succeeds) {
    std::string path = writeEnvelope ("ddblayout", "1.9.0", "{\"type\":\"tabbed_playlist\"}");

    json_t *layout = layoutmanager_load_file (path.c_str (), kCurrentVersion, nullptr);

    ASSERT_NE (layout, nullptr);
    EXPECT_STREQ (json_string_value (json_object_get (layout, "type")), "tabbed_playlist");
    json_decref (layout);
}

TEST_F (GtkLayoutManagerTests, test_load_olderVersion_succeeds) {
    std::string path = writeEnvelope ("ddblayout", "1.8.9", "{}");

    json_t *layout = layoutmanager_load_file (path.c_str (), kCurrentVersion, nullptr);

    ASSERT_NE (layout, nullptr);
    json_decref (layout);
}

TEST_F (GtkLayoutManagerTests, test_load_wrongMagic_fails) {
    std::string path = writeEnvelope ("other", "1.9.0", "{}");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_WRONG_MAGIC);
}

TEST_F (GtkLayoutManagerTests, test_load_higherPointVersion_fails) {
    std::string path = writeEnvelope ("ddblayout", "1.9.1", "{}");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_UNSUPPORTED_VERSION);
}

TEST_F (GtkLayoutManagerTests, test_load_higherMinorVersion_fails) {
    std::string path = writeEnvelope ("ddblayout", "1.10.0", "{}");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_UNSUPPORTED_VERSION);
}

TEST_F (GtkLayoutManagerTests, test_load_higherMajorVersion_fails) {
    std::string path = writeEnvelope ("ddblayout", "2.0.0", "{}");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_UNSUPPORTED_VERSION);
}

TEST_F (GtkLayoutManagerTests, test_load_malformedVersion_fails) {
    for (const char *version : { "", "1", "1.9", "1.9.0.0", "a.b.c", "-1.9.0", "1.9.x", "1..0", " 1.9.0" }) {
        std::string path = writeEnvelope ("ddblayout", version, "{}");

        layoutmanager_error_t error;
        EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr) << version;
        EXPECT_EQ (error, LAYOUTMANAGER_ERROR_INVALID_FILE) << version;
    }
}

TEST_F (GtkLayoutManagerTests, test_load_missingLayout_fails) {
    std::string path = writeFile (tempDir, "test.ddblayout", "{\"magic\":\"ddblayout\",\"version\":\"1.9.0\"}");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_INVALID_FILE);
}

TEST_F (GtkLayoutManagerTests, test_load_layoutOfAnotherUI_fails) {
    // a file saved by cocoaui has "layout_cocoaui" and no "layout_gtkui"
    std::string path = writeFile (
        tempDir,
        "test.ddblayout",
        "{\"magic\":\"ddblayout\",\"version\":\"1.9.0\",\"layout_cocoaui\":{}}");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_INVALID_FILE);
}

TEST_F (GtkLayoutManagerTests, test_load_layoutNotObject_fails) {
    std::string path = writeEnvelope ("ddblayout", "1.9.0", "[1,2]");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_INVALID_FILE);
}

TEST_F (GtkLayoutManagerTests, test_load_invalidJSON_fails) {
    std::string path = writeFile (tempDir, "test.ddblayout", "not json");

    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file (path.c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_INVALID_FILE);
}

TEST_F (GtkLayoutManagerTests, test_load_missingFile_fails) {
    layoutmanager_error_t error;
    EXPECT_EQ (layoutmanager_load_file ((tempDir + "/nonexistent.ddblayout").c_str (), kCurrentVersion, &error), nullptr);
    EXPECT_EQ (error, LAYOUTMANAGER_ERROR_IO);
}

// MARK: - Folders

TEST_F (GtkLayoutManagerTests, test_scanDirectory_listsOnlyLayoutFilesSortedByName) {
    writeFile (tempDir, "Beta.ddblayout", "{}");
    writeFile (tempDir, "alpha.ddblayout", "{}");
    writeFile (tempDir, "Gamma.DDBLAYOUT", "{}");
    writeFile (tempDir, "notes.txt", "");
    writeFile (tempDir, ".hidden.ddblayout", "{}");
    writeFile (tempDir, ".ddblayout", "{}");

    std::vector<std::string> expected = { "alpha", "Beta", "Gamma" };
    EXPECT_EQ (scanNames (tempDir), expected);
}

TEST_F (GtkLayoutManagerTests, test_scanDirectory_entryHasFullPath) {
    std::string path = writeFile (tempDir, "Layout.ddblayout", "{}");

    layoutmanager_entry_t *entries = layoutmanager_scan_directory (tempDir.c_str ());

    ASSERT_NE (entries, nullptr);
    EXPECT_STREQ (entries->path, path.c_str ());
    layoutmanager_entries_free (entries);
}

TEST_F (GtkLayoutManagerTests, test_scanDirectory_namesWithSpacesAndSymbols_keptAsIs) {
    writeFile (tempDir, "Tabs + Cover Art (Default).ddblayout", "{}");

    std::vector<std::string> expected = { "Tabs + Cover Art (Default)" };
    EXPECT_EQ (scanNames (tempDir), expected);
}

TEST_F (GtkLayoutManagerTests, test_scanDirectory_missingDirectory_isEmpty) {
    EXPECT_EQ (layoutmanager_scan_directory ((tempDir + "/missing").c_str ()), nullptr);
}

TEST_F (GtkLayoutManagerTests, test_scanDirectory_nullDirectory_isEmpty) {
    EXPECT_EQ (layoutmanager_scan_directory (nullptr), nullptr);
}

TEST_F (GtkLayoutManagerTests, test_createDirectory_createsMissingParents) {
    std::string path = tempDir + "/a/b/gtkui_layouts";

    EXPECT_EQ (layoutmanager_create_directory (path.c_str ()), 0);
    EXPECT_EQ (access (path.c_str (), F_OK), 0);
}

TEST_F (GtkLayoutManagerTests, test_createDirectory_existingDirectory_succeeds) {
    EXPECT_EQ (layoutmanager_create_directory (tempDir.c_str ()), 0);
}

TEST_F (GtkLayoutManagerTests, test_createDirectory_pathIsFile_fails) {
    std::string path = writeFile (tempDir, "file", "");

    EXPECT_NE (layoutmanager_create_directory (path.c_str ()), 0);
}

TEST_F (GtkLayoutManagerTests, test_createDirectory_nullPath_fails) {
    EXPECT_NE (layoutmanager_create_directory (nullptr), 0);
}
