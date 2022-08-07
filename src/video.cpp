#include <modes.h>
#include <ascii.h>
#include <ascii_data.h>
#include <ncurses.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include <vector>
#include <utility>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>
}

const int maxFrameWidth = 640;
const int maxFrameHeight = 480;

int loadAudio(const char* file, bool* endFlag, bool* loaded); 

void frameGenerator(cv::VideoCapture* videoCapture, std::vector<pixel_data>& buffer, int bufferLength, double* percentage) {
    cv::VideoCapture capture = *videoCapture;
    double scaleFactor = 1;
    int originalWidth = capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int originalHeight = capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    if (originalWidth > maxFrameWidth || originalHeight > maxFrameWidth) {
        scaleFactor = std::min((double)maxFrameWidth / originalWidth, (double)maxFrameHeight / originalHeight);
    }

    int videoWidth = (int)(scaleFactor * originalWidth);
    int videoHeight = (int)(scaleFactor * originalHeight);
    cv::Size videoSize(videoWidth, videoHeight);
    
    for (int i = 0; i < bufferLength; i++) {
        cv::Mat readImage;
        cv::Mat resizedImage;
        capture >> readImage;
        if (readImage.rows != videoWidth || readImage.rows != videoHeight) {
            cv::resize(readImage, resizedImage, videoSize, 0, 0, cv::INTER_AREA);
        } else {
            resizedImage = readImage;
        }

        Color pixels[resizedImage.rows * resizedImage.cols];
        *percentage = (double)i / bufferLength;
        get_pixels(&resizedImage, pixels, resizedImage.cols, resizedImage.rows);
        pixel_data data;
        data.pixels = pixels;
        data.width = resizedImage.cols;
        data.height = resizedImage.rows;
        buffer[i] = data;
    }

}

int videoProgram(const char* fileName) {


  cv::VideoCapture capture;
    capture.open(fileName);

  
  if (!capture.isOpened()) {
      std::cout << "Could not open file: " << fileName << std::endl;
    return EXIT_FAILURE;
  }

  initscr();

  int totalFrames = capture.get(cv::CAP_PROP_FRAME_COUNT);
  double targetFPS = capture.get(cv::CAP_PROP_FPS);

  bool audioLoaded = false;
  bool endFlag = false;
  std::thread soundThread(loadAudio, fileName, &endFlag, &audioLoaded);
  while (!audioLoaded) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  int currentFrame = 0;
  std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
  std::chrono::nanoseconds preferredTime = std::chrono::nanoseconds(0);
  std::chrono::nanoseconds actualTime = std::chrono::nanoseconds(0);
  const std::chrono::nanoseconds preferredTimePerFrame = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds((int)(1000 / targetFPS)));

    cv::Size frameSize(maxFrameWidth, maxFrameHeight);
  while (currentFrame < totalFrames) {
    cv::Mat originalFrame;
    cv::Mat frame;
    capture >> originalFrame;
    
    get_output_size(originalFrame.cols, originalFrame.rows, &(frameSize.width), &(frameSize.height));
    if (originalFrame.rows > frameSize.height && originalFrame.cols > frameSize.width) {
        cv::resize(originalFrame, frame, frameSize, 0, 0, cv::INTER_AREA);
    } else {
        frame = originalFrame;
    }
    
    Color pixels[frame.rows * frame.cols];
    get_pixels(&frame, pixels, frame.cols, frame.rows);
    int outputWidth, outputHeight;
    get_output_size(frame.cols, frame.rows, &outputWidth, &outputHeight);
    ascii_image asciiImage = get_image(pixels, frame.cols, frame.rows, outputWidth, outputHeight);
    print_ascii_image(&asciiImage);
    refresh();
    

    preferredTime = preferredTime + preferredTimePerFrame;
    actualTime = std::chrono::steady_clock::now() - start_time;
    auto diff = actualTime - preferredTime;
    auto delay = preferredTimePerFrame - diff;
    std::this_thread::sleep_for(delay);
    refresh();
    currentFrame++;
  }

  endFlag = true;
  std::this_thread::sleep_for(std::chrono::seconds(1));

  capture.release();
  return EXIT_SUCCESS;
}

