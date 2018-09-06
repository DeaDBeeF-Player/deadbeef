import Foundation

class DSPPresetController : PresetManagerDelegate, PresetSerializer {
    enum DSPPresetControllerError : Error {
        case InvalidPreset
    }

    var presetMgr : PresetManager!

    init() throws {
        presetMgr = PresetManager(className: "DSP", saveName: "DSP", delegate: self, serializer: self)
        try presetMgr.load()
    }

    // PresetManagerDelegate

    func getDisplayName(index: Int) throws -> String? {
        return presetMgr.data[index].name
    }

    func isEditable(index: Int) -> Bool {
        return true
    }

    func isSaveable(index: Int) -> Bool {
        return true
    }

    // PresetSerializer

    func load() throws {
        var presets : [PresetData] = []
        let fname = plug_get_system_dir (Int32(DDB_SYS_DIR_CONFIG.rawValue))
        let data = Data(bytes: fname!, count: Int(strlen(fname)))
        let str = String(data: data, encoding: String.Encoding.utf8)! + "/presets/dsp"

        // find all txt files in the folder
        let fileManager = FileManager.default
        if let enumerator:FileManager.DirectoryEnumerator = fileManager.enumerator(atPath: str) {
            while let element = enumerator.nextObject() as? String {
                if (element.hasSuffix(".txt")) {
                    // Can't use the original dsp preset parser, since it loads stuff into actual objects instead of a dict
                    if let preset = try loadPreset(name: String(element[..<element.index(element.endIndex, offsetBy: -4)]), fname: str+"/"+element) {
                        presets.append(preset)
                    }
                }
            }
        }
        presetMgr.data = presets
    }

    func save() throws {
    }

    func save(presetIndex:Int) throws {
    }

    // internal

    // a preset is a list of dictionaries
    func loadPreset (name: String, fname : String) throws -> PresetData? {
        var preset = PresetData(name:name)
        preset.subItems = []
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
            if (list.count != 2 || list[1] != "{") {
                throw DSPPresetControllerError.InvalidPreset
            }
            l = l+1;
            var node = PresetSubItem(id:String(list[0]))
            var idx = 0
            while (l < lines.count && lines[l] != "}") {
                line = lines[l].trimmingCharacters(in: .whitespaces)
                node.parameters[String(idx)] = line
                l = l+1
                idx = idx+1
            }
            if (l == lines.count) {
                throw DSPPresetControllerError.InvalidPreset // missing curly brace
            }
            l = l+1
            preset.subItems?.append (node)
        }
        return preset
    }

}


