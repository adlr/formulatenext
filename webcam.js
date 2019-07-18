"use strict";

let onload = () => {
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
    }
  });

  // Input [0,255], output [0,1]
  let sRGBtoLinear = (val) => {
    val /= 255;
    if (val <= 0.0031308)
      return val * 12.92;
    return 1.055 * Math.pow(val, 1 / 2.4) - 0.055;
  };
  
  // Input [0,1], output [0,255]
  let linearTosRGB = (val) => {
    if (val < 0 || val > 1) {
      console.log('overunder ' + val);
    }
    if (val <= 0.04045)
      return (255 / 12.92) * val;
    let ret = 255 * Math.pow((val + 0.055)/1.055, 2.4);
    return Math.min(255, ret);
  };

  let rgbToyuv = (vals) => {
    return [(vals[0] *  0.2126  + vals[1] *  0.7152  + vals[2] *  0.0722),
            (vals[0] * -0.09991 + vals[1] * -0.33609 + vals[2] *  0.436),
            (vals[0] *  0.615   + vals[1] * -0.55861 + vals[2] * -0.05639)];
  };

  let yuvTorgb = (vals) => {
    return [(vals[0] * 1 + vals[1] *  0       + vals[2] *  1.28033),
            (vals[0] * 1 + vals[1] * -0.21482 + vals[2] * -0.38059),
            (vals[0] * 1 + vals[1] *  2.12798 + vals[2] *  0)];
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
  };
}

document.addEventListener('DOMContentLoaded', onload);

