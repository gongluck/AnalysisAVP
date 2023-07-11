# RTP/RTCP

- [RTP/RTCP](#rtprtcp)
  - [RTP](#rtp)
    - [RTP Fixed Header](#rtp-fixed-header)
    - [RTP Header Extension](#rtp-header-extension)
  - [RTCP](#rtcp)
    - [Feedback Messages Header](#feedback-messages-header)
    - [Feedback Control Information](#feedback-control-information)
      - [Generic NACK message](#generic-nack-message)
      - [Picture Loss Indication](#picture-loss-indication)

## RTP

### RTP Fixed Header

```c++
   The RTP header has the following format:

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

```

- V：RTP 协议的版本号，占 2 位，当前协议版本号为 2。
- P：填充标志，占 1 位，如果 P=1，则在该报文的尾部填充一个或多个额外的八位组，它们不是有效载荷的一部分。
- X：扩展标志，占 1 位，如果 X=1，则在 RTP 报头后跟有一个扩展报头。
- CC：CSRC 计数器，占 4 位，指示 CSRC 标识符的个数。
- M: 标记，占 1 位，不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始。
- PT: 有效载荷类型，占 7 位，用于说明 RTP 报文中有效载荷的类型，如 GSM 音频、JPEM 图像等，在流媒体中大部分是用来区分音频流和视频流的，这样便于客户端进行解析。
- 序列号：占 16 位，用于标识发送者所发送的 RTP 报文的序列号，每发送一个报文，序列号增 1。这个字段当下层的承载协议用 UDP 的时候，网络状况不好的时候可以用来检查丢包。同时出现网络抖动的情况可以用来对数据进行重新排序，在 helix 服务器中这个字段是从 0 开始的，同时音频包和视频包的 sequence 是分别记数的。
- 时戳(Timestamp)：占 32 位，时戳反映了该 RTP 报文的第一个八位组的采样时刻。接收者使用时戳来计算延迟和延迟抖动，并进行同步控制。
- 同步信源(SSRC)标识符：占 32 位，用于标识同步信源。该标识符是随机选择的，用于唯一地标识一个实时媒体流。在 WebRTC 中，每个媒体流（例如音频或视频）都有一个唯一的 SSRC 值，用于区分不同的流。
- 特约信源(CSRC)标识符：每个 CSRC 标识符占 32 位，可以有 0 ～ 15 个。每个 CSRC 标识了包含在该 RTP 报文有效载荷中的所有特约信源，是在混音或合成多个音频流时使用的标识符。当多个音频流混合在一起时，每个音频流都会贡献一部分数据，CSRC 用于标识每个输入流的贡献者。

### RTP Header Extension

```c++
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      defined by profile       |           length              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        header extension                       |
   |                             ....                              |

```

- 若 RTP 固定头中的扩展比特位置 1，则一个长度可变的头扩展部分被加到 RTP 固定头之后。
- RTP 固定头之后只允许有一个头扩展。为允许多个互操作实现独立生成不同的头扩展，或某种特定实现有多种不同的头扩展，扩展项的前 16 比特用以识别标识符或参数。这 16 比特的格式由具体实现的上层协议定义。
- 头扩展包含 16 比特的长度域，指示扩展项中 32 比特字的个数，不包括 4 个字节扩展头(因此零是有效值)。
- 基本的 RTP 说明并不定义任何头扩展本身。

## RTCP

### Feedback Messages Header

```c++
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|   FMT   |       PT      |          length               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of packet sender                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  SSRC of media source                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :            Feedback Control Information (FCI)      

  Payload type (PT): 8 bits
    This is the RTCP packet type that identifies the packet as being
    an RTCP FB message.  Two values are defined by the IANA:

    Name   | Value | Brief Description
    ----------+-------+------------------------------------
    RTPFB  |  205  | Transport layer FB message
    PSFB   |  206  | Payload-specific FB message
```

### Feedback Control Information

#### Generic NACK message

```c++
  The Generic NACK message is identified by PT=RTPFB and FMT=1.

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            PID                |             BLP               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Picture Loss Indication

```c++
   The PLI FB message is identified by PT=PSFB and FMT=1.
   There MUST be exactly one PLI contained in the FCI field.
```