import Foundation
import Cocoa

@objc(Scriptable)
protocol Scriptable {
    // create a scriptable with specified type
    // e.g. PresetManager.create ("DSP") would create a DSP Preset Manager
    @objc static func create (_ type: String, parent: Scriptable?) -> Scriptable?

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
    @objc func loadFromJsonString (_ jsonString: String)
    @objc func saveToJsonString () -> String

    @objc func loadFromDictionary (_ data: [String:Any]?)
    @objc func saveToDictionary () -> [String:Any]

    // native file IO
    @objc func load ()
    @objc func save ()

    // add/remove items
    @objc func addItem (type: String) -> Scriptable?
    @objc func removeItem (index: Int)

    // parent access
    @objc func getParent() -> Scriptable?

    // data access
    @objc func getEnabled() -> Bool
    @objc func setEnabled(_ enabled:Bool)
    @objc func getName() -> String?
    @objc func setName(_ name:String?)
    @objc func getValue() -> String?
    @objc func setValue(_ value:String?)
    @objc func getType() -> String
    @objc func getItems() -> [Scriptable]
}

@objc
class ScriptableBase : NSObject { // implementation of some Scriptable methods, for all subclasses
    var parent : Scriptable?
    var enabled : Bool
    var type : String
    var name : String?
    var value : String?
    var items : [Scriptable]

    init(_ type: String, parent: Scriptable?) {
        self.parent = parent
        self.enabled = true
        self.type = type
        self.items = []
    }

    @objc func getItemTypes () -> [String] {
        return []
    }

    @objc func getItemName (type:String) -> String {
        return type
    }

    @objc func getItems() -> [Scriptable] {
        return self.items
    }

    @objc func getParent() -> Scriptable? {
        return parent
    }

    @objc func getEnabled() -> Bool {
        return enabled
    }
    @objc func setEnabled(_ enabled:Bool) {
        self.enabled = enabled
    }
    @objc func getName() -> String? {
        return self.name
    }
    @objc func setName(_ name:String?) {
        self.name = name
    }
    @objc func getValue() -> String? {
        return self.value
    }
    @objc func setValue(_ value:String?) {
        self.value = value
    }
    @objc func getType() -> String {
        return self.type
    }

    @objc func getItemClass () -> AnyClass? {
        return nil
    }

    @objc func loadFromJsonString (_ jsonString: String) {
    }

    @objc func saveToJsonString () -> String {
        return "stub"
    }

    @objc func loadFromDictionary (_ data: [String:Any]?) {
        guard let d = data else {
            return
        }

        // enabled
        self.enabled = d["enabled"] as? Bool ?? true

        // name
        if let n = d["name"] as? String {
            self.name = n;
        }
        else if let n = d["name"] as? Int {
            self.name = String(n);
        }

        // value
        if let v = d["value"] as? String {
            self.value = v;
        }
        else if let v = d["value"] as? Int {
            self.value = String(v);
        }
        else if let v = d["value"] as? Float {
            self.value = String(v);
        }

        // items
        if let items = d["items"] as? [[String:Any]] {
            for item in items {
                var type = "null";
                if let t = item["type"] as? String{
                    type = t
                }
                let it = getItemClass()?.create (type, parent: self as! Scriptable) ?? MissingNode.create (type, parent: self as! Scriptable)!
                it.loadFromDictionary (item);
                self.items.append(it);
            }
        }
    }

    @objc func saveToDictionary () -> [String:Any] {
        var items : [[String:Any]] = []
        var ret : [String:Any] =
        [
            "type":self.type,
        ]

        ret["enabled"] = self.enabled
        if let n = self.name {
            ret["name"] = n
        }
        if let v = self.value {
            ret["value"] = v
        }

        for item in self.items {
            let i = item.saveToDictionary ()
            items.append(i)
        }
        if (items.count > 0) {
            ret["items"] = items
        }

        return ret;
    }

    @objc func load () {
    }

    @objc func save () {
    }

