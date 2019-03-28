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

let Init = null;
let Render = null;

document.addEventListener('DOMContentLoaded', function() {
  document.getElementById('file-input').addEventListener('change',
                                                         loadFile, false);
  draw();
  Init = Module.cwrap('Init', 'number', ['number', 'number']);
  Render = Module.cwrap('Render', 'number',
                        ['number',  // width
                         'number',  // height
                         'number',  // ma
                         'number',  // mb
                         'number',  // mc
                         'number',  // md
                         'number',  // me
                         'number',  // mf
                         'number',  // sl
                         'number',  // st
                         'number',  // sr
                         'number']);  // sb
  Free = Module.cwrap('FreeBuffer', null, ['number']);
  document.getElementById('render').onclick = function() {
    let ma = document.getElementById('mat_a').value;
    let mb = document.getElementById('mat_b').value;
    let mc = document.getElementById('mat_c').value;
    let md = document.getElementById('mat_d').value;
    let me = document.getElementById('mat_e').value;
    let mf = document.getElementById('mat_f').value;
    let sl = document.getElementById('sz_l').value;
    let st = document.getElementById('sz_t').value;
    let sr = document.getElementById('sz_r').value;
    let sb = document.getElementById('sz_b').value;
    let canvas = document.getElementById('canvas');
    var ctx = canvas.getContext('2d');
    var dpr = window.devicePixelRatio || 1;
    var rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * dpr;
    canvas.height = rect.height * dpr;
    ctx.scale(dpr, dpr);

    let ptr = Render(canvas.width, canvas.height,
                     ma, mb, mc, md, me, mf, sl, st, sr, sb);
    // drop image in
    let arr = new Uint8ClampedArray(Module.HEAPU8.buffer,
                                    ptr, canvas.width *
                                    canvas.height * 4);
    let img = new ImageData(arr, canvas.width, canvas.height);
    ctx.putImageData(img, 0, 0);
    Free(ptr);
  };
}, false);

var draw = function() {
  let canvas = document.getElementById('canvas');
  var ctx = canvas.getContext('2d');

  // high DPI support
  var dpr = window.devicePixelRatio || 1;
  var rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);

  ctx.font = "20px Arial";
  ctx.fillStyle = "#909090";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = "#000000";
  ctx.fillText("hi", 30, 30);
};

const zoomIn = function() {
  docview.zoom(1.1);
};
const zoomOut = function() {
  docview.zoom(1/1.1);
};
const zoom100 = function() {
  docview.zoomabs(1);
};

let loadFile = function(element) {
  let file = element.target.files[0];
  if (!file) {
    return;
  }
  let reader = new FileReader();
  reader.onload = function(el) {
    let data = new Uint8Array(reader.result);
    let buf = Module._malloc(data.length);
    Module.HEAPU8.set(data, buf);
    let rv = Init(buf, data.length);
    console.log('got a file of length ' + data.length);
  };
  reader.readAsArrayBuffer(file);
}

