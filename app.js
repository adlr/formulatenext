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

document.addEventListener('DOMContentLoaded', function() {
  doc = new Doc(3);
  docview = new DocView(doc, function() {
    setTimeout(function() {
      console.log('size changed');
      const docSize = docview.size;
      const viewportSize = new Size(outer.clientWidth,
                                    outer.clientHeight);
      viewOffset.x = Math.max(viewportSize.width -
                              docSize.width, 0) / 2;
      viewOffset.y = Math.max(viewportSize.height -
                              docSize.height, 0) / 2;
      if (viewOffset.x) {
        inner.style.width = viewportSize.width + "px";
      } else {
        inner.style.width = docSize.width;
      }
      if (viewOffset.y) {
        inner.style.height = viewportSize.height + "px";
      } else {
        inner.style.height = docSize.height;
      }
      draw();
    }, 0);
  });

  var outer = document.getElementById('outer');
  var container = document.getElementById('container');
  var content = document.getElementById('content');
  var canvas = document.getElementById('canvas');
  console.log('start');
  throttle('scroll', 'optimizedScroll', outer);
  outer.addEventListener('optimizedScroll', function() {
    var dx = outer.scrollLeft;
    var dy = outer.scrollTop;
    container.style.transform = 'translate(' + dx + 'px, ' + dy + 'px)';
    draw();
  });

  throttle('resize', 'optimizedResize');
  var fixupContentSize = function() {
    canvas.style.height = content.style.height = outer.clientHeight + 'px';
    canvas.style.width = content.style.width = outer.clientWidth + 'px';
    canvas.width = outer.clientWidth;
    canvas.height = outer.clientHeight;
    draw();
  };
  fixupContentSize();
  window.addEventListener('optimizedResize', fixupContentSize);
  
  document.getElementById('zoomin').onclick = zoomIn;
  document.getElementById('zoomout').onclick = zoomOut;
  document.getElementById('zoom100').onclick = zoom100;
  document.getElementById('place').onclick = function() {
    console.log('place: ' + document.getElementById('string').value);
  };

  document.getElementById('file-input').addEventListener('change',
                                                         loadFile, false);
}, false);

var draw = function() {
  var canvas = document.getElementById('canvas');
  var inner = document.getElementById('inner');
  var outer = document.getElementById('outer');
  var ctx = canvas.getContext('2d');

  // high DPI support
  var dpr = window.devicePixelRatio || 1;
  var rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;
  ctx.scale(dpr, dpr);

  // ctx.font = "20px Arial";
  ctx.fillStyle = "#909090";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  // ctx.fillText("hi " + inner.clientHeight + ', ' + inner.clientWidth + '\n' +
  //              outer.scrollTop + ', ' + outer.scrollLeft + ', ' +
  //              outer.clientWidth + ', ' + outer.clientHeight, 30, 30);

  ctx.translate(-outer.scrollLeft, -outer.scrollTop);
  ctx.translate(viewOffset.x, viewOffset.y);
  
  docview.draw(ctx, new Rect(new Point(outer.scrollLeft, outer.scrollTop),
                             new Size(outer.clientWidth, outer.clientHeight)));
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
    let contents = el.target.result;
    console.log('got a file');
  }
  reader.readAsArrayBuffer(file);
}

let loadPDF = function(arraybuffer) {

}
