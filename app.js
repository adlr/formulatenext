'use strict';

var throttle = function(type, name, obj) {
  obj = obj || window;
  var running = false;
  var func = function() {
    if (running) { return; }
    running = true;
    requestAnimationFrame(function() {
      obj.dispatchEvent(new CustomEvent(name));
      running = false;
    });
  };
  obj.addEventListener(type, func);
};

let runtime_ready = false;

let SetZoom = null;
let SetScaleAndSize = null;
let SetScrollOrigin = null;
let Init = null;
let SetBackgroundColors = null;
let SetFileSize = null;
let AppendFileBytes = null;
let FinishFileLoad = null;
let AppendPDF = null;
let MouseEvent = null;
let DownloadFile = null;
let UndoRedoClicked = null;
let ToolbarClicked = null;
let UpdateEditText = null;
let InsertSignature = null;

const ID_MAIN = 0;
const ID_THUMB = 1;

class ButtonMenuHelper {
  constructor(path, buttonid, clicked) {
    this.button = null;
    if (buttonid) {
      this.button = document.getElementById(buttonid);
      this.button.onclick = (ev) => { this.clicked(ev); };
    }
    this.menuItem = null;
    if (path) {
      this.menuItem = globalMenuBar.findMenu(path);
      this.menuItem.setCallback(this.button.onclick);
    }
    this.enabled = false;
    this.callback = clicked;
  }
  clicked(ev) {
    if (!this.enabled)
      return;
    this.callback();
  }
  setEnabled(enabled) {
    if (this.enabled == enabled)
      return;
    this.enabled = enabled;
    if (this.menuItem)
      this.menuItem.setEnabled(enabled);
    if (this.button) {
      if (enabled)
        this.button.classList.add('toolbar-button-enabled');
      else
        this.button.classList.remove('toolbar-button-enabled');
    }
  }
}

class SelectButtonGroup {
  constructor(buttons, callback) {
    this.buttons = buttons;
    this.enabled = false;
    this.selected = 0;
    this.callback = callback;
    for (let i = 0; i < this.buttons.length; i++) {
      this.buttons[i].addEventListener('click',
                                       (ev) => { this.setSelected(i); });
    }
    this.fixupCSS();
  }
  setSelected(index) {
    if (this.selected == index)
      return;
    this.selected = index;
    if (!this.enabled)
      return;
    this.fixupCSS();
    this.callback(index);
  }
  setEnabled(en) {
    if (this.enabled == en)
      return;
    this.enabled = en;
    this.fixupCSS();
  }
  fixupCSS() {
    for (let i = 0; i < this.buttons.length; i++) {
      if (this.selected != i && this.enabled)
        this.buttons[i].classList.add('toolbar-button-enabled');
      else
        this.buttons[i].classList.remove('toolbar-button-enabled');
      if (this.selected == i && this.enabled)
        this.buttons[i].classList.add('toolbar-selected');
      else
        this.buttons[i].classList.remove('toolbar-selected');
    }
  }
}

class SignatureManager {
  constructor(insertMenuPath, deleteMenuPath, importMenuPath) {
    let insertMenuItem = globalMenuBar.findMenu(insertMenuPath);
    this.insertMenu = new MenuContainer(insertMenuItem);
    insertMenuItem.setContainer(this.insertMenu);
    let deleteMenuItem = globalMenuBar.findMenu(deleteMenuPath);
    this.deleteMenu = new MenuContainer(deleteMenuItem);
    deleteMenuItem.setContainer(this.deleteMenu);
    this.ss = new SignatureStore();
    // build up menus
    for (let i = 0; i < this.ss.length(); i++) {
      this.insertMenuItems(i);
    }
    globalMenuBar.findMenu(importMenuPath).setCallback(() => {
      let subWindow = window.open('sig.html');
      subWindow.gAddSig = (path, width, height) => {
        this.addSig(path, width, height);
        subWindow.close();
      };
    });
  }
  insertMenuItems(index) {
    let svg = this.ss.getAsSVG(index);
    let sig = this.ss.getSignature(index);
    let inItem = new MenuItem(null, this.insertMenu, null, null, svg);
    inItem.setCallback(() => {
      InsertSignature(sig[0]);
    });
    this.insertMenu.addMenuItem(inItem);
    let delCallback = () => {
    };
    let delItem = new MenuItem(null, this.deleteMenu, null, delCallback, svg);
    delItem.setCallback(() => {
      this.insertMenu.delMenuItem(inItem);
      this.deleteMenu.delMenuItem(delItem);
      this.ss.deleteSignature(sig[0], sig[1], sig[2]);
    });
    this.deleteMenu.addMenuItem(delItem);
  }
  // Called when user has created a new signature:
  addSig(path, width, height) {
    this.ss.addSignature(path, width, height);
    this.insertMenuItems(this.ss.length() - 1);
  }
}

