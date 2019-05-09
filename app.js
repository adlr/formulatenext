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

document.addEventListener('DOMContentLoaded', function() {
  let fixupContentSize = null;
  Module['onRuntimeInitialized'] = function() {
    runtime_ready = true;
    console.log("runtime is ready");
    Init();
    fixupContentSize();
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
  
  document.getElementById('zoom-in').onclick = zoomIn;
  document.getElementById('zoom-out').onclick = zoomOut;
  let mouse_down = false;
  outer.addEventListener('mousedown', ev => {
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
    if (MouseUp) {
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

let g_zoom = 1.0;

const zoomIn = function(ev) {
  if (!runtime_ready) return;
  g_zoom *= 1.1;
  SetZoom(g_zoom);
};
const zoomOut = function(ev) {
  if (!runtime_ready) return;
  g_zoom /= 1.1;
  SetZoom(g_zoom);
};
const zoom100 = function() {
  if (!runtime_ready) return;
};

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

