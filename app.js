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

document.addEventListener('DOMContentLoaded', function() {
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
  
  document.getElementById('zoomin').onclick = zoom;
  document.getElementById('zoomout').onclick = zoom;
  document.getElementById('zoom100').onclick = zoom;
  document.getElementById('place').onclick = function() {
    console.log('place: ' + document.getElementById('string').value);
  };
}, false);

var draw = function() {


  var canvas = document.getElementById('canvas');
  var inner = document.getElementById('inner');
  var outer = document.getElementById('outer');
  var ctx = canvas.getContext('2d');
  ctx.font = "20px Arial";
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.fillText("hi " + inner.clientHeight + ', ' + inner.clientWidth + '\n' +
               outer.scrollTop + ', ' + outer.scrollLeft + ', ' +
               outer.clientWidth + ', ' + outer.clientHeight, 30, 30);
};

var drawBox = function(ctx) {
  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.lineTo(10, 0);
  ctx.lineTo(10, 10);
  ctx.lineTo(0, 10);
  ctx.lineTo(0, 0);
  ctx.lineTo(10, 10);
  ctx.stroke();
};

var zoomFactor = 1;
var pagewidth = 612;  // points
var pageheight = 792;  // points
var pages = 3;
var spacing = 20;  // pixels

var zoom = function() {
  console.log('zoom');
};

var setupFakeDoc = function() {
  var totalHeight = pages * Math.ceil(pageheight * zoomFactor + spacing) + spacing;
  var totalWidth = Math.ceil(pagewidth * zoomFactor) + 2 * spacing;
  var inner = document.getElementById('inner');
  inner.style.width = totalWidth + "px";
  inner.style.height = totalHeight + "px";
};