let bridge_undoRedoEnable = null;
let bridge_setToolboxState = null;
let bridge_startComposingText = null;
let bridge_stopComposingText = null;

document.addEventListener('DOMContentLoaded', function() {
  initGlobalMenuBar();

  let fixupContentSize = null;
  let fixupThumbnailSize = null;
  Module['onRuntimeInitialized'] = function() {
    // TestDraw = Module.cwrap('TestDraw', 'number',
    //                         ['number',  // width
    //                          'number']);  // height
    SetZoom = Module.cwrap('SetZoom', null, ['number']);
    SetScaleAndSize = Module.cwrap('SetScaleAndSize', null,
                                   ['number', 'number', 'number', 'number']);
    SetScrollOrigin = Module.cwrap('SetScrollOrigin', null,
                                   ['number', 'number', 'number']);
    Init = Module.cwrap('Init', null, []);
    SetBackgroundColors = Module.cwrap('SetBackgroundColors', null,
                                       ['number', 'number']);
    SetFileSize = Module.cwrap('SetFileSize', null, ['number']);
    AppendFileBytes = Module.cwrap('AppendFileBytes', null, ['number', 'number']);
    FinishFileLoad = Module.cwrap('FinishFileLoad', null, []);
    AppendPDF = Module.cwrap('AppendPDF', null, ['number', 'number']);
    MouseEvent = Module.cwrap('MouseEvent', 'number',
                              ['number', 'number', 'number', 'number']);
    DownloadFile = Module.cwrap('DownloadFile', null, []);
    UndoRedoClicked = Module.cwrap('UndoRedoClicked', null, ['number']);
    ToolbarClicked = Module.cwrap('ToolbarClicked', null, ['number']);
    UpdateEditText = Module.cwrap('UpdateEditText', null, ['string']);
    InsertSignature = Module.cwrap('InsertSignature', null, ['string']);

    runtime_ready = true;
    console.log("runtime is ready");
    Init();
    fixupContentSize();
    fixupThumbnailSize();
    SetBackgroundColors(0xffeaf8ff, 0xffb0b0b0);

    let zoom_level = 1.0;
    let zoomInButtonMenuHelper =
        new ButtonMenuHelper(null, 'zoom-in',
                             () => {
                               zoom_level *= 1.1;
                               SetZoom(zoom_level);
                             });
    let zoomOutButtonMenuHelper =
        new ButtonMenuHelper(null, 'zoom-out',
                             () => {
                               zoom_level /= 1.1;
                               SetZoom(zoom_level);
                             });
    zoomInButtonMenuHelper.setEnabled(true);
    zoomOutButtonMenuHelper.setEnabled(true);

    let undoButtonMenuHelper =
        new ButtonMenuHelper(['Edit', 'Undo'], 'undo',
                             () => { UndoRedoClicked(true); });
    let redoButtonMenuHelper =
        new ButtonMenuHelper(['Edit', 'Redo'], 'redo',
                             () => { UndoRedoClicked(false); });
    bridge_undoRedoEnable = (undoEnabled, redoEnabled) => {
      undoButtonMenuHelper.setEnabled(undoEnabled);
      redoButtonMenuHelper.setEnabled(redoEnabled);
    };

    let toolbox = new SelectButtonGroup([
      document.getElementById('tb-tool-arrow'),
      document.getElementById('tb-tool-text'),
      document.getElementById('tb-tool-freehand')
    ], (index) => {
      if (ToolbarClicked)
        ToolbarClicked(index);
    });
    bridge_setToolboxState = (enabled, tool) => {
      toolbox.setSelected(tool);
      toolbox.setEnabled(enabled);
    }

    let signatureManager =
        new SignatureManager(['Signature', 'Insert Signature'],
                             ['Signature', 'Delete Signature'],
                             ['Signature', 'Import Signature...']);
  };

  var outer = document.getElementById('main-scroll-outer');
  var canvas = document.getElementById('main-canvas');
  let initScrollHandler = (div, id) => {
    throttle('scroll', 'optimizedScroll', div);
    div.addEventListener('optimizedScroll', function() {
      if (!runtime_ready) return;
      SetScrollOrigin(id, div.scrollLeft, div.scrollTop);
    });
  };
  initScrollHandler(outer, ID_MAIN);
  initScrollHandler(document.getElementById('thumb-scroll-outer'),
                    ID_THUMB);

  throttle('resize', 'optimizedResize');
  fixupContentSize = function() {
    if (!runtime_ready) return;
    var dpr = window.devicePixelRatio || 1;
    var rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    SetScaleAndSize(ID_MAIN, dpr, rect.width, rect.height);
  };
  window.addEventListener('optimizedResize', fixupContentSize);  

  let thumbCanvas = document.getElementById('thumb-canvas');
  fixupThumbnailSize = () => {
    if (!runtime_ready) return;
    console.log('fixup thumbnail');
    const dpr = window.devicePixelRatio || 1;
    const rect = thumbCanvas.getBoundingClientRect();
    thumbCanvas.width = rect.width * dpr;
    thumbCanvas.height = rect.height * dpr;
    SetScaleAndSize(ID_THUMB, dpr, rect.width, rect.height);
  };

  // setup thumbnail resize
  let thumbResize = document.getElementById('thumb-resize');
  let flexMain = document.getElementById('flex-main');
  let thumbResizePosition = -1;
  let thumbWidth = getComputedStyle(flexMain).getPropertyValue(
    '--thumb-width').match(/\d+/)[0] | 0;
  let thumbMouseDrag = (ev) => {
    const delta = ev.clientX - thumbResizePosition;
    thumbResizePosition = ev.clientX;
    thumbWidth += delta;
    let useValue = Math.min(800, Math.max(50, thumbWidth));
    flexMain.style.setProperty('--thumb-width', useValue + 'px');
    fixupContentSize();
    fixupThumbnailSize();
  };
  let thumbMouseUp = (ev) => {
    document.removeEventListener('pointermove', thumbMouseDrag);
    document.removeEventListener('pointerup', thumbMouseUp);
  }
  thumbResize.addEventListener('pointerdown', (ev) => {
    ev.preventDefault();
    thumbResizePosition = ev.clientX;
    document.addEventListener('pointermove', thumbMouseDrag);
    document.addEventListener('pointerup', thumbMouseUp);
  });

  let calcFontMetrics = (fontsize) => {
    let div = document.createElement('div');
    div.style.position = 'absolute';
    div.style.top = '-10000px';
    div.style.left = '-10000px';
    div.style.fontFamily = 'Arial';
    div.style.fontSize = '' + fontsize + 'px';
    let span = document.createElement('span');
    span.style.fontSize = '0%';
    span.innerText = 'x';
    let spanbig = document.createElement('span');
    spanbig.innerText = 'x';
    div.appendChild(span);
    div.appendChild(spanbig);
    document.body.appendChild(div);
    let ret = span.offsetTop;
    console.log([span.offsetTop, span.offsetHeight,
                 spanbig.offsetTop, spanbig.offsetHeight].join(', '));
    document.body.removeChild(div);
    return ret;
  };

  let launchEditor = (xpos, ypos, zoom, text, caretPos) => {
    const dpr = window.devicePixelRatio || 1;
    xpos /= dpr;
    ypos /= dpr;
    console.log(`Edit at ${xpos} ${ypos}, ${zoom}, ${text}, ${caretPos}`);
    let vertOffset = calcFontMetrics((zoom * 12) | 0);
    let padding = 5;
    xpos -= padding;
    ypos -= padding + vertOffset;
    let textarea = document.createElement('textarea');
    textarea.rows = '1';
    textarea.cols = '1';
    textarea.classList.add('texteditor');

    textarea.style.left = (outer.offsetLeft + xpos) + 'px';
    textarea.style.top = (outer.offsetTop + ypos) + 'px';
    textarea.style.transform = 'scale(' + zoom + ')';
    let update = () => {
      textarea.style.height = '';
      textarea.style.height = Math.max(10, textarea.scrollHeight) + 'px';
      textarea.style.width = '';
      textarea.style.width = Math.max(10, textarea.scrollWidth + padding) + 'px';
      UpdateEditText(textarea.value);
    };
    textarea.addEventListener('keyup', update);
    textarea.addEventListener('input', update);
    textarea.addEventListener('blur', (ev) => {
      //textarea.parentNode.removeChild(textarea);
    });
    textarea.value = text;
    update();
    document.getElementById('main-view').appendChild(textarea);
    setTimeout(() => {
      textarea.focus();
      update();
      textarea.setSelectionRange(caretPos, caretPos);
      console.log("set caret here: " + caretPos);
    }, 100);
    bridge_stopComposingText = () => {
      textarea.parentNode.removeChild(textarea);
    }
  };
  bridge_startComposingText = launchEditor;

  const kEventKindDown = 0;
  const kEventKindDrag = 1;
  const kEventKindUp = 2;
  const kEventKindMove = 3;
  const kEventKindClick = 4;
  let pushMouseEvent = (ev, div, kind, id) => {
    if (MouseEvent === null)
      return;
    const kControlKey = 1;
    const kAltKey = 2;
    const kShiftKey = 4;

    const dpr = window.devicePixelRatio || 1;
    const xpos = (ev.offsetX - div.scrollLeft) * dpr;
    const ypos = (ev.offsetY - div.scrollTop) * dpr;
    const keys = (ev.ctrlKey ? kControlKey : 0) |
          (ev.altKey ? kAltKey : 0) |
          (ev.shiftKey ? kShiftKey : 0);
    return MouseEvent(id, xpos, ypos, kind, keys);
  };

  let setupMouseHandlers = (div, id) => {
    let dragInProgress = false;
    div.addEventListener('pointerdown', ev => {
      dragInProgress = pushMouseEvent(ev, div, kEventKindDown, id);
      if (dragInProgress)
        ev.preventDefault();
    });
    div.addEventListener('pointermove', ev => {
      if (dragInProgress) {
        pushMouseEvent(ev, div, kEventKindDrag, id);
        ev.preventDefault();
      } else {
        pushMouseEvent(ev, div, kEventKindMove, id);
      }
    });
    div.addEventListener('pointerup', ev => {
      if (dragInProgress) {
        pushMouseEvent(ev, div, kEventKindUp, id);
        dragInProgress = false;
        ev.preventDefault();
      }
    });
    div.addEventListener('pointercancel', ev => {
      if (dragInProgress) {
        pushMouseEvent(ev, div, kEventKindUp, id);
        dragInProgress = false;
        ev.preventDefault();
      }
    });
    div.addEventListener('click', ev => {
      console.log('click');
      pushMouseEvent(ev, div, kEventKindClick, id);
    });
  };
  setupMouseHandlers(outer, ID_MAIN);
  setupMouseHandlers(document.getElementById('thumb-scroll-outer'), ID_THUMB);

  // if |isOpen| is true, it's for file open, otherwise for append
  let setupFileChooserMenu = (path, isOpen) => {
    globalMenuBar.findMenu(path).setCallback((ev) => {
      document.getElementById('file-input').onchange = (elt) => {
        let file = elt.target.files[0];
        if (!file)
          return;
        if (isOpen)
          SetFileSize(file.size);
        let reader = new FileReader();
        reader.onload = (el) => {
          console.log('got a file ' + isOpen);
          let data = new Uint8Array(reader.result);
          let buf = Module._malloc(data.length);
          Module.HEAPU8.set(data, buf);
          if (isOpen)
            AppendFileBytes(buf, data.length);
          else
            AppendPDF(buf, data.length);
          Module._free(buf);
          if (isOpen)
            FinishFileLoad();
        };
        reader.readAsArrayBuffer(file);
      };
      let ev2 = document.createEvent('MouseEvents');
      ev2.initEvent('click', true, true);
      document.getElementById('file-input').dispatchEvent(ev2);
    });
  };
  setupFileChooserMenu(['File', 'Open...'], true);
  setupFileChooserMenu(['File', 'Append PDF...'], false);
}, false);

