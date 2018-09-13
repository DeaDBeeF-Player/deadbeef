import Foundation
import Cocoa

// One item of a preset
struct PresetSubItem {
    init (id: String) {
        self.id = id
        parameters = [:]
    }
    var id : String
    var parameters : [String : String]
}

// The whole preset
struct PresetData {
    init (name: String) {
        self.name = name
    }
    // The preset name
    var name : String

    // List of preset sub items, for multi-level case like dsp preset;
    // for the rest of the cases (flat presets) gonna be a single-item list
    var subItems : [PresetSubItem]?

    // optional dictionary of additional things, like read-only flags etc
    var extraProperties : [String:Any]?
}

protocol PresetManagerDelegate {
    // For when the names need to be reformatted before display
    func getDisplayName (index: Int) throws -> String?

    // Return true if the item can be edited
    func isEditable (index: Int) -> Bool

    // Return true if the item needs to be saved
    func isSaveable (index: Int) -> Bool

    // Generate a dropdown box for selecting a preset
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
    var currentPreset : PresetData
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
        data = []
        currentPreset = PresetData(name:"Current")
        selectedPreset = Int(conf_get_int("\(domain).\(context)", -1))
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

    // UI code needs to call that when a preset was selected by the user.
    // If no preset is selected, pass -1
    func presetSelected (index:Int) {
        selectedPreset = index;
        conf_set_int ("\(domain).\(context)", Int32(index));
    }

    @objc public func createSelectorUI(container : NSView) {
        delegate?.createSelectorUI(container: container)
    }
}