    @objc func addItem (type: String) -> Scriptable? {
        guard let c = getItemClass() as? Scriptable.Type else {
            return nil
        }

        let parent = self as! Scriptable
        let item = c.create (type, parent:parent) ?? MissingNode.create (type, parent:parent)!
        self.items.append (item)
        return item
    }

    @objc func removeItem (index: Int) {
        self.items.remove(at: index)
    }
}

@objc(ScriptableKeyValue)
class ScriptableKeyValue : ScriptableBase, Scriptable {
    @objc static func create (_ type: String, parent:Scriptable?) -> Scriptable? {
        return ScriptableKeyValue(type, parent:parent)
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
    @objc var isCurrent : Bool
    @objc var savePath : String

    override func getItemClass() -> AnyClass? {
        return DSPNode.self
    }

    func getScript() -> String? {
        return "property \"DSP Nodes\" itemlist<DSPNode> items 0;" // display only the list of items
    }

    override init (_ type : String, parent: Scriptable?) {
        isCurrent = false
        savePath = ""
        super.init (type, parent:parent)
    }

    static func create (_ type: String, parent: Scriptable?) -> Scriptable? {
        return DSPPreset(type, parent:parent)
    }

    @objc func displayName() -> String {
        if let n = self.name {
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

    override func save () {
        util_dsp_preset_save (self)
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

    init (type:String, parent: Scriptable?, plugin:UnsafePointer<DB_dsp_t>) {
        self.plugin = plugin
        let data = Data(bytes: plugin.pointee.plugin.name, count: Int(strlen(plugin.pointee.plugin.name)))
        _displayName = String(data: data, encoding: String.Encoding.utf8)!
        super.init(type, parent:parent)
    }

    @objc static func create (_ type: String, parent:Scriptable?) -> Scriptable? {
        if let p = plug_get_dsp_for_id(type) {
            return DSPNode (type:type, parent:parent, plugin: p)
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
    static func create (_ type: String, parent: Scriptable?) -> Scriptable?
    {
        return MissingNode (type, parent:parent);
    }

    @objc func displayName() -> String {
        return "Missing <\(self.type)>"
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
    static func create(_ type: String, parent: Scriptable?) -> Scriptable? {
        // FIXME: delegate
        return PresetManager.init (domain:type, parent:parent, context:"context", delegate:nil, serializer:PresetSerializerJSON())
    }

    func getScript() -> String? {
        return "property \"\(domain) Items\" itemlist<\(itemType)> items 0;" // display only the list of items
    }

    func displayName() -> String {
        return "\(domain) Preset Manager"
    }

    // preset domain is the whole system name, e.g. "dsp" or "encoder"
    // context is a specific user of the domain, e.g. "player" or "converter"
    convenience init (domain:String, parent: Scriptable?, context:String, delegate:PresetManagerDelegate?) {
        self.init (domain:domain, parent:parent, context:context, delegate:delegate, serializer:PresetSerializerJSON())
    }

    init (domain:String, parent: Scriptable?, context:String, delegate:PresetManagerDelegate?, serializer:PresetSerializer) {
        self.itemType = domain+"Node";
        self.domain = domain
        self.context = context
        self.delegate = delegate
        self.serializer = serializer
        super.init(domain, parent:parent)
    }

    override func load() {
        do {
            try serializer.load()
        }
        catch _ {
        }
        selectedPreset = Int(conf_get_int("\(domain).\(context)", 0))
        selectedPreset += 1;

        if (selectedPreset < 0) {
            selectedPreset = 0;
        }
        else if (selectedPreset >= self.items.count) {
            selectedPreset = self.items.count-1;
        }
    }

    override func save() {
        do {
            try serializer.save()
        }
        catch _ {
        }
        selectedPreset = Int(conf_get_int("\(domain).\(context)", 0))
        selectedPreset += 1;

        if (selectedPreset < 0) {
            selectedPreset = 0;
        }
        else if (selectedPreset >= self.items.count) {
            selectedPreset = self.items.count-1;
        }
    }
}

