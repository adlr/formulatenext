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

let ProcessImage = null;

let bridge_showGrey = (ptr, width, height) => {
  const arr = new Float32Array(Module.HEAPF32.buffer, ptr, width * height);
  let img = [];
  for (let y = 0; y < height; y++) {
    let row = [];
    for (let x = 0; x < width; x++) {
      row.push(arr[width * y + x]);
      if (x == 1000 && y == 500) {
	console.log('jsval: ' + arr[width * y + x]);
      }
    }
    img.push(row);
  }
  drawImageData(linearGreyToImgData(img));
};

document.addEventListener('DOMContentLoaded', () => {
  Module['onRuntimeInitialized'] = () => {
    ProcessImage = Module.cwrap('ProcessImage', null,
				['number', 'number', 'number']);
    loadImage((imgdata) => {
      console.log('got an image');
      let buf = Module._malloc(imgdata.width * imgdata.height * 4)
      Module.HEAPU8.set(imgdata.data, buf);
      ProcessImage(buf, imgdata.width, imgdata.height);
    });
  };
});

