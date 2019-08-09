"use strict";

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

// return [left, top, width, height];
let magicRect = () => {
  let histsize = [(1920/3) | 0, (1080/6) | 0];
  let histcorner = [((1920 - histsize[0]) / 2) | 0,
                    ((1080 * 2 / 3) - histsize[1]) | 0];
  return [histcorner[0], histcorner[1],
          histsize[0], histsize[1]];
}

let magicsubrect = (grey) => {
  let rect = magicRect();
  let histsize = [rect[2], rect[3]];
  let histcorner = [rect[0], rect[1]];
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
  console.log('using idx ' + leftidx);
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
  document.body.appendChild(video);
  navigator.mediaDevices.getUserMedia({ video: {
    width: { ideal: 4096 },
    height: { ideal: 2160 }
  }}).then(function(stream) {
    setTimeout(() => {
      const track = stream.getVideoTracks()[0];
      const capabilities = track.getCapabilities();
      console.log(capabilities);
      if (capabilities.focusDistance) {
        console.log("has focus distance");
      } else {
        console.log("no foc dist");
      }
    }, 2000);

    console.log(stream);
    //video.src = window.URL.createObjectURL(stream);
    video.srcObject = stream;
    video.play();
  });
  
};

// cropping
// copy all elements in |right| into |left| if they aren't in |left|.
let mergeArray = (left, right) => {
  for (let i = 0; i < right.length; i++) {
    if (left.indexOf(right[i]) < 0)
      left.push(right[i]);
  }
}

class ComponentAnalizer {
  constructor(img) {
    this.img = img;
    this.height = img.length;
    this.width = this.height ? img[0].length : 0;
    this.nextID = 1;

    // 0 = unset, 1+ = ID
    this.idImg = [];
    for (let y = 0; y < this.height; y++) {
      let row = [];
      for (let x = 0; x < this.width; x++) {
        row.push(0);
      }
      this.idImg.push(row);
    }
  }
  componentAtPixel(xpos, ypos) {
    if (!this.idImg[ypos])
      console.log(`missing idImg[${ypos}]`);
    if (this.idImg[ypos][xpos] > 0)
      return this.idImg[ypos][xpos];
    if (this.img[ypos][xpos] != 0) {
      // pixel isn't black. invalid
      return -1;
    }
    // We have an unset black pixel. Use the next ID and mark every pixel
    // in this component.
    const id = this.nextID++;
    this.flood(xpos, ypos, id);
    return id;
  }
  // this is a contorted version of floodbad (below) that doesn't use recursion
  // because it was exceeding the maximum call stack length
  flood(xpos, ypos, id) {
    const kStart = 0;
    const kUp = 1;
    const kRight = 2;
    const kDown = 3;

    let stack = [[xpos, ypos, kStart]];
    while (stack.length > 0) {
      let elt = stack.pop();
      xpos = elt[0];
      ypos = elt[1];
      switch (elt[2]) {
      case kStart:
        if (!this.img) {
          console.log('missing img');
        }
        if (!this.img[ypos]) {
          console.log(`missing img[${ypos}]`);
        }
        if (!this.img[ypos].length) {
          console.log(`missing img[${ypos}].length`);
        }
        if (this.img[ypos][xpos] != 0)
          continue;
        if (this.idImg[ypos][xpos] > 0)
          continue;  // already has an ID
        this.idImg[ypos][xpos] = id;
        if (xpos > 0) {
          stack.push([xpos, ypos, kUp]);
          stack.push([xpos - 1, ypos, kStart]);
          continue;
        }
      case kUp:
        if (ypos > 0) {
          stack.push([xpos, ypos, kRight]);
          stack.push([xpos, ypos - 1, kStart]);
          continue;
        }
      case kRight:
        if (xpos < (this.width - 1)) {
          stack.push([xpos, ypos, kDown]);
          stack.push([xpos + 1, ypos, kStart]);
          continue;
        }
      case kDown:
        if (ypos < (this.height - 1)) {
          stack.push([xpos, ypos + 1, kStart]);
          continue;
        }
      }
    }
  }

