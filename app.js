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
let SetFileSize = null;
let AppendFileBytes = null;
let FinishFileLoad = null;
let MouseEvent = null;
let DownloadFile = null;
let UndoRedoClicked = null;
let ToolbarClicked = null;
let UpdateEditText = null;

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

let bridge_undoRedoEnable = null;
let bridge_setToolboxState = null;
let bridge_startComposingText = null;
let bridge_stopComposingText = null;

document.addEventListener('DOMContentLoaded', function() {
  initGlobalMenuBar();

  let fixupContentSize = null;
  Module['onRuntimeInitialized'] = function() {
    runtime_ready = true;
    console.log("runtime is ready");
    Init();
    fixupContentSize();

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
  };
  // TestDraw = Module.cwrap('TestDraw', 'number',
  //                         ['number',  // width
  //                          'number']);  // height
  SetZoom = Module.cwrap('SetZoom', null, ['number']);
  SetScaleAndSize = Module.cwrap('SetScaleAndSize', null,
                                 ['number', 'number', 'number']);
  SetScrollOrigin = Module.cwrap('SetScrollOrigin', null,
                                 ['number', 'number']);
  Init = Module.cwrap('Init', null, []);
  SetFileSize = Module.cwrap('SetFileSize', null, ['number']);
  AppendFileBytes = Module.cwrap('AppendFileBytes', null, ['number', 'number']);
  FinishFileLoad = Module.cwrap('FinishFileLoad', null, []);
  MouseEvent = Module.cwrap('MouseEvent', 'number',
                            ['number', 'number', 'number', 'number']);
  DownloadFile = Module.cwrap('DownloadFile', null, []);
  UndoRedoClicked = Module.cwrap('UndoRedoClicked', null, ['number']);
  ToolbarClicked = Module.cwrap('ToolbarClicked', null, ['number']);
  UpdateEditText = Module.cwrap('UpdateEditText', null, ['string']);

  var outer = document.getElementById('main-scroll-outer');
  var canvas = document.getElementById('main-canvas');
  throttle('scroll', 'optimizedScroll', outer);
  outer.addEventListener('optimizedScroll', function() {
    if (!runtime_ready) return;
    SetScrollOrigin(outer.scrollLeft, outer.scrollTop);
  });

  throttle('resize', 'optimizedResize');
  fixupContentSize = function() {
    if (!runtime_ready) return;
    var dpr = window.devicePixelRatio || 1;
    var rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    SetScaleAndSize(dpr, rect.width, rect.height);
  };
  window.addEventListener('optimizedResize', fixupContentSize);
  
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

  let launchEditor = (xpos, ypos, zoom) => {
    const dpr = window.devicePixelRatio || 1;
    xpos /= dpr;
    ypos /= dpr;
    console.log(`Edit at ${xpos} ${ypos}`);
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
    textarea.value = '';
    update();
    document.body.appendChild(textarea);
    setTimeout(() => {
      textarea.focus();
      update();
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
  let pushMouseEvent = (ev, kind) => {
    if (MouseEvent === null)
      return;
    const kControlKey = 1;
    const kAltKey = 2;
    const kShiftKey = 4;

    const dpr = window.devicePixelRatio || 1;
    const xpos = (ev.offsetX - outer.scrollLeft) * dpr;
    const ypos = (ev.offsetY - outer.scrollTop) * dpr;
    const keys = (ev.ctrlKey ? kControlKey : 0) |
          (ev.altKey ? kAltKey : 0) |
          (ev.shiftKey ? kShiftKey : 0);
    return MouseEvent(xpos, ypos, kind, keys);
  };

  let dragInProgress = false;
  outer.addEventListener('mousedown', ev => {
    dragInProgress = pushMouseEvent(ev, kEventKindDown);
    console.log("drag ip: " + dragInProgress);
  });
  outer.addEventListener('mousemove', ev => {
    if (ev.buttons) {
      if (dragInProgress) {
        pushMouseEvent(ev, kEventKindDrag);
      }
    } else {
      pushMouseEvent(ev, kEventKindMove);
    }
  });
  outer.addEventListener('mouseup', ev => {
    console.log("mouse up drag ip: " + dragInProgress);
    if (dragInProgress) {
      pushMouseEvent(ev, kEventKindUp);
      dragInProgress = false;
    }
  });

  document.getElementById('file-input').addEventListener('change',
                                                         loadFile, false);
}, false);

var PushCanvas = (bufptr, width, height) => {
  var canvas = document.getElementById('main-canvas');
  if (canvas.width != width || canvas.height != height) {
    console.log(`Size mismatch! Canvas is (${canvas.width}, ${canvas.height}). Given (${width}, ${height})`);
  }
  var ctx = canvas.getContext('2d');
  let arr = new Uint8ClampedArray(Module.HEAPU8.buffer,
                                  bufptr, canvas.width *
                                  canvas.height * 4);
  let img = new ImageData(arr, canvas.width, canvas.height);
  ctx.putImageData(img, 0, 0);
};

var PushCanvasXYWH = (bufptr, xpos, ypos, width, height) => {
  var canvas = document.getElementById('main-canvas');
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
let bridge_setSize = function(width, height, xpos, ypos) {
  let inner = document.getElementById('main-scroll-inner');
  inner.style.width = width + 'px';
  inner.style.height = height + 'px';
  let outer = document.getElementById('main-scroll-outer');
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
  setTimeout(() => {
    console.log('freeing memory for firefox');
    window.URL.revokeObjectURL(data);
  }, 100);
};

let loadFile = function(element) {
  let file = element.target.files[0];
  if (!file) {
    return;
  }
  SetFileSize(file.size);

  let reader = new FileReader();
  reader.onload = function(el) {
    console.log('got a file');
    let data = new Uint8Array(reader.result);
    let buf = Module._malloc(data.length);
    Module.HEAPU8.set(data, buf);
    AppendFileBytes(buf, data.length);
    Module._free(buf);
    FinishFileLoad();
  }
  reader.readAsArrayBuffer(file);
}

