import Foundation
import Cocoa

struct ScriptableData {
    var type : String
    var name : String?
    var value : String?
    var items : [Scriptable]

    init(_ type:String) {
        self.type = type
        items = []
    }
}

@objc(Scriptable)
protocol Scriptable {
    // create a scriptable with specified type
    // e.g. PresetManager.create ("DSP") would create a DSP Preset Manager
    @objc static func create (_ type: String) -> Scriptable?

    // enumeration / factory
    @objc func getItemTypes () -> [String]
    @objc func getItemName (type:String) -> String

     // all items are always of the same type
    @objc func getItemClass () -> AnyClass?

    // get a script needed to display the editor of this scriptable
    @objc func getScript() -> String?

    // get a display name of this scriptable (e.g. "DSP Preset Manager")
    @objc func displayName() -> String

    // serialization
    @objc func load (data: [String:Any]?)
    @objc func save () -> [String:Any]

    // add/remove items
    @objc func addItem (type: String) -> Scriptable?
    @objc func removeItem (index: Int)

    // data access
    @objc func getName() -> String?
    @objc func setName(_ name:String?)
    @objc func getValue() -> String?
    @objc func setValue(_ value:String?)
    @objc func getType() -> String
    @objc func getItems() -> [Scriptable]
}

@objc
class ScriptableBase : NSObject { // implementation of some Scriptable methods, for all subclasses
    var data : ScriptableData

    init(_ type: String) {
        data = ScriptableData(type)
    }

    @objc func getItemTypes () -> [String] {
        return []
    }

    @objc func getItemName (type:String) -> String {
        return type
    }

    @objc func getItems() -> [Scriptable] {
        return data.items
    }

    @objc func getName() -> String? {
        return data.name
    }
    @objc func setName(_ name:String?) {
        data.name = name
    }
    @objc func getValue() -> String? {
        return data.value
    }
    @objc func setValue(_ value:String?) {
        data.value = value
    }
    @objc func getType() -> String {
        return data.type
    }

    @objc func getItemClass () -> AnyClass? {
        return nil
    }

    @objc func load (data: [String:Any]?) {
        guard let d = data else {
            return
        }

        // name and value
        if let n = d["name"] as? String {
            self.data.name = n;
        }
        else if let n = d["name"] as? Int {
            self.data.name = String(n);
        }

        if let v = d["value"] as? String {
            self.data.value = v;
        }
        else if let v = d["value"] as? Int {
            self.data.value = String(v);
        }
        else if let v = d["value"] as? Float {
            self.data.value = String(v);
        }

        // items
        if let items = d["items"] as? [[String:Any]] {
            for item in items {
                var type = "null";
                if let t = item["type"] as? String{
                    type = t
                }
                let it = getItemClass()?.create (type) ?? MissingNode.create (type)!
                it.load (data:item);
                self.data.items.append(it);
            }
        }
    }

    @objc func save () -> [String:Any] {
        var items : [[String:Any]] = []
        var ret : [String:Any] =
        [
            "type":data.type,
        ]

        if let n = data.name {
            ret["name"] = n
        }
        if let v = data.value {
            ret["value"] = v
        }

        for item in data.items {
            let i = item.save ()
            items.append(i)
        }
        if (items.count > 0) {
            ret["items"] = items
        }

        return ret;
    }

    @objc func addItem (type: String) -> Scriptable? {
        guard let c = getItemClass() as? Scriptable.Type else {
            return nil
        }

        let item = c.create (type) ?? MissingNode.create (type)!
        data.items.append (item)
        return item
    }

    @objc func removeItem (index: Int) {
        data.items.remove(at: index)
    }
}


@objc(ScriptableKeyValue)
class ScriptableKeyValue : ScriptableBase, Scriptable {
    @objc static func create (_ type: String) -> Scriptable? {
        return ScriptableKeyValue(type)
    }

    func getScript() -> String? {
        return nil
    }

    @objc func displayName() -> String {
        return ""
    }

}

@objc(DSPPreset)
class DSPPreset : ScriptableBase, Scriptable {
    override func getItemClass() -> AnyClass? {
        return DSPNode.self
    }

    func getScript() -> String? {
        return "property \"DSP Nodes\" itemlist<DSPNode> items 0;" // display only the list of items
    }

