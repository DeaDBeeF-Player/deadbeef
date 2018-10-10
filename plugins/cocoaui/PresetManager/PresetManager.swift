import Foundation
import Cocoa

struct ScriptableData {
    var type : String
    var name : String?
    var value : String?
    var items : [Scriptable]?
}

@objc(Scriptable)
protocol Scriptable {
    @objc static func create (type: String) -> Scriptable

    func getScript() -> String?
    @objc func displayName() -> String
    @objc func load (data: NSDictionary)
}

@objc(ScriptableKeyValue)
class ScriptableKeyValue : NSObject, Scriptable {
    var _data : ScriptableData

    init(_ type: String) {
        _data = ScriptableData(type:type, name:nil, value:nil, items:nil)
    }

    @objc static func create (type: String) -> Scriptable {
        return ScriptableKeyValue(type)
    }

    func getScript() -> String? {
        return nil
    }

    @objc func displayName() -> String {
        return ""
    }

    @objc func load (data: NSDictionary) {
        if let n = data.value(forKey: "name") as? String {
            _data.name = n;
        }
        else if let n = data.value(forKey: "name") as? Int {
            _data.name = String(n);
        }

        if let v = data.value(forKey: "value") as? String {
            _data.value = v;
        }
        else if let v = data.value(forKey: "value") as? Int {
            _data.value = String(v);
        }
        else if let v = data.value(forKey: "value") as? Float {
            _data.value = String(v);
        }
    }
}

@objc(DSPPreset)
class DSPPreset : NSObject, Scriptable {
    var plugin : UnsafePointer<DB_dsp_t>
    var _data : ScriptableData?
    func data() -> ScriptableData? {
        return _data;
    }

    func getScript() -> String? {
        return ""
    }

    init (plugin:UnsafePointer<DB_dsp_t>) {
        self.plugin = plugin
    }

    static func create (type: String) -> Scriptable {
        if let p = plug_get_dsp_for_id(type) {
            return DSPNode (plugin: p)
        }
        return DummyNode.create(type:"<Missing \(type)>")
    }
    @objc func displayName() -> String {
        if let n = _data?.name {
            return n
        }
        return "null"
    }
    @objc func load (data: NSDictionary) {
    }
}

@objc(DSPNode)
class DSPNode : NSObject, Scriptable {
    var plugin : UnsafePointer<DB_dsp_t>
    var _displayName : String
    var _data : ScriptableData?
    func data() -> ScriptableData? {
        return _data;
    }

    func getScript() -> String? {
        let data = Data(bytes: plugin.pointee.configdialog, count: Int(strlen(plugin.pointee.configdialog)))
        return String(data: data, encoding: String.Encoding.utf8)
    }

    init (plugin:UnsafePointer<DB_dsp_t>) {
        self.plugin = plugin
        let data = Data(bytes: plugin.pointee.plugin.name, count: Int(strlen(plugin.pointee.plugin.name)))
        _displayName = String(data: data, encoding: String.Encoding.utf8)!
    }

    @objc static func create (type: String) -> Scriptable {
        if let p = plug_get_dsp_for_id(type) {
            return DSPNode (plugin: p)
        }
        return DummyNode.create(type:type)
    }

    @objc func displayName() -> String {
        return _displayName
    }


    func loadItems(data: NSDictionary) -> [Scriptable]? {
        var items : [Scriptable]?;
        if let itemsList = data.value(forKey:"items") as? [NSDictionary] {
            items = []
            for i in itemsList {
                let it = ScriptableKeyValue.create(type:"")
                it.load(data:i);
                items!.append(it)
            }
        }
        return items
    }

    @objc func load (data: NSDictionary) {
        _data = ScriptableData(type: data.value(forKey: "type") as! String, name: data.value(forKey: "name") as? String, value:nil, items: loadItems(data:data))
    }

}

@objc(DummyNode)
class DummyNode : NSObject, Scriptable {
    var type: String
    init (type: String) {
        self.type = type
    }
    func getScript() -> String? {
        return nil
    }
    static func create (type: String) -> Scriptable
    {
        return DummyNode (type:type);
    }

    @objc func displayName() -> String {
        return type
    }

    @objc func load (data: NSDictionary) {
    }
}

// preset manager

protocol PresetManagerDelegate {
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

@objc
class PresetManager : NSObject {
    var data : [[String:Any]]
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
    }

    func load() throws {
        try serializer.load()
        selectedPreset = Int(conf_get_int("\(domain).\(context)", 0))
        selectedPreset += 1;

        if (selectedPreset < 0) {
            selectedPreset = 0;
        }
        else if (selectedPreset >= data.count) {
            selectedPreset = data.count-1;
        }
    }

    func save() throws {
        try serializer.save()
    }

    @objc func presetCount() -> Int {
        return data.count - 1;
    }

    @objc func presetItemCount(presetIndex:Int) -> Int {
        let preset = data[presetIndex];
        guard let items = preset["items"] as? [[String:Any]] else {
            return 0
        }
        return items.count
    }

    @objc func savePreset(index : Int) -> String {
        let preset = data[index];
        guard let data = try? JSONSerialization.data(withJSONObject: preset, options: []) else {
            return ""
        }
        let str = String(data: data, encoding: String.Encoding.utf8)
        return str!
//        return "{\"type\":\"m2s\", \"items\":[{ \"name\":0, \"value\": 1 },{ \"name\":0, \"value\": 1 }]}";
    }

    @objc func savePreset(index : Int, itemIndex : Int) -> String {
        let preset = data[index];
        guard let items = preset["items"] as? [[String:Any]] else {
            return ""
        }
        guard let data = try? JSONSerialization.data(withJSONObject: items[itemIndex], options: []) else {
            return ""
        }
        let str = String(data: data, encoding: String.Encoding.utf8)
        return str!
    }
    @objc func loadPreset(index : Int, fromString : String) {
        // ...
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

    /*
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

        let accessor = PluginConfigurationValueAccessorPreset.init(presetManager: self, presetIndex: Int32(presetIndex), subItemIndex: Int32(subItemIndex))
        
        viewController.initPluginConfiguration(plug!.pointee.configdialog, accessor: accessor)

        self.parentWindow = parentWindow
        self.sheet = sheet
        NSApp.beginSheet(sheet, modalFor: parentWindow, modalDelegate: self, didEnd: #selector(didEndDspConfigPanel(sheet:returnCode:contextInfo:)), contextInfo: nil)
        
    }*/

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

