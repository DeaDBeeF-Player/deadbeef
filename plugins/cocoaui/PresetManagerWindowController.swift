import Cocoa

class PresetManagerWindowController: NSWindowController, NSTableViewDelegate, NSTableViewDataSource {

    var presetMgr : PresetManager!

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

    func tableView(_ tableView: NSTableView, objectValueFor tableColumn: NSTableColumn?, row: Int) -> Any? {
        var offsettedRow = row
        if (presetMgr.hasCurrent()) {
            offsettedRow += 1
        }
        return presetMgr.getItems()[offsettedRow].displayName()

    }
}
