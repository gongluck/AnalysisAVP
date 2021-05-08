"use strict";

var audioSource = document.querySelector("select#audioSource");
var audioOutput = document.querySelector("select#audioOutput");
var videoSource = document.querySelector("select#videoSource");

if (!navigator.mediaDevices || !navigator.mediaDevices.enumerateDevices) {
  console.log("enumerateDevices is not supported!");
} else {
  navigator.mediaDevices.enumerateDevices().then(gotDevices).catch(handleError);
}

function gotDevices(deviceInfos) {
  deviceInfos.forEach(function (deviceInfo) {
    console.log(
      deviceInfo.kind +
        "\r\nlabel = " +
        deviceInfo.label +
        "\r\nid = " +
        deviceInfo.deviceId +
        "\r\ngroupId = " +
        deviceInfo.groupId
    );

    var option = document.createElement("option");
    option.text = deviceInfo.label;
    option.value = deviceInfo.deviceId;

    if (deviceInfo.kind === "audioinput") {
      audioSource.appendChild(option);
    } else if (deviceInfo.kind === "audiooutput") {
      audioOutput.appendChild(option);
    } else if (deviceInfo.kind === "videoinput") {
      videoSource.appendChild(option);
    }
  });
}

function handleError(err) {
  console.log(err.name + " : " + err.message);
}
