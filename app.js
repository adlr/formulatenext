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
let SetScale = null;
let SetSize = null;
let SetScrollOrigin = null;
let Init = null;
let SetFileSize = null;
let AppendFileBytes = null;
let FinishFileLoad = null;
let MouseDown = null
let MouseDrag = null;
let MouseUp = null
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
  SetScale = Module.cwrap('SetScale', null, ['number']);
  SetSize = Module.cwrap('SetSize', null, ['number', 'number']);
  SetScrollOrigin = Module.cwrap('SetScrollOrigin', null,
                                 ['number', 'number']);
  Init = Module.cwrap('Init', null, []);
  SetFileSize = Module.cwrap('SetFileSize', null, ['number']);
  AppendFileBytes = Module.cwrap('AppendFileBytes', null, ['number', 'number']);
  FinishFileLoad = Module.cwrap('FinishFileLoad', null, []);
  MouseDown = Module.cwrap('MouseDown', null, ['number', 'number']);
  MouseDrag = Module.cwrap('MouseDrag', null, ['number', 'number']);
  MouseUp = Module.cwrap('MouseUp', null, ['number', 'number']);
  DownloadFile = Module.cwrap('DownloadFile', null, []);
  UndoRedoClicked = Module.cwrap('UndoRedoClicked', null, ['number']);
  ToolbarClicked = Module.cwrap('ToolbarClicked', null, ['number']);
  UpdateEditText = Module.cwrap('UpdateEditText', null, ['string']);

  var outer = document.getElementById('outer');
  var canvas = document.getElementById('canvas');
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
    SetScale(dpr);
    SetSize(rect.width, rect.height);
  };
  window.addEventListener('optimizedResize', fixupContentSize);
  
  let calcFontMetrics = () => {
    let div = document.createElement('div');
    div.style.position = 'absolute';
    div.style.top = '-10000px';
    div.style.left = '-10000px';
    div.style.fontFamily = 'Arial';
    div.style.fontSize = '12px';
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
    let vertOffset = calcFontMetrics();
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

  // document.getElementById('zoom-in').onclick = zoomIn;
  // document.getElementById('zoom-out').onclick = zoomOut;
  let mouse_down = false;
  outer.addEventListener('mousedown', ev => {
    // if (ev.ctrlKey) {
    //   launchEditor(ev.offsetX - outer.scrollLeft,
    //                ev.offsetY - outer.scrollTop);
    //   return;
    // }

    if (MouseDown) {
      MouseDown(ev.offsetX - outer.scrollLeft,
                ev.offsetY - outer.scrollTop);
    }
    mouse_down = true;
  });
  outer.addEventListener('mousemove', ev => {
    if (mouse_down && MouseDrag) {
      MouseDrag(ev.offsetX - outer.scrollLeft,
                ev.offsetY - outer.scrollTop);
    }
  });
  outer.addEventListener('mouseup', ev => {
    if (mouse_down && MouseUp) {
      MouseUp(ev.offsetX - outer.scrollLeft,
                ev.offsetY - outer.scrollTop);
    }
    mouse_down = false;
  });

  document.getElementById('file-input').addEventListener('change',
                                                         loadFile, false);
}, false);

var PushCanvas = (bufptr, width, height) => {
  var canvas = document.getElementById('canvas');
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

// let g_zoom = 1.0;

// const zoomIn = function(ev) {
//   if (!runtime_ready) return;
//   g_zoom *= 1.1;
//   SetZoom(g_zoom);
// };
// const zoomOut = function(ev) {
//   if (!runtime_ready) return;
//   g_zoom /= 1.1;
//   SetZoom(g_zoom);
// };
// const zoom100 = function() {
//   if (!runtime_ready) return;
// };

// set the size/position of the scrollbar view
let bridge_setSize = function(width, height, xpos, ypos) {
  let inner = document.getElementById('inner');
  inner.style.width = width + 'px';
  inner.style.height = height + 'px';
  let outer = document.getElementById('outer');
  outer.scrollLeft = xpos;
  outer.scrollTop = ypos;
  // console.log('setsize ' +
  //             width + ', ' + height + ', ' + xpos + ', ' + ypos);
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

