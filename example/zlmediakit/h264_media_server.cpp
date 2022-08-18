/*
 * @Author: gongluck
 * @Date: 2022-08-11 14:57:52
 * @Last Modified by: gongluck
 * @Last Modified time: 2022-08-11 15:10:58
 */

#include <iostream>
#include <fstream>
#include <thread>

#include "mk_mediakit.h"

const char *publicip = "192.168.0.145";
unsigned short httport = 80;
unsigned short rtsport = 554;
unsigned short rtmport = 1935;
const char *publicport = "5555";
unsigned short localport = 5555;

#define CHECKERR(err, OK)                                                \
  if ((err) != (OK))                                                     \
  {                                                                      \
    std::cerr << "error : " << (err) << " in " << __LINE__ << std::endl; \
    exit(err);                                                           \
  }

static void on_h264_frame(void *user_data, mk_h264_splitter splitter, const char *data, int size)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  static int dts = 0;
  mk_frame frame = mk_frame_create(MKCodecH264, dts, dts, data, size, NULL, NULL);
  dts += 40;
  int ret = mk_media_input_frame((mk_media)user_data, frame);
  CHECKERR(ret, 1);
  mk_frame_unref(frame);
}

/**
 * 收到http api请求广播(包括GET/POST)
 * @param parser http请求内容对象
 * @param invoker 执行该invoker返回http回复
 * @param consumed 置1则说明我们要处理该事件
 * @param sender http客户端相关信息
 */
//测试url : http://127.0.0.1/api/test
void API_CALL on_mk_http_request(
    const mk_parser parser, const mk_http_response_invoker invoker, int *consumed, const mk_sock_info sender)
{
  char ip[64];
  log_printf(
      4,
      "client info, local: %s:%d, peer: %s:%d\n"
      "%s %s?%s %s\n"
      "User-Agent: %s\n"
      "%s",
      mk_sock_info_local_ip(sender, ip), mk_sock_info_local_port(sender), mk_sock_info_peer_ip(sender, ip + 32),
      mk_sock_info_peer_port(sender), mk_parser_get_method(parser), mk_parser_get_url(parser),
      mk_parser_get_url_params(parser), mk_parser_get_tail(parser), mk_parser_get_header(parser, "User-Agent"),
      mk_parser_get_content(parser, NULL));

  const char *url = mk_parser_get_url(parser);
  *consumed = 1;

  //拦截api: /api/test
  if (strcmp(url, "/api/test") == 0)
  {
    const char *response_header[] = {"Content-Type", "text/html", NULL};
    const char *content = "<html>"
                          "<head>"
                          "<title>hello world</title>"
                          "</head>"
                          "<body bgcolor=\"white\">"
                          "<center><h1>hello world</h1></center><hr>"
                          "<center>"
                          "ZLMediaKit-4.0</center>"
                          "</body>"
                          "</html>";
    mk_http_body body = mk_http_body_from_string(content, 0);
    mk_http_response_invoker_do(invoker, 200, response_header, body);
    mk_http_body_release(body);
  }
  //拦截api: /index/api/webrtc
  else if (strcmp(url, "/index/api/webrtc") == 0)
  {
    mk_webrtc_http_response_invoker_do(invoker, parser, sender);
  }
  else
  {
    *consumed = 0;
    return;
  }
}

/**
 * 在http文件服务器中,收到http访问文件或目录的广播,通过该事件控制访问http目录的权限
 * @param parser http请求内容对象
 * @param path 文件绝对路径
 * @param is_dir path是否为文件夹
 * @param invoker 执行invoker返回本次访问文件的结果
 * @param sender http客户端相关信息
 */
void API_CALL on_mk_http_access(
    const mk_parser parser, const char *path, int is_dir, const mk_http_access_path_invoker invoker,
    const mk_sock_info sender)
{
  char ip[64];
  log_printf(
      4,
      "client info, local: %s:%d, peer: %s:%d, path: %s ,is_dir: %d\n"
      "%s %s?%s %s\n"
      "User-Agent: %s\n"
      "%s",
      mk_sock_info_local_ip(sender, ip), mk_sock_info_local_port(sender), mk_sock_info_peer_ip(sender, ip + 32),
      mk_sock_info_peer_port(sender), path, (int)is_dir, mk_parser_get_method(parser), mk_parser_get_url(parser),
      mk_parser_get_url_params(parser), mk_parser_get_tail(parser), mk_parser_get_header(parser, "User-Agent"),
      mk_parser_get_content(parser, NULL));

  //有访问权限,每次访问文件都需要鉴权
  mk_http_access_path_invoker_do(invoker, NULL, NULL, 0);
}

