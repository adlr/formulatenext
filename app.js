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

var doc;
var docview;
var viewOffset = new Point(0, 0);

let runtime_ready = false;

document.addEventListener('DOMContentLoaded', function() {
  Module['onRuntimeInitialized'] = function() {
    runtime_ready = true;
    console.log("runtime is ready");
    draw();
  };
  TestDraw = Module.cwrap('TestDraw', 'number',
                          ['number',  // width
                           'number']);  // height
  SetZoom = Module.cwrap('SetZoom', null, ['number']);

  var outer = document.getElementById('outer');
  var canvas = document.getElementById('canvas');
  throttle('scroll', 'optimizedScroll', outer);
  outer.addEventListener('optimizedScroll', function() {
    draw();
  });

  throttle('resize', 'optimizedResize');
  var fixupContentSize = function() {
    draw();
  };
  window.addEventListener('optimizedResize', fixupContentSize);
  
  document.getElementById('zoom-in').onclick = zoomIn;
  document.getElementById('zoom-out').onclick = zoomOut;
  let mouse_down = false;
  document.getElementById('inner').addEventListener('mousedown', ev => {
    console.log('mouse down ' + ev.offsetX + ', ' + ev.offsetY);
    mouse_down = true;
  });
  document.getElementById('inner').addEventListener('mousemove', ev => {
    if (mouse_down)
      console.log('mouse move ' + ev.offsetX + ', ' + ev.offsetY);
  });
  document.getElementById('inner').addEventListener('mouseup', ev => {
    console.log('mouse up   ' + ev.offsetX + ', ' + ev.offsetY);
    mouse_down = false;
  });

  document.getElementById('file-input').addEventListener('change',
                                                         loadFile, false);
}, false);

var draw = function() {
  var canvas = document.getElementById('canvas');
  var ctx = canvas.getContext('2d');

  // high DPI support
  var dpr = window.devicePixelRatio || 1;
  var rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);

  // do test string
  if (runtime_ready) {
    let bufptr = TestDraw(outer.scrollLeft, outer.scrollTop,
                          canvas.width, canvas.height, dpr);
    let arr = new Uint8ClampedArray(Module.HEAPU8.buffer,
                                    bufptr, canvas.width *
                                    canvas.height * 4);
    let img = new ImageData(arr, canvas.width, canvas.height);
    ctx.putImageData(img, 0, 0);
  }
};

let g_zoom = 1.0;

const zoomIn = function(ev) {
  g_zoom *= 1.1;
  SetZoom(g_zoom);
  draw();
};
const zoomOut = function(ev) {
  g_zoom /= 1.1;
  SetZoom(g_zoom);
  draw();
};
const zoom100 = function() {
  docview.zoomabs(1);
};

let bridge_setSize = function(width, height, xpos, ypos) {
  console.log('setsize ' +
              width + ', ' + height + ', ' + xpos + ', ' + ypos);
};

let loadFile = function(element) {
  let file = element.target.files[0];
  if (!file) {
    return;
  }
  let reader = new FileReader();
  reader.onload = function(el) {
    let contents = el.target.result;
    console.log('got a file');
  }
  reader.readAsArrayBuffer(file);
}

let loadPDF = function(arraybuffer) {

}
