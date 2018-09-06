import Foundation

// one item of a preset
struct PresetSubItem {
    init (id: String) {
        self.id = id
        parameters = [:]
    }
    var id : String
    var parameters : [String : String]
}

// the whole preset
struct PresetData {
    init (name: String) {
        self.name = name
    }
    // the preset name
    var name : String

    // list of preset sub items, for multi-level case like dsp preset;
    // for the rest of the cases (flat presets) gonna be a single-item list
    var subItems : [PresetSubItem]?

    // optional dictionary of additional things, like read-only flags etc
    var extraProperties : [String:Any]?
}

protocol PresetManagerDelegate {
    // for when the names need to be reformatted before display
    func getDisplayName (index: Int) throws -> String?

    // return true if the item can be edited
    func isEditable (index: Int) -> Bool

    // return true if the item needs to be saved
    func isSaveable (index: Int) -> Bool
}

protocol PresetSerializer {
    // load data into manager.data
    func load () throws

    // save data from manager.data
    func save () throws

    func save (presetIndex:Int) throws
}

class PresetSerializerJSON : PresetSerializer {
    func load() throws {
    }

    func save() throws {
    }

    func save (presetIndex:Int) throws {
    }
}

class PresetManager {
    // list of presets
    var data : [PresetData]
    var className : String
    var saveName : String
    var delegate : PresetManagerDelegate?
    var serializer : PresetSerializer

    init (className:String, saveName:String, delegate:PresetManagerDelegate?) {
        self.className = className
        self.saveName = saveName
        self.delegate = delegate
        self.serializer = PresetSerializerJSON()
        self.data = []
    }

    init (className:String, saveName:String, delegate:PresetManagerDelegate?, serializer:PresetSerializer) {
        self.className = className
        self.saveName = saveName
        self.delegate = delegate
        self.serializer = serializer
        self.data = []
    }

    func load() throws {
        try serializer.load()
    }

    func save() throws {
        try serializer.save()
    }

    func save(presetIndex:Int) throws {
        try serializer.save(presetIndex:presetIndex)
    }
}

