import Cocoa

class PresetManagerWindowController: NSWindowController, NSTableViewDelegate, NSTableViewDataSource {

    var presetMgr : PresetManager!
    @IBOutlet weak var presetList: NSTableView!

    override func windowDidLoad() {
        super.windowDidLoad()
    }

    func setPresetMgr (_ presetMgr : PresetManager) {
        self.presetMgr = presetMgr
    }

    func numberOfRows(in tableView: NSTableView) -> Int {
        var count = presetMgr.getItems().count
        if (presetMgr.hasCurrent()) {
            count -= 1
        }
        return count
    }

    func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
        let result = tableView.makeView(withIdentifier: NSUserInterfaceItemIdentifier(rawValue: "Title"), owner: self)
        if let textView = result?.subviews[0] as? NSTextField {
            var offsettedRow = row
            if (presetMgr.hasCurrent()) {
                offsettedRow += 1
            }
            if (offsettedRow < presetMgr.getItems().count) {
                textView.stringValue = presetMgr.getItems()[offsettedRow].displayName()
            }
            return textView
        }
        return nil
    }

    func tableView(_ tableView: NSTableView, didAdd rowView: NSTableRowView, forRow row: Int) {
        _ = presetMgr.addItem(type: presetMgr.getItemTypes()[0])
        let view = tableView.view(atColumn: 0, row: row, makeIfNecessary: true)
        if (view?.acceptsFirstResponder ?? false) {
            view?.window?.makeFirstResponder(view)
        }
    }

    func tableView(_ tableView: NSTableView, didRemove rowView: NSTableRowView, forRow row: Int) {
        presetMgr.removeItem(index: row)
    }

    @IBAction func buttonBarAction(_ sender: NSSegmentedControl) {
        let seg = sender.selectedSegment;
        switch (seg) {
        case 0: // add
            presetList.beginUpdates()
            presetList.insertRows(at: [presetList.numberOfRows], withAnimation: .slideRight)
            presetList.endUpdates()
        case 1: // remove
            presetList.beginUpdates()
            presetList.removeRows(at: [presetList.selectedRow], withAnimation: .slideLeft)
            presetList.endUpdates()
        default:
            return
        }
    }
}