let PushCanvasXYWH = (id, bufptr, xpos, ypos, width, height) => {
  if (id != ID_MAIN && id != ID_THUMB) {
    console.log("Invalid ID!\n");
    return;
  }
  var canvas = document.getElementById(id == ID_MAIN ?
                                       'main-canvas' : 'thumb-canvas');
  var ctx = canvas.getContext('2d');
  let arr = new Uint8ClampedArray(Module.HEAPU8.buffer,
                                  bufptr, width * height * 4);
  let img = new ImageData(arr, width, height);
  ctx.putImageData(img, xpos, ypos);
};

let bridge_drawBezier =
    (startx, starty, ctrl1x, ctrl1y, ctrl2x, ctrl2y, stopx, stopy, width) => {
      let canvas = document.getElementById('main-canvas');
      let ctx = canvas.getContext('2d');
      ctx.beginPath();
      ctx.lineWidth = width;
      ctx.moveTo(startx, starty);
      ctx.bezierCurveTo(ctrl1x, ctrl1y, ctrl2x, ctrl2y, stopx, stopy);
      ctx.stroke();
    };

// set the size/position of the scrollbar view
let bridge_setSize = function(id, width, height, xpos, ypos) {
  if (id != ID_MAIN && id != ID_THUMB) {
    console.log("Invalid ID!\n");
    return;
  }
  let inner = document.getElementById(
    id == ID_MAIN ? 'main-scroll-inner' : 'thumb-scroll-inner');
  inner.style.width = inner.style.minWidth = width + 'px';
  inner.style.height = height + 'px';
  let outer = document.getElementById(
    id == ID_MAIN ? 'main-scroll-outer' : 'thumb-scroll-outer');
  outer.scrollLeft = xpos;
  outer.scrollTop = ypos;
};

let bridge_downloadBytes = (addr, len) => {
  const arr = new Uint8ClampedArray(Module.HEAPU8.buffer, addr, len);
  const blob = new Blob([arr], {type: 'application/pdf'});
  const data = window.URL.createObjectURL(blob);
  var link = document.createElement('a');
  link.href = data;
  link.download="file.pdf";
  link.click();
  setTimeout(() => { window.URL.revokeObjectURL(data); }, 100);
};