    static func create (_ type: String) -> Scriptable? {
        return DSPPreset(type)
    }

    @objc func displayName() -> String {
        if let n = data.name {
            return n
        }
        return ""
    }

    override func getItemTypes () -> [String] {
        var list : [String] = []
        let plugins = plug_get_dsp_list ()

        var i : Int = 0;
        while let p = plugins?[i]?.pointee {
            let data = Data(bytes: p.plugin.id, count: Int(strlen(p.plugin.id)))
            list.append(String(data: data, encoding: String.Encoding.utf8)!)
            i += 1
        }

        return list;
    }

    override func getItemName (type: String) -> String {
        if let p = plug_get_for_id(type) {
            let data = Data(bytes: p.pointee.name, count: Int(strlen(p.pointee.name)))
            return String(data: data, encoding: String.Encoding.utf8)!
        }
        return "null";
    }
}

@objc(DSPNode)
class DSPNode : ScriptableBase, Scriptable {
    override func getItemClass() -> AnyClass? {
        return ScriptableKeyValue.self
    }

    var plugin : UnsafePointer<DB_dsp_t>
    var _displayName : String

    func getScript() -> String? {
        let data = Data(bytes: plugin.pointee.configdialog, count: Int(strlen(plugin.pointee.configdialog)))
        return String(data: data, encoding: String.Encoding.utf8)
    }

    init (type:String, plugin:UnsafePointer<DB_dsp_t>) {
        self.plugin = plugin
        let data = Data(bytes: plugin.pointee.plugin.name, count: Int(strlen(plugin.pointee.plugin.name)))
        _displayName = String(data: data, encoding: String.Encoding.utf8)!
        super.init(type)
    }

    @objc static func create (_ type: String) -> Scriptable? {
        if let p = plug_get_dsp_for_id(type) {
            return DSPNode (type:type, plugin: p)
        }
        return nil
    }

    @objc func displayName() -> String {
        return _displayName
    }
}

@objc(DummyNode)
class MissingNode : ScriptableBase, Scriptable {
    override func getItemClass() -> AnyClass? {
        return ScriptableKeyValue.self
    }

    func getScript() -> String? {
        return nil
    }
    static func create (_ type: String) -> Scriptable?
    {
        return MissingNode (type);
    }

    @objc func displayName() -> String {
        return "Missing <\(data.type)>"
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
class PresetManager : ScriptableBase, Scriptable {
    let itemType : String; // e.g. DSPNode

    var domain : String
    var context : String
    var delegate : PresetManagerDelegate?
    var serializer : PresetSerializer
    var parentWindow : NSWindow!
    var sheet : NSWindow!

    var selectedPreset : Int = -1


    override func getItemClass() -> AnyClass? {
        return NSClassFromString("\(itemType)Preset")
    }

    // Scriptable API
    static func create(_ type: String) -> Scriptable? {
        // FIXME: delegate
        return PresetManager.init (domain:type, context:"context", delegate:nil, serializer:PresetSerializerJSON())
    }

    func getScript() -> String? {
        return "property \"\(domain) Items\" itemlist<\(itemType)> items 0;" // display only the list of items
    }

    func displayName() -> String {
        return "\(domain) Preset Manager"
    }

    // preset domain is the whole system name, e.g. "dsp" or "encoder"
    // context is a specific user of the domain, e.g. "player" or "converter"
    convenience init (domain:String, context:String, delegate:PresetManagerDelegate?) {
        self.init (domain:domain, context:context, delegate:delegate, serializer:PresetSerializerJSON())
    }

    init (domain:String, context:String, delegate:PresetManagerDelegate?, serializer:PresetSerializer) {
        self.itemType = domain+"Node";
        self.domain = domain
        self.context = context
        self.delegate = delegate
        self.serializer = serializer
        super.init(domain)
    }

    func load() throws {
        try serializer.load()
        selectedPreset = Int(conf_get_int("\(domain).\(context)", 0))
        selectedPreset += 1;

        if (selectedPreset < 0) {
            selectedPreset = 0;
        }
        else if (selectedPreset >= data.items.count) {
            selectedPreset = data.items.count-1;
        }
    }
}