typedef struct AudioOutputData {
    AVAudioFifo* fifo;
    bool* interrupted;
} AudioOutputData;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    AudioOutputData* data = reinterpret_cast<AudioOutputData*>(pDevice->pUserData);

    if (*(data->interrupted) == false) {
        if (av_audio_fifo_size(data->fifo) > 0) {
            av_audio_fifo_read(data->fifo, &pOutput, frameCount);
        }
    }

    (void)pInput;
}

int loadAudio(const char* file, bool* endFlag, bool* loaded) {
   
    AVFormatContext* pFormatContext = NULL;
    
    int result = avformat_open_input(&pFormatContext, file, nullptr, nullptr);
    if (result != 0) {
        std::cout << "Could not open file" << std::endl;
        return EXIT_FAILURE;
    }

    result = avformat_find_stream_info(pFormatContext, nullptr);
    if (result < 0) {
        std::cout << "Unable to find stream info" << std::endl;
        return EXIT_FAILURE;
    }
    
    
    AVCodec* decoder(nullptr);
    int audioIndex = av_find_best_stream(pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
    if (audioIndex < 0) {
        std::cout << "Could not find audio stream" << std::endl;
        return EXIT_FAILURE;
    }

    if (!decoder) {
        std::cout << "No decoder found" << std::endl;
        return EXIT_FAILURE;
    }

    AVStream* stream = pFormatContext->streams[audioIndex];
    av_dump_format(pFormatContext, 0, nullptr, 0);
    AVCodecContext* audioCodecContext = avcodec_alloc_context3(decoder);

    avcodec_parameters_to_context(audioCodecContext, stream->codecpar);

    result = avcodec_open2(audioCodecContext, decoder, nullptr);
    if (result < 0) {
        std::cout << "Could not open decoder" << std::endl;
        return EXIT_FAILURE;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    SwrContext* resampler{swr_alloc_set_opts(nullptr,
            stream->codecpar->channel_layout,
            AV_SAMPLE_FMT_FLT,
            stream->codecpar->sample_rate,
            stream->codecpar->channel_layout,
            (AVSampleFormat)stream->codecpar->format,
            stream->codecpar->sample_rate,
            0, 
            nullptr)};


    AVAudioFifo* fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, stream->codecpar->channels, 1);    

    while (av_read_frame(pFormatContext, packet) == 0) {
        if (packet->stream_index != audioIndex) { continue; }
        result = avcodec_send_packet(audioCodecContext, packet);
        if (result < 0) {
            if (result != AVERROR(EAGAIN)) {
                std::cout << "Decoding Error" << std::endl;
                return EXIT_FAILURE;
            }
        }
    
        result = avcodec_receive_frame(audioCodecContext, frame);
        while (result == 0) {
            //Resample frame
            AVFrame* resampledFrame = av_frame_alloc();
            resampledFrame->sample_rate = frame->sample_rate;
            resampledFrame->channel_layout = frame->channel_layout;
            resampledFrame->channels = frame->channels;
            resampledFrame->format = AV_SAMPLE_FMT_FLT;

            result = swr_convert_frame(resampler, resampledFrame, frame);
            if (result < 0) {
                std::cout << "Unable to sample frame" << std::endl;
            }

            av_frame_unref(frame);
            av_audio_fifo_write(fifo, (void**)resampledFrame->data, resampledFrame->nb_samples);
            av_frame_free(&resampledFrame);
            result = avcodec_receive_frame(audioCodecContext, frame);
        }
        /* av_packet_unref(packet); */
    }


     AudioOutputData data;
     data.fifo = fifo;
     data.interrupted = endFlag;


    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = stream->codecpar->channels;               // Set to 0 to use the device's native channel count.
    config.sampleRate        = stream->codecpar->sample_rate;           // Set to 0 to use the device's native sample rate.
    config.dataCallback      = data_callback;   // This function will be called when miniaudio needs more data.
    config.pUserData         = &data;   // Can be accessed from the device object (device.pUserData).



    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }


    if (ma_device_start(&device) != MA_SUCCESS) {
        std::cout << "Failed to start playback" << std::endl;
        ma_device_uninit(&device);
        return EXIT_FAILURE;
    };
    *loaded = true;
    
    while (*(data.interrupted) == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    ma_device_uninit(&device);
    
    avformat_close_input(&pFormatContext);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&audioCodecContext);
    swr_free(&resampler);
    av_audio_fifo_free(fifo);

    return EXIT_SUCCESS;
}
