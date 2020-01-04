'use strict';

class RichToolbarController {
  constructor() {
    const fonts = ['Arimo', 'Caveat', 'Cousine', 'Tinos'];
    const sizes = [7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 28, 32, 36];
    this.defaultFont = 'Arimo';
    this.defaultSize = 13;

    let quillSize = Quill.import('attributors/style/size');
    this.sizeWhitelist = quillSize.whitelist = [];
    Quill.register(quillSize, true);

    let quillFont = Quill.import('attributors/style/font');
    this.fontWhitelist = quillFont.whitelist = ['Arimo', 'Cousine', 'Tinos'];
    Quill.register(quillFont, true);

    this.quill = null;

    let loadDOM = (name, id, def, list, cb) => {
      console.log(`init ${name}`);
      this[name] = 
        new DropdownMenu(false, document.getElementById(id));
      this[name].setText(def);
      let container = new MenuContainer(this[name]);
      console.log(`list has ${list.length} items`);
      for (let i = 0; i < list.length; i++) {
        let item = list[i];
        container.addMenuItem(
          new MenuItem(item, container, null, () => { cb(item); }, null));
      }
      this[name].setContainer(container);
    };
    loadDOM('fontDropdown', 'tb-dropdown-font', this.defaultFont, fonts,
            (newFont) => { this.setFont(newFont); });
    loadDOM('sizeDropdown', 'tb-dropdown-size', this.defaultSize, sizes,
            (newSize) => { this.setSize(newSize); });

    // set up bold/italic buttons
    this.boldButtonHelper =
      new ButtonMenuHelper(null, 'tb-format-bold', () => {
        this.toggleBoldItalic('bold');
      });
    this.boldButtonHelper.setEnabled(true);
    this.italicButtonHelper =
      new ButtonMenuHelper(null, 'tb-format-italic', () => {
        this.toggleBoldItalic('italic');
      });
    this.italicButtonHelper.setEnabled(true);
  }
  startedEditing(quill) {
    this.quill = quill;
    this.quill.on('editor-change', (eventName, ...args) => {
      if (eventName === 'selection-change')
        this.updateToolbar();
    });
    this.updateToolbar();
  }
  stoppedEditing() {
    this.quill = null;
  }
  toggleBoldItalic(property) {
    const format = this.quill.getFormat();
    const set = format.hasOwnProperty(property) && format[property];
    const selection = this.quill.getSelection();
    this.quill.formatText(selection.index, selection.length,
                          property, !set, 'api');
    this.updateToolbar();
  }
  updateToolbar() {
    if (!this.quill)
      return;
    const format = this.quill.getFormat();
    this.boldButtonHelper.setSelected(
      format.hasOwnProperty('bold') && format.bold);
    this.italicButtonHelper.setSelected(
      format.hasOwnProperty('italic') && format.italic);
    if (format.hasOwnProperty('font') && typeof format.font === 'string')
      this.fontDropdown.setText(format.font);
    else
      this.fontDropdown.setText(this.defaultFont);
    if (format.hasOwnProperty('size') && typeof format.size === 'string')
      this.sizeDropdown.setText(
        format.size.substring(0, format.size.lastIndexOf('px')));
    else
      this.sizeDropdown.setText(this.defaultSize);
  }
  setFont(font) {  // from user
    if (this.fontWhitelist.indexOf(font) < 0)
      this.fontWhitelist.push(font);
    // this.fontDropdown.setText(font);
    // console.log(`set font to ${font}`);
    if (!this.quill)
      return;
    const selection = this.quill.getSelection();
    this.quill.formatText(selection.index, selection.length,
                          'font', font, 'api');
    this.updateToolbar();
  }
  setSize(size) {  // from user
    let px = size + 'px';
    if (this.sizeWhitelist.indexOf(px) < 0)
      this.sizeWhitelist.push(px);
    // this.sizeDropdown.setText(size);
    if (!this.quill)
      return;
    const selection = this.quill.getSelection();
    this.quill.formatText(selection.index, selection.length,
                          'size', px);
    this.updateToolbar();
  }
}
