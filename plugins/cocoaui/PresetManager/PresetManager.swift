import Foundation
import Cocoa

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

    // generate a dropdown box for selecting a preset
    func createSelectorUI (container : NSView)
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

@objc class PresetManager : NSObject {
    // list of presets
    var data : [PresetData]
    var domain : String
    var context : String
    var delegate : PresetManagerDelegate?
    var serializer : PresetSerializer

    var selectedPreset : Int = -1

    // preset domain is the whole system name, e.g. "dsp" or "encoder"
    // context is a specific user of the domain, e.g. "player" or "converter"
    convenience init (domain:String, context:String, delegate:PresetManagerDelegate?) {
        self.init (domain:domain, context:context, delegate:delegate, serializer:PresetSerializerJSON())
    }

    init (domain:String, context:String, delegate:PresetManagerDelegate?, serializer:PresetSerializer) {
        self.domain = domain
        self.context = context
        self.delegate = delegate
        self.serializer = serializer
        self.data = []
        self.selectedPreset = Int(conf_get_int("\(domain).\(context)", -1))
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

    @objc public func createSelectorUI(container : NSView) {
        delegate?.createSelectorUI(container: container)
    }
}

