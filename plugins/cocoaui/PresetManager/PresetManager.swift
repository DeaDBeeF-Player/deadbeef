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
    var _parent : Scriptable?
    var _enabled : Bool
    var _type : String
    var _name : String?
    var _value : String?
    var _items : [Scriptable]

    init(_ type: String, parent: Scriptable?) {
        self._parent = parent
        self._enabled = true
        self._type = type
        self._items = []
    }

    @objc func getItemTypes () -> [String] {
        return []
    }

    @objc func getItemName (type:String) -> String {
        return type
    }

    @objc func getItems() -> [Scriptable] {
        return self._items
    }

    @objc func getParent() -> Scriptable? {
        return _parent
    }

    @objc func getEnabled() -> Bool {
        return _enabled
    }
    @objc func setEnabled(_ enabled:Bool) {
        self._enabled = enabled
    }
    @objc func getName() -> String? {
        return self._name
    }
    @objc func setName(_ name:String?) {
        self._name = name
    }
    @objc func getValue() -> String? {
        return self._value
    }
    @objc func setValue(_ value:String?) {
        self._value = value
    }
    @objc func getType() -> String {
        return self._type
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
        self._enabled = d["enabled"] as? Bool ?? true

        // name
        if let n = d["name"] as? String {
            self._name = n
        }
        else if let n = d["name"] as? Int {
            self._name = String(n)
        }

        // value
        if let v = d["value"] as? String {
            self._value = v
        }
        else if let v = d["value"] as? Int {
            self._value = String(v)
        }
        else if let v = d["value"] as? Float {
            self._value = String(v)
        }

        // items
        if let items = d["items"] as? [[String:Any]] {
            for item in items {
                var type = "null"
                if let t = item["type"] as? String{
                    type = t
                }
                let it = getItemClass()?.create (type, parent: self as? Scriptable) ?? MissingNode.create (type, parent: self as? Scriptable)!
                it.loadFromDictionary (item)
                self._items.append(it)
            }
        }
    }

    @objc func saveToDictionary () -> [String:Any] {
        var items : [[String:Any]] = []
        var ret : [String:Any] =
        [
            "type":self._type,
        ]

        ret["enabled"] = self._enabled
        if let n = self._name {
            ret["name"] = n
        }
        if let v = self._value {
            ret["value"] = v
        }

        for item in self._items {
            let i = item.saveToDictionary ()
            items.append(i)
        }
        if (items.count > 0) {
            ret["items"] = items
        }

        return ret
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
        self._items.append (item)
        return item
    }

    @objc func removeItem (index: Int) {
        self._items.remove(at: index)
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
        if let n = self._name {
            return n
        }
        return ""
    }

    override func getItemTypes () -> [String] {
        var list : [String] = []
        let plugins = plug_get_dsp_list ()

        var i : Int = 0
        while let p = plugins?[i]?.pointee {
            let data = Data(bytes: p.plugin.id, count: Int(strlen(p.plugin.id)))
            list.append(String(data: data, encoding: String.Encoding.utf8)!)
            i += 1
        }

        return list
    }

    override func getItemName (type: String) -> String {
        if let p = plug_get_for_id(type) {
            let data = Data(bytes: p.pointee.name, count: Int(strlen(p.pointee.name)))
            return String(data: data, encoding: String.Encoding.utf8)!
        }
        return "null"
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
        return MissingNode (type, parent:parent)
    }

    @objc func displayName() -> String {
        return "Missing <\(self._type)>"
    }
}

// preset manager
protocol PresetManagerDelegate {
    // Return true if the item can be edited
    func presetManager(_ presetManager:PresetManager, isEditable index: Int) -> Bool

    // Return true if the item needs to be saved
    func presetManager(_ presetManager:PresetManager, isSaveable index: Int) -> Bool

    // Return true if this preset manager contains "current" item at index 0, which needs to be excluded in some cases
    func presetManagerHasCurrentPreset (_ presetManager:PresetManager) -> Bool
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
    let itemType : String // e.g. DSPNode

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

    func hasCurrent () -> Bool {
        return delegate?.presetManagerHasCurrentPreset(self) ?? false
    }

    // preset domain is the whole system name, e.g. "dsp" or "encoder"
    // context is a specific user of the domain, e.g. "player" or "converter"
    convenience init (domain:String, parent: Scriptable?, context:String, delegate:PresetManagerDelegate?) {
        self.init (domain:domain, parent:parent, context:context, delegate:delegate, serializer:PresetSerializerJSON())
    }

    init (domain:String, parent: Scriptable?, context:String, delegate:PresetManagerDelegate?, serializer:PresetSerializer) {
        self.itemType = domain+"Node"
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
        selectedPreset += 1

        if (selectedPreset < 0) {
            selectedPreset = 0
        }
        else if (selectedPreset >= self._items.count) {
            selectedPreset = self._items.count-1
        }
    }

    override func save() {
        do {
            try serializer.save()
        }
        catch _ {
        }
        selectedPreset = Int(conf_get_int("\(domain).\(context)", 0))
        selectedPreset += 1

        if (selectedPreset < 0) {
            selectedPreset = 0
        }
        else if (selectedPreset >= self._items.count) {
            selectedPreset = self._items.count-1
        }
    }
}

