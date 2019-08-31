
#include <iostream>

#include <jni.h>

#include "FFMpegVideoPlayer.h"
#include "../Helper/Time.h"
#include "../Helper/CPUPriorities.h"

#define TAG "FFMpegVideoPlaeyr.cpp"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)

static int g_shutdown_callback(void *d) {
  return reinterpret_cast<FFMpegVideoPlayer*>(d)->shutdown_callback();
}

FFMpegVideoPlayer::FFMpegVideoPlayer(std::string url, int cpu_priority,
				     std::function<void(uint8_t[],int)> callback, uint32_t bufsize) :
  m_url(url), m_cpu_priority(cpu_priority), m_bufsize(bufsize), m_shutdown(false),
  m_callback(callback) {

  // Allocate the packet
  av_init_packet(&m_packet);

  av_register_all();

  // Register the network interface
  avformat_network_init();
}

void FFMpegVideoPlayer::start_playing() {
  m_shutdown = false;
  m_thread.reset(new std::thread([this] { this->run(); }));
}

void FFMpegVideoPlayer::stop_playing() {
  m_shutdown = true;
  if (m_thread) {
    m_thread->join();
    m_thread.reset();
  }
}

void FFMpegVideoPlayer::run() {
  LOGV("FFMPeg playing: %s", m_url.c_str());

  // Add a callback that we can use to shutdown.
  m_format_ctx = avformat_alloc_context();
  m_format_ctx->interrupt_callback.callback = g_shutdown_callback;
  m_format_ctx->interrupt_callback.opaque = this;

  // Configure some options for the input stream.
  AVDictionary *opts = 0;
  //av_dict_set(&opts, "rtsp_transport", "tcp", 0);
  // Timeout in usec.
  av_dict_set_int(&opts, "stimeout", 1000000, 0);
  av_dict_set_int(&opts, "rw_timeout", 1000000, 0);
  av_dict_set_int(&opts, "reorder_queue_size", 1, 0);
  av_dict_set(&opts, "rtsp_transport", "tcp", 0);

  // Open the input stream.
  if (avformat_open_input(&m_format_ctx, m_url.c_str(), NULL, &opts) != 0) {
    LOGV("Error connecting to: %s", m_url.c_str());
    return;
  }

  // Start reading packets from stream
  if (avformat_find_stream_info(m_format_ctx, 0) < 0) {
    LOGV("Can't find video stream info");
    return;
  }

  // Find the video stream.
  m_video_stream = av_find_best_stream(m_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (m_video_stream < 0) {
    LOGV("Can't find video stream in input file");
    return;
  }

  AVCodecParameters *origin_par = m_format_ctx->streams[m_video_stream]->codecpar;

  // Find the decoder
  if (!(m_codec = avcodec_find_decoder(origin_par->codec_id))) {
    LOGV("Error finding the decoder");
    return;
  }

  // Allocate the context
  if (!(m_codec_ctx = avcodec_alloc_context3(m_codec))) {
    LOGV("Error opening the decoding context");
    return;
  }

  if (avcodec_parameters_to_context(m_codec_ctx, origin_par)) {
    LOGV("Error copying the decoder context");
    return;
  }

  // Open the codec
  if (avcodec_open2(m_codec_ctx, m_codec, 0) < 0) {
    LOGV("Error opening the decoding codec");
    return;
  }

  while (av_read_frame(m_format_ctx, &m_packet) >= 0) {
    if (m_shutdown) {
      break;
    }
    if (m_packet.stream_index == m_video_stream) {
      LOGV("Read packet: %d", m_packet.size);
      m_callback(m_packet.data, m_packet.size);
    }
  }

  // Close the codec context
  if (avcodec_close(m_codec_ctx) != 0) {
    LOGV("Error closing the AV codec context");
    return;
  }
  av_free(m_codec_ctx);

  // Close the format context
  avformat_close_input(&m_format_ctx);
}

int FFMpegVideoPlayer::shutdown_callback() {
  if (m_shutdown) {
    LOGV("Shutting down");
    return 1;
  }
  return 0;
}