  floodbad(xpos, ypos, id) {
    if (this.img[ypos][xpos] != 0)
      return;
    this.idImg[ypos][xpos] = id;
    if (xpos > 0)
      this.flood(xpos - 1, ypos, id);
    if (ypos > 0)
      this.flood(xpos, ypos - 1, id);
    if (xpos < this.width)
      this.flood(xpos + 1, ypos, id);
    if (ypos < this.height)
      this.flood(xpos, ypos + 1, id);
  }
  componentIsValid(id) {
    // A component is valid if it's not too thick/big at any given point
    const kInvalidSize = 20;
    for (let y = 0; y < this.height - kInvalidSize; y++) {
      for (let x = 0; x < this.width - kInvalidSize; x++) {
        // Sanity-check two corners before doing exhaustive search
        if (this.idImg[y][x] != id ||
            this.idImg[y + kInvalidSize - 1][x + kInvalidSize - 1] != id)
          continue;
        // Do exhaustive search
        let valid = false;
        for (let i = y; i < y + kInvalidSize; i++) {
          for (let j = x; j < x + kInvalidSize; j++) {
            if (this.idImg[i][j] != id) {
              i = y + kInvalidSize;  // break out of two loops
              valid = true;
              break;
            }
          }
        }
        if (!valid)
          return false;
      }
    }
    return true;
  }
  findNearbyQuick(id) {
    let leftTest = [];
    let upTest = [];
    let rightTest = [];
    let downTest = [];
    let generateLists = () => {
      const kMaxDist = 20;
      const kMinThreshSq = (kMaxDist - 0.5) * (kMaxDist - 0.5);
      const kMaxThreshSq = (kMaxDist + 0.5) * (kMaxDist + 0.5);
      for (let ypos = -kMaxDist; ypos <= kMaxDist; ypos++) {
        for (let xpos = -kMaxDist; xpos <= kMaxDist; xpos++) {
          let distSq = xpos * xpos + ypos * ypos;
          if (distSq > kMinThreshSq && distSq < kMaxThreshSq) {
            if (ypos < 0 && Math.abs(xpos) <= Math.abs(ypos))
              upTest.push([xpos, ypos]);
            if (xpos < 0 && Math.abs(ypos) <= Math.abs(xpos))
              leftTest.push([xpos, ypos]);
            if (ypos > 0 && Math.abs(xpos) <= Math.abs(ypos))
              downTest.push([xpos, ypos]);
            if (xpos > 0 && Math.abs(ypos) <= Math.abs(xpos))
              rightTest.push([xpos, ypos]);
          }
        }
      }
    };
    generateLists();
    let hitComponents = (hitList, xpos, ypos) => {
      let ret = [];
      for (let i = 0; i < hitList.length; i++) {
        let x = xpos + hitList[i][0];
        let y = ypos + hitList[i][1];
        if (x >= 0 && y >= 0 && x < this.width && y < this.height) {
          let hitComponent = this.componentAtPixel(x, y);
          if (hitComponent != id && hitComponent > 0)
            ret.push(hitComponent);
        }
      }
      return ret;
    };
    let ret = [];
    for (let y = 0; y < this.height; y++) {
      for (let x = 0; x < this.width; x++) {
        if (this.componentAtPixel(x, y) != id)
          continue;
        if (x > 0 && this.componentAtPixel(x - 1, y) < 0)
          mergeArray(ret, hitComponents(leftTest, x, y));
        if (y > 0 && this.componentAtPixel(x, y - 1) < 0)
          mergeArray(ret, hitComponents(upTest, x, y));
        if (x < (this.width - 1) && this.componentAtPixel(x + 1, y) < 0)
          mergeArray(ret, hitComponents(rightTest, x, y));
        if (y < (this.height - 1) && this.componentAtPixel(x, y + 1) < 0)
          mergeArray(ret, hitComponents(downTest, x, y));
      }
    }
    return ret;
  }
  findNearby(id, visited, xpos, ypos, iteration) {
    const kMaxDistance = 3;  // todo: raise higher
    if (iteration > kMaxDistance)
      return [];
    if (xpos < 0 || ypos < 0) {
      console.log(`err: ${id} ${xpos} ${ypos} ${iteration}`);
    }
    let component = this.componentAtPixel(xpos, ypos);
    if (component > 0 && component != id)
      return [component];
    if (component == id)
      return [];
    let ret = [];
    if (visited[ypos][xpos] == 0 || visited[ypos][xpos] > iteration) {
      // visit this cell
      visited[ypos][xpos] = iteration;
      if (xpos > 0)
        mergeArray(ret, this.findNearby(id, visited, xpos - 1, ypos,
                                        iteration + 1));
      if (ypos > 0)
        mergeArray(ret, this.findNearby(id, visited, xpos, ypos - 1,
                                        iteration + 1));
      if (xpos < this.width - 1)
        mergeArray(ret, this.findNearby(id, visited, xpos + 1, ypos,
                                        iteration + 1));
      if (ypos < this.height - 1)
        mergeArray(ret, this.findNearby(id, visited, xpos, ypos + 1,
                                        iteration + 1));
    }
    return ret;
  }
  nearbyComponents(id) {
    // create a new matrix to store progress
    let visited = [];
    for (let i = 0; i < this.height; i++) {
      let row = [];
      row.length = this.width;
      row.fill(0);
      visited.push(row);
    }
    let ret = [];
    for (let y = 0; y < this.height; y++) {
      for (let x = 0; x < this.width; x++) {
        mergeArray(ret, this.findNearby(id, visited, x, y, 1));
      }
    }
    return ret;
  }
  // Get all components reachable from the given subrectangle
  getReachableComponents(left, top, right, bottom) {
    let ret = [];
    for (let y = top; y < bottom; y++) {
      for (let x = left; x < right; x++) {
        const component = this.componentAtPixel(x, y);
        if (component < 0)
          continue;
        if (ret.indexOf(component) >= 0)
          continue;
        if (!this.componentIsValid(component)) {
          console.log('found invalid component in subrect');
          return [];
        }
        mergeArray(ret, [component]);
      }
    }
    if (ret.length == 0) {
      console.log('no components found');
      return ret;
    }
    console.log('found ' + ret.length + ' components in box');
    // expand with reachable valid components
    let todo = Array.from(ret);  // copy array
    let invalid = [];
    while (todo.length > 0) {
      console.log(todo);
      let found = [];  // IDs found this iteration
      for (let i = 0; i < todo.length; i++) {
        let reachable = this.findNearbyQuick(todo[i]);
        for (let j = 0; j < reachable.length; j++) {
          const nearcomp = reachable[j];
          if (invalid.indexOf(nearcomp) >= 0)
            continue;
          if (ret.indexOf(nearcomp) < 0 &&
              found.indexOf(nearcomp) < 0) {
            if (!this.componentIsValid(nearcomp)) {
              invalid.push(nearcomp);
              continue;
            }
            found.push(nearcomp);
          }
        }
      }
      ret = ret.concat(found);
      todo = found;
    }
    return ret;
  }
  // Get an image with just certain components
  imageWithComponents(ids) {
    let ret = [];
    for (let y = 0; y < this.height; y++) {
      let row = [];
      for (let x = 0; x < this.width; x++) {
        if (ids.indexOf(this.idImg[y][x]) >= 0)
          row.push(0);
        else
          row.push(1);
      }
      ret.push(row);
    }
    return ret;
  }
}

