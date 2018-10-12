//
//  PresetManagerTest.swift
//  Tests
//
//  Created by Oleksiy Yakovenko on 06/09/2018.
//  Copyright © 2018 Alexey Yakovenko. All rights reserved.
//

import XCTest

class PresetManagerTest: XCTestCase {

    override func setUp() {
        super.setUp()
        let str = Bundle(for: type(of: self)).resourcePath! + "/PresetManagerData"
        set_dbconfdir (str)

        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }

    func testLoadDSPPreset_ReturnsExpectedData() throws {
        let ctl = try DSPPresetController(context:"test")

        let presets = ctl.presetMgr.getItems()
        XCTAssertEqual(presets.count, 1)

        XCTAssertEqual(presets[0].getName(), "mypreset")

        let items = presets[0].getItems()

        XCTAssertEqual(items.count, 3)
        XCTAssertEqual(items[0].getType(), "supereq")
        XCTAssertEqual(items[1].getType(), "SRC")
        XCTAssertEqual(items[2].getType(), "m2s")
    }

}
