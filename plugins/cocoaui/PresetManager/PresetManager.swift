import Foundation
import Cocoa

protocol Scriptable {
    func getScript() -> String
}

protocol ScriptableFactory {
    static func create(id : String) -> Scriptable
}

class DSPNode : Scriptable {
    let script : String
    init (script: String) {
        self.script = script
    }
    func getScript () -> String {
        return script
    }
}

class DSPNodeFactory : ScriptableFactory {
    static func create (id : String) -> Scriptable {
        let plug = plug_get_dsp_for_id (id);
        let script = plug!.pointee.configdialog!
        let data = Data(bytes: script, count: Int(strlen(script)))
        return DSPNode(script: String(data: data, encoding: String.Encoding.utf8)!)
    }
}



// One item of a preset
struct PresetSubItem {
    init (id: String) {
        self.id = id
        self.enabled = false
        parameters = [:]
    }
    var id : String
    var enabled : Bool
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
    var parentWindow : NSWindow!
    var sheet : NSWindow!

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
    @objc func presetSelected (sender:NSPopUpButton) {
        selectedPreset = sender.indexOfSelectedItem
        conf_set_int ("\(domain).\(context)", Int32(selectedPreset))
    }

    @objc public func initSelectorPopUpButton (_ btn : NSPopUpButton) {
        for d in data {
            btn.addItem(withTitle: d.name)
        }
        btn.action = #selector(presetSelected(sender:))
        btn.target = self
    }

    @objc public func configure (presetIndex: Int, subItemIndex:Int, sheet:NSWindow, parentWindow:NSWindow, viewController:PluginConfigurationViewController) {
        let p = presetIndex == -1 ? currentPreset : data[presetIndex];
        let plug = plug_get_dsp_for_id (p.subItems![subItemIndex].id)

        let accessor = PluginConfigurationValueAccessorDSP.init(presetManager: self, presetIndex: Int32(presetIndex), subItemIndex: Int32(subItemIndex))
        
        viewController.initPluginConfiguration(plug!.pointee.configdialog, accessor: accessor)

        self.parentWindow = parentWindow
        self.sheet = sheet
        NSApp.beginSheet(sheet, modalFor: parentWindow, modalDelegate: self, didEnd: #selector(didEndDspConfigPanel(sheet:returnCode:contextInfo:)), contextInfo: nil)
        
    }

    @objc func didEndDspConfigPanel(sheet:NSWindow, returnCode:NSInteger, contextInfo:UnsafeMutableRawPointer!) {
        sheet.orderOut(parentWindow)
    }

    @objc func dspConfigCancelAction(sender:Any?) {
        // FIXME
        // [NSApp endSheet:_dspConfigPanel returnCode:NSCancelButton];
    }

    @objc func dspConfigOkAction(sender:Any?) {
        // FIXME
        // [NSApp endSheet:_dspConfigPanel returnCode:NSOKButton];
    }


}

