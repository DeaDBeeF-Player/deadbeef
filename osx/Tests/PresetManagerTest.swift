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

    func testLoadDSPPreset_ReturnsExpectedData() {
        var ctl : DSPPresetController?;
        XCTAssertNoThrow(ctl = try DSPPresetController(context:"test"))
        XCTAssertEqual(ctl?.presetMgr.data.count, 1)
        XCTAssertEqual(ctl?.presetMgr.data[0].name, "dsppreset")
        XCTAssertEqual(ctl?.presetMgr.data[0].subItems?.count, 3)
        XCTAssertEqual(ctl?.presetMgr.data[0].subItems?[0].id, "supereq")
        XCTAssertEqual(ctl?.presetMgr.data[0].subItems?[1].id, "SRC")
        XCTAssertEqual(ctl?.presetMgr.data[0].subItems?[2].id, "m2s")
    }

}