/**
 * 在http文件服务器中,收到http访问文件或目录前的广播,通过该事件可以控制http url到文件路径的映射
 * 在该事件中通过自行覆盖path参数，可以做到譬如根据虚拟主机或者app选择不同http根目录的目的
 * @param parser http请求内容对象
 * @param path 文件绝对路径,覆盖之可以重定向到其他文件
 * @param sender http客户端相关信息
 */
void API_CALL on_mk_http_before_access(const mk_parser parser, char *path, const mk_sock_info sender)
{
  char ip[64];
  log_printf(
      4,
      "client info, local: %s:%d, peer: %s:%d, path: %s\n"
      "%s %s?%s %s\n"
      "User-Agent: %s\n"
      "%s",
      mk_sock_info_local_ip(sender, ip), mk_sock_info_local_port(sender), mk_sock_info_peer_ip(sender, ip + 32),
      mk_sock_info_peer_port(sender), path, mk_parser_get_method(parser), mk_parser_get_url(parser),
      mk_parser_get_url_params(parser), mk_parser_get_tail(parser), mk_parser_get_header(parser, "User-Agent"),
      mk_parser_get_content(parser, NULL));
  //覆盖path的值可以重定向文件
}

int main(int argc, char *argv[])
{
  std::cout << "zlmediakit demo" << std::endl;
  std::cout << "Usage : "
            << "thisfilename h264file" << std::endl;
  if (argc < 2)
  {
    std::cerr << "please see the usage message." << std::endl;
    return -1;
  }
  std::ifstream h264in(argv[1], std::ios::binary);
  if (h264in.fail())
  {
    std::cerr << "can not open file " << argv[1] << std::endl;
    return -1;
  }

  int ret = 0;

  mk_config config = {0};
  config.ini = NULL,
  config.ini_is_path = 1;
  config.log_level = 0;
  config.log_mask = LOG_CONSOLE | LOG_FILE;
  config.log_file_path = "./zlmlog";
  config.log_file_days = 0;
  config.ssl = NULL;
  config.ssl_is_path = 1;
  config.ssl_pwd = NULL;
  config.thread_num = 0;
  mk_env_init(&config);
  mk_set_option("http.rootPath", "./www");
  ret = mk_http_server_start(httport, 0);
  CHECKERR(ret, httport);
  ret = mk_rtsp_server_start(rtsport, 0);
  CHECKERR(ret, rtsport);
  ret = mk_rtmp_server_start(rtmport, 0);
  CHECKERR(ret, rtmport);
  mk_set_option("rtc.externIP", publicip);
  mk_set_option("rtc.port", publicport);
  ret = mk_rtc_server_start(localport);
  CHECKERR(ret, localport);

  mk_events events = {0};
  events.on_mk_http_request = on_mk_http_request;
  events.on_mk_http_access = on_mk_http_access;
  events.on_mk_http_before_access = on_mk_http_before_access;

  mk_events_listen(&events);

  mk_media media = mk_media_create("__defaultVhost__", "live", "test", 0, 0, 0);
  CHECKERR((media != nullptr), true);
  // h264的codec
  // mk_media_init_video(media, 0, 0, 0, 0, 2 * 104 * 1024);
  codec_args v_args = {0};
  mk_track v_track = mk_track_create(MKCodecH264, &v_args);
  mk_media_init_track(media, v_track);
  mk_media_init_complete(media);
  mk_track_unref(v_track);

  //创建h264分帧器
  mk_h264_splitter splitter = mk_h264_splitter_create(on_h264_frame, media);

  const int inputsize = 1024;
  char indata[1024];
  while (h264in.read(reinterpret_cast<char *>(indata), inputsize))
  {
    mk_h264_splitter_input_data(splitter, indata, inputsize);
  }

  mk_h264_splitter_release(splitter);
  mk_media_release(media);
  mk_stop_all_server();
  h264in.close();

  return 0;
}