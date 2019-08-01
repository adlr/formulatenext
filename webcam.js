"use strict";

let drawImageData = (imgdata) => {
  let canvas = document.createElement('canvas');
  canvas.width = imgdata.width;
  canvas.height = imgdata.height;
  if (canvas.width == 1920) {
    canvas.style.width = (canvas.width / 1) + 'px';
    canvas.style.height = (canvas.height / 1) + 'px';
  } else {
    canvas.style.width = canvas.width + 'px';
    canvas.style.height = canvas.height + 'px';
  }
  var ctx = canvas.getContext('2d');
  ctx.putImageData(imgdata, 0, 0);
  document.body.appendChild(canvas);
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
  img.src = 'full_bilateral_3x3.png';
}

// Input [0,255], output [0,1]
let sRGBtoLinear = (val) => {
  val /= 255;
  if (val <= 0.0031308)
    return val * 12.92;
  return 1.055 * Math.pow(val, 1 / 2.4) - 0.055;
};

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

// Inputs [0,1], outputs [0,1]
let rgbToyuv = (vals) => {
  return [(vals[0] *  0.2126  + vals[1] *  0.7152  + vals[2] *  0.0722),
          (vals[0] * -0.09991 + vals[1] * -0.33609 + vals[2] *  0.436),
          (vals[0] *  0.615   + vals[1] * -0.55861 + vals[2] * -0.05639)];
};

// Inputs [0,1], outputs [0,1]
let yuvTorgb = (vals) => {
  return [(vals[0] * 1 + vals[1] *  0       + vals[2] *  1.28033),
          (vals[0] * 1 + vals[1] * -0.21482 + vals[2] * -0.38059),
          (vals[0] * 1 + vals[1] *  2.12798 + vals[2] *  0)];
};

let imgDataToLinearGrey = (imgdata) => {
  let ret = [];
  for (let y = 0; y < imgdata.height; y++) {
    let row = [];
    for (let x = 0; x < imgdata.width; x++) {
      let idx = (imgdata.width * y + x) * 4;
      let rgb = [sRGBtoLinear(imgdata.data[idx]),
                 sRGBtoLinear(imgdata.data[idx + 1]),
                 sRGBtoLinear(imgdata.data[idx + 2])];
      let yuv = rgbToyuv(rgb);
      row.push(yuv[0]);
    }
    ret.push(row);
  }
  return ret;
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

let magicsubrect = (grey) => {
  let histsize = [(1920/3) | 0, (1080/6) | 0];
  let histcorner = [((1920 - histsize[0]) / 2) | 0,
                    ((1080 * 2 / 3) - histsize[1]) | 0];
  let ret = [];
  for (let y = 0; y < histsize[1]; y++) {
    let row = [];
    for (let x = 0; x < histsize[0]; x++) {
      row.push(grey[y + histcorner[1]][x + histcorner[0]]);
    }
    ret.push(row);
  }
  return ret;
};

let sobel2d = (grey) => {
  let height = grey.length;
  let width = height ? grey[0].length : 0;
  let ret = [];
  for (let y = 0; y < height; y++) {
    let row = [];
    for (let x = 0; x < width; x++) {
      if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
        row.push(0);
        continue;
      }
      let gx =
          -grey[y - 1][x - 1]     + grey[y - 1][x + 1] +
          -grey[y    ][x - 1] * 2 + grey[y    ][x + 1] * 2 +
          -grey[y + 1][x - 1]     + grey[y + 1][x + 1];
      let gy =
          -grey[y - 1][x - 1] - 2 * grey[y - 1][x] - grey[y - 1][x + 1] +
          grey[y  + 1][x - 1] + 2 * grey[y + 1][x] + grey[y + 1][x + 1];
      let out = Math.sqrt(gx * gx + gy * gy);
      row.push(out);
    }
    ret.push(row);
  }
  return ret;
};