// crop all white (1) pixels in |grey| to a new matrix and return it.
// Keep |border| pixels around the crop.
let crop = (grey, border) => {
  const height = grey.length;
  const width = height == 0 ? 0 : grey[0].length;
  let left = 0;
  let top = 0;
  let right = width;
  let bottom = height;
  topLoop:
  for ( ; top < bottom; top++) {
    for (let x = 0; x < width; x++) {
      if (grey[top][x] != 1)
        break topLoop;
    }
  }
  bottomLoop:
  for ( ; bottom > top; bottom--) {
    for (let x = 0; x < width; x++) {
      if (grey[bottom - 1][x] != 1)
        break bottomLoop;
    }
  }
  leftLoop:
  for ( ; left < right; left++) {
    for (let y = top; y < bottom; y++) {
      if (grey[y][left] != 1)
        break leftLoop;
    }
  }
  rightLoop:
  for ( ; right > left; right--) {
    for (let y = top; y < bottom; y++) {
      if (grey[y][right - 1] != 1)
        break rightLoop;
    }
  }
  console.log(`crop ${width} x ${height} -> ${left} ${top} ${right} ${bottom}`);
  if (left == right || top == bottom)
    return [];
  left = Math.max(0, left - border);
  top = Math.max(0, top - border);
  right = Math.min(width, right + border);
  bottom = Math.min(height, bottom + border);
  // Copy to new matrix
  let ret = [];
  for (let y = top; y < bottom; y++) {
    let row = [];
    for (let x = left; x < right; x++) {
      row.push(grey[y][x])
    }
    ret.push(row);
  }
  return ret;
};

let onload = () => {
  

  loadImage((imgdata) => {
    let grey = imgDataToLinearGrey(imgdata);
    let sobel = sobel2d(grey);
    let subSobel = magicsubrect(sobel);
    let sub = magicsubrect(grey);
    let med = median(sub, subSobel);
    console.log('median: ' + med);
    let threshimg = threshold(grey, med);
    let threshImgData = linearGreyToImgData(threshimg)
    drawImageData(threshImgData);

    let ca = new ComponentAnalizer(threshimg);
    const rect = magicRect();
    console.log("getting reachable components");
    let ids = ca.getReachableComponents(rect[0], rect[1],
                                        rect[0] + rect[2],
                                        rect[1] + rect[3]);
    console.log("drawing components: " + ids);
    let sigGrey = ca.imageWithComponents(ids);
    console.log("cropping");
    let croppedSigGrey = crop(sigGrey, 5);
    console.log('converting to imgdata');
    let sigImgData = linearGreyToImgData(croppedSigGrey);
    drawImageData(sigImgData);

    imgDataToSVG(sigImgData, drawSVG);
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
    img.src = 'webcam2.png';
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

