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

  process = () => {
    let rFact = 0.2126;
    let gFact = 0.7252;
    let bFact = 0.0722;

    let data = rawctx.getImageData(0, 0, 1920, 1080);
    console.log(data);
    let outData = thctx.getImageData(0, 0, 1920, 1080);
    for (let y = 0; y < 1080; y++) {
      for (let x = 0; x < 1920; x++) {
	let idx = (1920 * y + x) * 4;
	let grey = (data.data[idx] * rFact +
		    data.data[idx + 1] * gFact +
		    data.data[idx + 2] * bFact);
        grey = Math.min(Math.max(0, grey), 255);
	outData.data[idx] = outData.data[idx + 1] = outData.data[idx + 2] = grey;
	outData.data[idx + 3] = 0xff;
      }
    }
    thctx.putImageData(outData, 0, 0);
  };
}

document.addEventListener('DOMContentLoaded', onload);

