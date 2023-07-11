# Qos

- [Qos](#qos)
  - [Nack](#nack)

## Nack

- 关闭 FEC，以便抓包分析

  ```c++
  // webrtc/media/engine\webrtc_video_engine.cc
  //supported_formats.push_back(webrtc::SdpVideoFormat(kRedCodecName));
  //supported_formats.push_back(webrtc::SdpVideoFormat(kUlpfecCodecName));
  ```

- 关闭 SRTP，以便抓包分析

  ```c++
  // webrtc/api/peer_connection_interface.h
  disable_encryption = true;
  ```

- Conductor::CreatePeerConnection(bool dtls)配置不生成密钥

  ```c++
  // webrtc/examples/peerconnection/client/conductor.cc
  if (!CreatePeerConnection(/*dtls=*/false)) {
    main_wnd_->MessageBox("Error", "CreatePeerConnection failed", true);
    DeletePeerConnection();
  }
  ```

- [Nack](../rtp_rtcp/README.md#generic-nack-message) 包

  ![Nack包](../images/webrtc/Qos/nack/packet_nack.png)

- 接收端触发 Nack 堆栈

  ![接收端触发Nack堆栈](../images/webrtc/Qos/nack/receiver_callstack.png)

- 接收端定时触发 Nack 堆栈

  ![接收端定时触发Nack堆栈](../images/webrtc/Qos/nack/receiver_timer_callstack.png)

- [PLI](../rtp_rtcp/README.md#picture-loss-indication) 包

  ![PLI包](../images/webrtc/Qos/nack/packet_pli.png)

- 接收端触发 PLI 堆栈

  ![接收端触发PLI堆栈](../images/webrtc/Qos/nack/receiver_pli_callstack.png)
