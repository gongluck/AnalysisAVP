"use strict";

var audioSource = document.querySelector("select#audioSource");
var audioOutput = document.querySelector("select#audioOutput");
var videoSource = document.querySelector("select#videoSource");
var videoplay = document.querySelector("video#player");
var filtersSelect = document.querySelector("select#filter");
var snapshot = document.querySelector("button#snapshot");
var picture = document.querySelector("canvas#picture");
picture.width = 320;
picture.height = 240;
var divConstraints = document.querySelector("div#constraints");

function start() {
  if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
    console.log("getUserMedia is not supported!");
  } else {
    var deviceId = videoSource.value;
    var constraints = {
      video: {
        deviceId: deviceId ? deviceId : undefined,
        width: {
          max: 640,
          min: 300,
        },
        height: {
          max: 500,
          min: 200,
        },
        frameRate: 30,
        facingMode: "environment",
      },
      audio: {
        noiseSuppression: true,
        echoCancellation: true,
      },
    };
    navigator.mediaDevices
      .getUserMedia(constraints)
      .then(gotMediaStream)
      .then(gotDevices)
      .catch(handleError);
  }
}

function gotMediaStream(stream) {
  videoplay.srcObject = stream;
  var videoTrack = stream.getVideoTracks()[0];
  var videoConstraints = videoTrack.getSettings();
  divConstraints.textContent = JSON.stringify(videoConstraints, null, 2);
  return navigator.mediaDevices.enumerateDevices();
}

function gotDevices(deviceInfos) {
  deviceInfos.forEach(function (deviceinfo) {
    var option = document.createElement("option");
    option.text = deviceinfo.label;
    option.value = deviceinfo.deviceId;
    if (deviceinfo.kind === "audioinput") {
      audioSource.appendChild(option);
    } else if (deviceinfo.kind === "audiooutput") {
      audioOutput.appendChild(option);
    } else if (deviceinfo.kind === "videoinput") {
      videoSource.appendChild(option);
    }
  });
}

function handleError(err) {
  console.log(err.name + " : " + err.message);
}

start();

videoSource.onchange = start;

filtersSelect.onchange = function () {
  videoplay.className = filtersSelect.value;
};

snapshot.onclick = function () {
  picture
    .getContext("2d")
    .drawImage(videoplay, 0, 0, picture.width, picture.height);
};
