import Cocoa

class HelpWindowController: NSWindowController {

    @IBOutlet var textView: NSTextView!
    override func windowDidLoad() {
        super.windowDidLoad()

        let path = Bundle.main.url(forResource: "help-cocoa", withExtension: "txt")
        var content = "";
        do {
            content = try String(contentsOf: path!, encoding: .utf8)
        }
        catch {
        }
        
        textView.textStorage?.setAttributedString(NSAttributedString(string: content));
        textView.setSelectedRange(NSMakeRange(0, 0));
    }

}