let median = (grey, sobelGrey) => {
  let height = grey.length;
  let width = height ? grey[0].length : 0;
  let pairs = [];
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      pairs.push([grey[y][x], sobelGrey[y][x]]);
    }
  }

  // sort pairs based on grey value
  pairs.sort((left, right) => { return left[0] - right[0]; });

  // find median
  let leftsum = 0;
  let rightsum = 0;
  let leftidx = 0;
  let rightidx = pairs.length - 1;

  while (leftidx != rightidx) {
    if (leftsum < rightsum) {
      // increase left side
      leftsum += pairs[leftidx++][1];
    } else {
      // increase right side
      rightsum += pairs[rightidx--][1];
    }
  }
  return pairs[leftidx][0];
};

let threshold = (grey, level) => {
  let height = grey.length;
  let width = height ? grey[0].length : 0;
  let ret = [];
  for (let y = 0; y < height; y++) {
    let row = [];
    for (let x = 0; x < width; x++) {
      row.push(grey[y][x] >= level ? 1 : 0);
    }
    ret.push(row);
  }
  return ret;
};

let imgDataToSVG = (imgdata, cb) => {
  let pot = Potrace;
  pot.imgCanvas.width = imgdata.width;
  pot.imgCanvas.height = imgdata.height;
  let ctx = pot.imgCanvas.getContext('2d');
  ctx.putImageData(imgdata, 0, 0);
  pot.loadBm();
  pot.setParameter({turdsize: 0});
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

let onload = () => {
  loadImage((imgdata) => {
    drawImageData(imgdata);
    let grey = imgDataToLinearGrey(imgdata);
    let sobel = sobel2d(grey);
    let subSobel = magicsubrect(sobel);
    let sub = magicsubrect(grey);
    let med = median(sub, subSobel);
    let threshimg = threshold(grey, med);
    let threshImgData = linearGreyToImgData(threshimg)
    drawImageData(threshImgData);
    imgDataToSVG(threshImgData, drawSVG);
  });
  return;

  // Old code below, kept only b/c it allows capure from webcam.

  // Elements for taking the snapshot
  var canvas = document.getElementById('preview');
  var context = canvas.getContext('2d');
  var rawframe = document.getElementById('rawframe');
  var rawctx = rawframe.getContext('2d');
  var thresh = document.getElementById('thresh');
  var thctx = thresh.getContext('2d');
  var uv = document.getElementById('uv').getContext('2d');
  var go = true;
  let process = null;

  let loadvideo = () => {
    // Grab elements, create settings, etc.
    var video = document.getElementById('video');

    // Get access to the camera!
    if(navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
      // Not adding `{ audio: true }` since we only want video now
      navigator.mediaDevices.getUserMedia({ video: {
        width: { ideal: 4096 },
        height: { ideal: 2160 }
      }}).then(function(stream) {
        const track = stream.getVideoTracks()[0];
        const capabilities = track.getCapabilities();
        if (capabilities.focusDistance) {
          console.log("has focus distance");
        } else {
          console.log("no foc dist");
        }

        console.log(stream);
        //video.src = window.URL.createObjectURL(stream);
        video.srcObject = stream;
        video.play();
      });
    }

    let draw = () => {
      if (!go)
        return;
      context.drawImage(video, 0, 0, 1920, 1080);
      rawctx.drawImage(video, 0, 0, 1920, 1080);
      context.strokeStyle = "#ff0000";
      context.lineWidth = 3;
      context.beginPath();
      context.moveTo(100, 720);
      context.lineTo(1820, 720);
      context.stroke();
      requestAnimationFrame(draw);
    };
    requestAnimationFrame(draw);

    // Trigger photo take
    document.getElementById("snap").addEventListener("click", function() {
      console.log(`${video.videoWidth}x${video.videoHeight}`);
      go = !go;
      if (go) {
        console.log("go!");
        requestAnimationFrame(draw);
      } else {
        console.log("stop");
        video.srcObject.getTracks()[0].stop();
        process();

        var image = rawframe.toDataURL("image/png");/*.replace("image/png",
                                                      "image/octet-stream"); */
        var imagetag = new Image();
        imagetag.src = "" + image;

        var w = window.open("");
        w.document.write(imagetag.outerHTML);

        // console.log('' + image);
        // window.open('', '_blank');
        // var aLink = document.createElement('a');
        // aLink.download = 'image.png';
        // aLink.href = image;
        // aLink.click();
      }
    });
  }

  let loadimage = () => {
    let img = new Image();
    img.onload = () => {
      rawctx.drawImage(img, 0, 0, canvas.width, canvas.height);
      process();
    };
    img.src = 'webcam1.png';
  };

  //loadvideo();
  loadimage();

  let getHistogram = (data, left, top, width, height, rowbytes) => {
    let ret = [];
    ret.length = 256;
    ret.fill(0);
    for (let y = top; y < (top + height); y++) {
      for (let x = left; x < (left + width); x++) {
        let index = rowbytes * y + x * 4;
        let value = data[index];  // just getting red value
        ret[value]++;
      }
    }
    for (let i = 0; i < ret.length; i++) {
      ret[i] = Math.log(ret[i]);
    }
    return ret;
  };

  let plotHist = (hist) => {
    const max = Math.max(...hist);
    let elt = document.getElementById('hist')
    let ctx = elt.getContext('2d');
    ctx.beginPath();
    ctx.rect(0, 0, elt.width, elt.height);
    ctx.fillStyle = 'white';
    ctx.fill();

    ctx.beginPath();
    ctx.fillStyle = 'black';
    for (let i = 0; i < hist.length; i++) {
      let height = elt.height * hist[i] / max;
      ctx.rect(i, elt.height - height, 1, height);
    }
    ctx.fill();
  };

  let drawhistrect = (ctx, left, top, width, height) => {
    ctx.beginPath();
    ctx.strokeStyle = '#ff0000';
    context.lineWidth = 1;
    ctx.rect(left, top, width, height);
    ctx.stroke();
  };

  process = () => {
    let data = rawctx.getImageData(0, 0, 1920, 1080);
    let outData = thctx.getImageData(0, 0, 1920, 1080);
    let outUV = uv.getImageData(0, 0, 1920, 1080);
    for (let y = 0; y < 1080; y++) {
      for (let x = 0; x < 1920; x++) {
	let idx = (1920 * y + x) * 4;
        let rgb = [data.data[idx],
                   data.data[idx + 1],
                   data.data[idx + 2]].map(sRGBtoLinear);
        let yuv = rgbToyuv(rgb);
	let grey = linearTosRGB(yuv[0]);
        yuv[0] = 0.5;
	outData.data[idx] = outData.data[idx + 1] = outData.data[idx + 2] = grey;
	outData.data[idx + 3] = 0xff;
        let rgbColorOnly = yuvTorgb(yuv).map(linearTosRGB);
        outUV.data[idx] = rgbColorOnly[0];
        outUV.data[idx + 1] = rgbColorOnly[1];
        outUV.data[idx + 2] = rgbColorOnly[2];
        outUV.data[idx + 3] = 0xff;
      }
    }
    thctx.putImageData(outData, 0, 0);
    uv.putImageData(outUV, 0, 0);

    let histsize = [(1920/3) | 0, (1080/6) | 0];
    let histcorner = [((1920 - histsize[0]) / 2) | 0,
                      ((1080 * 2 / 3) - histsize[1]) | 0];

    drawhistrect(rawctx, histcorner[0], histcorner[1], histsize[0], histsize[1]);
    plotHist(getHistogram(outData.data, histcorner[0],
                          histcorner[1], histsize[0], histsize[1],
                          1920 * 4));
  };

  let origdata = null;
  document.getElementById('hist').addEventListener('mousemove', (evt) => {
    let cutoff = 256 * evt.offsetX / evt.target.clientWidth;
    if (!origdata) {
      origdata = thctx.getImageData(0, 0, 1920, 1080);
    }
    let data = origdata;
    let outData = uv.getImageData(0, 0, 1920, 1080);
    for (let y = 0; y < 1080; y++) {
      for (let x = 0; x < 1920; x++) {
        let idx = 4 * (1920 * y + x);
        const val = data.data[idx] > cutoff ? 0xff : 0x00;
        outData.data[idx] = val;
        outData.data[idx + 1] = val;
        outData.data[idx + 2] = val;
      }
    }
    uv.putImageData(outData, 0, 0);
  });
}

document.addEventListener('DOMContentLoaded', onload);

