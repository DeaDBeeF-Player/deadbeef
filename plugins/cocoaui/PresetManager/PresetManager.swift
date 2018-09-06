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

    // preset parameters, for one-level case like encoder preset
    var parameters : [String : String]?

    // list of preset sub items, for multi-level case like dsp preset
    var subItems : [PresetSubItem]?

    // optional dictionary of additional things, like read-only flags etc
    var extraProperties : [String:Any]?
}

protocol PresetManagerDelegate {
    // for when the names need to be reformatted before display
    func getDisplayName (data: PresetData, index: Int) throws -> String?

    // return true if the item can be edited
    func isEditable (data: PresetData, index: Int) -> Bool

    // return true if the item needs to be saved
    func isSaveable (data: PresetData, index: Int) -> Bool
}

protocol PresetSerializer {
    // load data into manager.data
    func load (manager : PresetManager) throws

    // save data from manager.data
    func save (manager : PresetManager) throws
}

class PresetSerializerJSON : PresetSerializer {
    func load(manager: PresetManager) throws {
    }

    func save(manager: PresetManager) throws {
    }

}

class PresetManager {
    // list of presets
    var _data : [PresetData]
    var _className : String
    var _saveName : String
    var _delegate : PresetManagerDelegate?
    var _serializer : PresetSerializer

    init (className:String, saveName:String, delegate:PresetManagerDelegate?) {
        _className = className
        _saveName = saveName
        _delegate = delegate
        _serializer = PresetSerializerJSON()
        _data = []
    }

    init (className:String, saveName:String, delegate:PresetManagerDelegate?, serializer:PresetSerializer) {
        _className = className
        _saveName = saveName
        _delegate = delegate
        _serializer = serializer
        _data = []
    }

    func load() throws {
        try _serializer.load(manager: self)
    }

    func save() throws {
        try _serializer.save(manager: self)
    }
}

