// Copyright...

// Debug:

// Input [0,1], output [0,255]
let linearTosRGB = (val) => {
  // if (val < 0 || val > 1) {
  //   console.log('overunder ' + val);
  // }
  val = Math.min(Math.max(0, val), 1);
  if (val <= 0.04045)
    return (255 / 12.92) * val;
  let ret = 255 * Math.pow((val + 0.055)/1.055, 2.4);
  return Math.min(255, ret);
};

let drawImageData = (imgdata) => {
  let canvas = document.createElement('canvas');
  canvas.width = imgdata.width;
  canvas.height = imgdata.height;
  if (canvas.width == 1920) {
    canvas.style.width = (canvas.width / 4) + 'px';
    canvas.style.height = (canvas.height / 4) + 'px';
  } else {
    canvas.style.width = canvas.width + 'px';
    canvas.style.height = canvas.height + 'px';
  }
  var ctx = canvas.getContext('2d');
  ctx.putImageData(imgdata, 0, 0);
  document.body.appendChild(canvas);
};

let linearGreyToImgData = (grey) => {
  let height = grey.length;
  let width = height ? grey[0].length : 0;
  let ret = new ImageData(width, height);
  let idx = 0;
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      let srgb = linearTosRGB(grey[y][x]);
      ret.data[idx++] = srgb;
      ret.data[idx++] = srgb;
      ret.data[idx++] = srgb;
      ret.data[idx++] = 0xff;
    }
  }
  return ret;
};

let loadImage = (cb) => {
  let img = new Image();
  img.onload = () => {
    let canvas = document.createElement('canvas');
    canvas.width = img.width;
    canvas.height = img.height;
    let ctx = canvas.getContext('2d');
    ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
    let imgdata = ctx.getImageData(0, 0, canvas.width, canvas.height);
    cb(imgdata);
  };
  //img.src = 'full_bilateral_3x3.png';
  img.src = 'webcam1.png';
}

let imgDataToSVG = (imgdata, cb) => {
  let pot = Potrace;
  pot.imgCanvas.width = imgdata.width;
  pot.imgCanvas.height = imgdata.height;
  let ctx = pot.imgCanvas.getContext('2d');
  ctx.putImageData(imgdata, 0, 0);
  pot.loadBm();
  pot.setParameter({turdsize: 0, alphamax: 1.334});
  pot.process(() => {
    let svg = pot.getSVG(1);
    cb(svg);
  });
};

let drawSVG = (str) => {
  let div = document.createElement('div');
  div.innerHTML = str;
  document.body.appendChild(div);
};

let showCapture = () => {
  // console.log(navigator.mediaDevices.getSupportedConstraints());
  let video = document.createElement('video');
  video.width = 1920;
  video.height = 1080;
  // document.body.appendChild(video);
  navigator.mediaDevices.getUserMedia({ video: {
    width: { ideal: 4096 },
    height: { ideal: 2160 }
  }}).then(function(stream) {
    console.log(stream);
    //video.src = window.URL.createObjectURL(stream);
    video.srcObject = stream;
    video.play();
  });

  let canvas = document.createElement('canvas');
  canvas.width = 1920;
  canvas.height = 1080;
  canvas.style.width = (canvas.width / 4) + 'px';
  canvas.style.height = (canvas.height / 4) + 'px';
  var ctx = canvas.getContext('2d');
  document.body.appendChild(canvas);

  let rawcanvas = document.createElement('canvas');
  rawcanvas.width = 1920;
  rawcanvas.height = 1080;
  rawcanvas.style.display = 'none';
  var rawctx = rawcanvas.getContext('2d');
  document.body.appendChild(rawcanvas);
  ctx.translate(1920, 0);
  ctx.scale(-1, 1);

  let rectWidth = (1920 / 6) | 0;
  let rectHeight = (1080 / 6) | 0;
  let rectLeft = ((1920 - rectWidth) / 2) | 0;
  let rectTop = ((1080 * 2 / 3) - rectHeight) | 0;

  let preview = () => {
    rawctx.drawImage(video, 0, 0, 1920, 1080);
    ctx.save();
    ctx.translate(-1920 / 2, -1080 / 2);
    ctx.scale(2, 2);
    ctx.drawImage(video, 0, 0, 1920, 1080);
    ctx.strokeStyle = "#000000";
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(rectLeft, rectTop)
    ctx.lineTo(rectLeft + rectWidth, rectTop);
    ctx.moveTo(rectLeft, rectTop + rectHeight)
    ctx.lineTo(rectLeft + rectWidth, rectTop + rectHeight);
    ctx.stroke();
    ctx.restore();
    requestAnimationFrame(preview);
  };
  requestAnimationFrame(preview);

  let button = document.createElement('button');
  button.innerHTML = 'Snap';
  document.body.appendChild(button);
  button.addEventListener('click', () => {
    let imgdata = rawctx.getImageData(0, 0, 1920, 1080);
    let buf = Module._malloc(imgdata.width * imgdata.height * 4);
    Module.HEAPU8.set(imgdata.data, buf);
    ProcessImage(buf, imgdata.width, imgdata.height);
  });
};

let ProcessImage = null;

let bridge_showGrey = (ptr, width, height, rowbytes) => {
  const arr = new Float32Array(Module.HEAPF32.buffer, ptr,
			       (rowbytes / 4) * height);
  let img = [];
  for (let y = 0; y < height; y++) {
    let row = [];
    for (let x = 0; x < width; x++) {
      row.push(arr[(rowbytes / 4) * y + x]);
    }
    img.push(row);
  }
  drawImageData(linearGreyToImgData(img));
};

let bridge_renderBitmap = (ptr, width, height, rowbytes) => {
  const arr = new Float32Array(Module.HEAPF32.buffer, ptr,
			       (rowbytes / 4) * height);
  let img = [];
  for (let y = 0; y < height; y++) {
    let row = [];
    for (let x = 0; x < width; x++) {
      row.push(arr[(rowbytes / 4) * y + x]);
    }
    img.push(row);
  }
  imgDataToSVG(linearGreyToImgData(img), drawSVG);
};

document.addEventListener('DOMContentLoaded', () => {
  Module['onRuntimeInitialized'] = () => {
    ProcessImage = Module.cwrap('ProcessImage', null,
				['number', 'number', 'number']);
    showCapture();
    // loadImage((imgdata) => {
    //   console.log('got an image');
    //   let buf = Module._malloc(imgdata.width * imgdata.height * 4)
    //   Module.HEAPU8.set(imgdata.data, buf);
    //   ProcessImage(buf, imgdata.width, imgdata.height);
    // });
  };
});

