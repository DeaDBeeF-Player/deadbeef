import Foundation
import Cocoa

@objc class DSPPresetController : NSObject, PresetManagerDelegate, PresetSerializer, NSTableViewDataSource {
    enum DSPPresetControllerError : Error {
        case InvalidPreset
        case AlreadyLoaded
    }

    @objc var presetMgr : PresetManager!

    @objc class func create(context:String) throws -> DSPPresetController {
        return try DSPPresetController(context:context)
    }

    init(context:String) throws {
        super.init()
        presetMgr = PresetManager(domain: "dsp", context: context, delegate: self, serializer: self)
        try presetMgr.load()
    }

    // PresetManagerDelegate

    func isEditable(index: Int) -> Bool {
        return true
    }

    func isSaveable(index: Int) -> Bool {
        return true
    }

    // PresetSerializer

    func load() throws {
        if (presetMgr.data.items.count != 0) {
            throw DSPPresetControllerError.AlreadyLoaded
        }
        let conpath_u8 = plug_get_system_dir (Int32(DDB_SYS_DIR_CONFIG.rawValue))
        let data = Data(bytes: conpath_u8!, count: Int(strlen(conpath_u8)))
        let confpath = String(data: data, encoding: String.Encoding.utf8)!
        let str = confpath + "/presets/dsp"

        // load current preset
        if let preset = try loadPreset(name: "current", fname: confpath + "/dspconfig", hasEnabledFlag: true) {
            let dsppreset = DSPPreset.create(type:"dsppreset")
            dsppreset.load (data:preset)
            presetMgr.data.items.append(dsppreset)
        }

        // find all txt files in the folder
        let fileManager = FileManager.default
        if let enumerator:FileManager.DirectoryEnumerator = fileManager.enumerator(atPath: str) {
            while let element = enumerator.nextObject() as? String {
                if (element.hasSuffix(".txt")) {
                    // Can't use the original dsp preset parser, since it loads stuff into actual objects instead of a dict
                    if let preset = try loadPreset(name: String(element[..<element.index(element.endIndex, offsetBy: -4)]), fname: str+"/"+element, hasEnabledFlag: false) {
                        let dsppreset = DSPPreset.create(type:"dsppreset")
                        dsppreset.load (data:preset)
                        presetMgr.data.items.append(dsppreset)
                    }
                }
            }
        }
    }

    func save() throws {
    }

    func save(presetIndex:Int) throws {
    }

    /*
    // NSTableViewDataSource
    func numberOfRows(in:NSTableView) -> Int {
        return presetMgr.data[0].subItems!.count
    }

    func tableView(_ tableView: NSTableView,
                   objectValueFor tableColumn: NSTableColumn?,
                   row: Int) -> Any? {
        let id = presetMgr.data[0].subItems![row].id
        if let plug = plug_get_for_id(id) {
            let name = plug.pointee.name!
            let data = Data(bytes: name, count: Int(strlen(name)))
            return String(data: data, encoding: String.Encoding.utf8)
        }
        else {
            return "<missing plugin>"
        }
    }
    // ui utilities
    @objc func addItem (id: String) {
        presetMgr.data[0].subItems?.append(PresetSubItem(id:id))
    }

    @objc func removeItem (index: Int) {
        presetMgr.data[0].subItems?.remove(at: index)
    }
     */

    // internal

    // a preset is a list of dictionaries
    func loadPreset (name: String, fname : String, hasEnabledFlag : Bool) throws -> [String:Any]? {
        var preset : [String:Any] = ["name":name]
        var nodes : [[String:Any]] = []

        let data = try String(contentsOfFile: fname, encoding: .utf8)
        let lines = data.components(separatedBy: .newlines)
        var l = 0
        while (l < lines.count) {
            var line = lines[l].trimmingCharacters(in: .whitespaces)
            if (line.count == 0) {
                l = l+1
                continue
            }
            let list = line.split(separator: " ")
            l = l+1

            let cnt = hasEnabledFlag ? 3 : 2

            if (list.count != cnt || list[cnt-1] != "{") {
                throw DSPPresetControllerError.InvalidPreset
            }
            var node : [String:Any] = ["type":String(list[0])]
            if (hasEnabledFlag && list[1] != "0") {
                node["enabled"] = true
            }
            var idx = 0

            var items : [[String:Any]] = [];
            while (l < lines.count && lines[l] != "}") {
                line = lines[l].trimmingCharacters(in: .whitespaces)
                items.append(["name":String(idx),"value":line])
                l = l+1
                idx = idx+1
            }
            node["items"] = items

            if (l == lines.count) {
                throw DSPPresetControllerError.InvalidPreset // missing curly brace
            }
            l = l+1
            nodes.append (node)
        }
        preset["items"] = nodes;
        return preset
    }

}


