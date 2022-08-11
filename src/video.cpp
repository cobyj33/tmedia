#include <cstdint>
#include <cstdlib>
#include <modes.h>
#include <ascii.h>
#include <ascii_data.h>
#include <ncurses.h>

#include <ascii_constants.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

#define VIDEO_FIFO_MAX_SIZE 200000

#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include <vector>
#include <utility>
#include <mutex>
#include <deque>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>
}


// typedef struct VideoFIFO {
//     VideoFrame* first;
//     VideoFrame* last;
//     VideoFrame* current;
//     int index;
//     int count;
//     double time;
//     int timeBase;
// } VideoFIFO;

// typedef struct VideoFrame {
//     uint8_t dataBuffer[FRAME_DATA_SIZE * 5 / 4];
//     int pts;
//     VideoFrame* next;
// } VideoFrame;

// VideoFrame* get_video_frame(AVFrame* avframe) {
//     VideoFrame* VideoFrame;
// }

// bool add_video_frame(VideoFifo* fifo, VideoFrame frame) {
//     if (fifo->first == NULL) {
//         fifo->first = &frame;
//         fifo->last = &frame;
//     } else {
//         fifo->last->next = &frame;
//         fifo->last = fifo->last->next;
//     }
//     fifo->count += 1;
// }

int get_packet_stats(const char* fileName, int* videoPackets, int* audioPackets);

class VideoState {
    public: 
        AVFormatContext* formatContext = nullptr;
        AVStream* videoStream;
        AVStream* audioStream;
        const AVCodec* audioDecoder = nullptr;
        const AVCodec* videoDecoder = nullptr;
        AVCodecContext* audioCodecContext;
        AVCodecContext* videoCodecContext;

        AVFifo* videoFifo;
        AVAudioFifo* audioFifo;
        SwsContext* videoResizer;
        SwrContext* audioResampler;

        ma_device* audioDevice;
        
        std::mutex videoMutex;
        std::mutex audioMutex;
        std::mutex fetchingMutex;
        std::deque<int> pts;

        int frameCount;
        int availableFrames;
        int currentFrame;

        int currentPacket;
        int totalPackets;
        double time;
        double timeBase;
        double frameRate;

        int frameWidth;
        int frameHeight;
        double loadingStatus;

        bool valid = false;
        bool inUse = false;
        bool allPacketsRead = false;
        bool playing = false;

    public:
        const char* fileName;

        VideoState() { }

        VideoState(const char* fileName) {
            valid = this->init(fileName) == EXIT_SUCCESS ? true : false;
        }

        int init(const char* fileName) {
            int result;

            result = avformat_open_input(&(this->formatContext), fileName, nullptr, nullptr);
            if (result < 0) {
                std::cout << "Could not open file: " << fileName << std::endl;
                return EXIT_FAILURE;
            }

            result = avformat_find_stream_info(this->formatContext, nullptr);
            if (result < 0) {
                std::cout << "Could not get stream info of: " << fileName << std::endl;
                return EXIT_FAILURE;
            }

            int videoStreamIndex = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &(this->videoDecoder), 0);
            if (videoStreamIndex < 0) {
                std::cout << "Could not get video stream of: " << fileName << std::endl; 
                return EXIT_FAILURE;
            }

            this->videoStream = this->formatContext->streams[videoStreamIndex];
            this->timeBase = 1.0 * this->videoStream->time_base.num / this->videoStream->time_base.den;
            this->frameRate = 1.0 * this->videoStream->avg_frame_rate.num * this->videoStream->avg_frame_rate.den;
            
            int audioStreamIndex = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &(this->audioDecoder), 0);
            if (audioStreamIndex < 0) {
                std::cout << "Could not get audio stream of: " << fileName << std::endl;
                return EXIT_FAILURE;
            }

            this->audioStream = this->formatContext->streams[audioStreamIndex];

            this->audioCodecContext = avcodec_alloc_context3(this->audioDecoder);
            this->videoCodecContext = avcodec_alloc_context3(this->videoDecoder);

            result = avcodec_parameters_to_context(this->audioCodecContext, this->audioStream->codecpar);
            if (result < 0) {
                std::cout << "Could not bind audio avcodec params to context" << std::endl;
                return EXIT_FAILURE;
            }

            result = avcodec_parameters_to_context(this->videoCodecContext, this->videoStream->codecpar);
            if (result < 0) {
                std::cout << "Could not bind video avcodec params to context" << std::endl;
                return EXIT_FAILURE;
            }

            result = avcodec_open2(this->audioCodecContext, this->audioDecoder, nullptr);
            if (result < 0) {
                std::cout << "Could not open audio codec context " << std::endl;
                return EXIT_FAILURE;
            }

            result = avcodec_open2(this->videoCodecContext, this->videoDecoder, nullptr);
            if (result < 0) {
                std::cout << "Could not open video codec context " << std::endl;
                return EXIT_FAILURE;
            }

            int frameWidth, frameHeight;
            get_output_size(this->videoCodecContext->width, this->videoCodecContext->height, MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, &frameWidth, &frameHeight);
            this->frameWidth = frameWidth;
            this->frameHeight = frameHeight;

            this->videoResizer = sws_getContext(
                    this->videoCodecContext->width,
                    this->videoCodecContext->height,
                    this->videoCodecContext->pix_fmt, 
                    frameWidth, 
                    frameHeight, 
                    AV_PIX_FMT_GRAY8, 
                    SWS_FAST_BILINEAR, 
                    nullptr, 
                    nullptr, 
                    nullptr);

            AVChannelLayout inputLayout = this->audioStream->codecpar->ch_layout;
            AVChannelLayout outputLayout = inputLayout;
            this->audioResampler = swr_alloc();
            result = swr_alloc_set_opts2(
                &audioResampler,
                &outputLayout,
                AV_SAMPLE_FMT_FLT,
                this->audioStream->codecpar->sample_rate,
                &inputLayout,
                this->audioCodecContext->sample_fmt,
                this->audioStream->codecpar->sample_rate,
                0,
                nullptr);

            if (result < 0) {
                std::cout << "Could not create audio resampler" << std::endl;
                return EXIT_FAILURE;
            }

            result = swr_init(audioResampler);
            if (result < 0) {
                std::cout << "Could not initialize audio resampler" << std::endl;
                return EXIT_FAILURE;
            }

            this->videoFifo = av_fifo_alloc2(1000, frameWidth * frameHeight, AV_FIFO_FLAG_AUTO_GROW);

            if (this->videoFifo == NULL) {
                std::cout << "Could not initialize Video FIFO" << std::endl;
                return EXIT_FAILURE;
            }
            av_fifo_auto_grow_limit(this->videoFifo, VIDEO_FIFO_MAX_SIZE);

            this->audioFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, audioStream->codecpar
        ->ch_layout.nb_channels, 1);

            if (this->audioFifo == NULL) {
                std::cout << "Could not initialize audio FIFO" << std::endl;
                return EXIT_FAILURE;
            }

            int frameCount, audioCount;
            result = get_packet_stats(fileName, &frameCount, &audioCount);
            this->frameCount = frameCount;
            this->totalPackets = frameCount + audioCount;
            if (result < 0) {
                std::cout << "Could not get frame count" << std::endl;
                return EXIT_FAILURE;
            }

            this->fileName = fileName;
            return EXIT_SUCCESS;
        }

        void begin() {
            initscr();

            this->inUse = true;
            this->playing = true;
            std::thread video(videoPlaybackThread, this);
            std::thread audio(audioPlaybackThread, this);
            std::thread bufferLoader(backgroundLoadingThread, this);
            
            // backgroundLoadingThread(this);

            audio.join();
            video.join();
            bufferLoader.join();

            this->inUse = false;
            this->playing = false;
        }

        void end() {
            this->inUse = false;
            this->playing = false;
        }

        private:

        static void videoPlaybackThread(VideoState* state) {
            int result;
            uint8_t dataBuffer[FRAME_DATA_SIZE * 3 / 2];
            int frameCount = 0;
            std::unique_lock<std::mutex> videoLock{state->videoMutex, std::defer_lock};

            std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
            videoLock.lock();
            while (state->inUse && (!state->allPacketsRead || (state->allPacketsRead && av_fifo_can_read(state->videoFifo) > 1))   ) {
                
                if (av_fifo_can_read(state->videoFifo) >= 10) {
                    result = av_fifo_read(state->videoFifo, (void*)dataBuffer, 1);
                    videoLock.unlock();
                } else {
                    while (!state->allPacketsRead && av_fifo_can_read(state->videoFifo) < 10) {
                        if (videoLock.owns_lock()) {
                            videoLock.unlock();
                        }

                        state->fetch_next(10);
                    }

                    videoLock.lock();
                    continue;
                }

                int windowWidth, windowHeight;
                get_window_size(&windowWidth, &windowHeight);
                int outputWidth, outputHeight;
                get_output_size(state->frameWidth, state->frameHeight, windowWidth, windowHeight, &outputWidth, &outputHeight);
                ascii_image asciiImage = get_image(dataBuffer, state->frameWidth, state->frameHeight, outputWidth, outputHeight);
                erase();
                print_ascii_image(&asciiImage);
                refresh();

                double nextFrameTimeSinceStartInSeconds;
                if (state->pts.size() > 0) {
                    nextFrameTimeSinceStartInSeconds = (state->pts.front() * state->timeBase);
                    state->pts.pop_front();
                } else {
                    nextFrameTimeSinceStartInSeconds = 1.0 * state->frameRate;
                }

                // std::cout << "Time base: " << state->videoStream->time_base.num << " / " << state->videoStream->time_base.den << std::endl;
                // std::cout << "Frame Time: " << nextFrameTimeSinceStartInSeconds << std::endl;

                std::chrono::nanoseconds timeElapsed = std::chrono::steady_clock::now() - start_time;
                std::chrono::nanoseconds waitDuration =  std::chrono::milliseconds((int)(nextFrameTimeSinceStartInSeconds * 1000)) - timeElapsed;
                std::this_thread::sleep_for(waitDuration);
                videoLock.lock();
            }


        }


        static void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
            VideoState* state = static_cast<VideoState*>(pDevice->pUserData);
            std::unique_lock<std::mutex> audioLock(state->audioMutex);
            if (state->inUse && state->playing) {
                av_audio_fifo_read(state->audioFifo, &pOutput, std::min((int)frameCount, av_audio_fifo_size(state->audioFifo)));

                while (av_audio_fifo_size(state->audioFifo) < frameCount && !state->allPacketsRead) {
                    if (audioLock.owns_lock()) {
                        audioLock.unlock();
                    }

                    state->fetch_next(10);
                }
            }

            (void)pInput;
        }

        static int audioPlaybackThread(VideoState* state) {
            ma_device_config config = ma_device_config_init(ma_device_type_playback);
            config.playback.format   = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
            config.playback.channels = state->audioStream->codecpar->ch_layout.nb_channels;               // Set to 0 to use the device's native channel count.
            config.sampleRate        = state->audioStream->codecpar->sample_rate;           // Set to 0 to use the device's native sample rate.
            config.dataCallback      = VideoState::audioDataCallback;   // This function will be called when miniaudio needs more data.
            config.pUserData         = state;   // Can be accessed from the device object (device.pUserData).

            ma_result miniAudioLog;
            ma_device audioDevice;
            miniAudioLog = ma_device_init(NULL, &config, &audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                std::cout << "FAILED TO INITIALIZE AUDIO DEVICE: " << miniAudioLog << std::endl;
                return EXIT_FAILURE;  // Failed to initialize the device.
            }

            state->audioDevice = &audioDevice;

            miniAudioLog = ma_device_start(state->audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                std::cout << "Failed to start playback: " << miniAudioLog << std::endl;
                ma_device_uninit(state->audioDevice);
                return EXIT_FAILURE;
            };

            while (ma_device_get_state(state->audioDevice) == ma_device_state_started) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            return EXIT_SUCCESS;
        }

        static void backgroundLoadingThread(VideoState* state) {
            while (!state->allPacketsRead) {
                state->fetch_next(1000);
            }
        }

        public:


        void free() {
            avcodec_free_context(&(this->videoCodecContext));
            avcodec_free_context(&(this->audioCodecContext));
            swr_free(&(this->audioResampler));
            sws_freeContext(this->videoResizer);

            av_fifo_freep2(&(this->videoFifo));
            av_audio_fifo_free(this->audioFifo);

            avformat_free_context(this->formatContext);
            ma_device_uninit(this->audioDevice);
            this->valid = false;
        }

        int fetch_next(int requestedPacketCount) {
            std::unique_lock<std::mutex> videoLock(this->videoMutex, std::defer_lock);
            std::unique_lock<std::mutex> audioLock(this->audioMutex, std::defer_lock);
            std::lock_guard<std::mutex> fetchingLock(this->fetchingMutex);

            AVPacket* readingPacket = av_packet_alloc();
            AVFrame* originalVideoFrame = av_frame_alloc();
            AVFrame* audioFrame = av_frame_alloc();
            int result;

            int videoPacketCount = 0;
            int audioPacketCount = 0;

            void (*cleanup)(AVPacket*, AVFrame*, AVFrame*) = [](AVPacket* packet, AVFrame* videoFrame, AVFrame* audioFrame){
                av_packet_free(&packet);
                av_frame_free(&videoFrame);
                av_frame_free(&audioFrame);
            };

            while (av_read_frame(this->formatContext, readingPacket) == 0) {
                this->currentPacket++;
                if (videoPacketCount >= requestedPacketCount) {
                    cleanup(readingPacket, originalVideoFrame, audioFrame);
                    return EXIT_SUCCESS;
                }
                
                if (readingPacket->stream_index == this->videoStream->index) {
                    result = avcodec_send_packet(this->videoCodecContext, readingPacket);

                    if (result < 0) {
                        if (result == AVERROR(EAGAIN) ) {
                            av_packet_unref(readingPacket);
                            continue;
                        } else (result != AVERROR(EAGAIN) )  ;{
                            std::cout << "Packet reading Error: " << result << std::endl;
                            cleanup(readingPacket, originalVideoFrame, audioFrame);
                            return EXIT_FAILURE;
                        }
                    }

                    result = avcodec_receive_frame(this->videoCodecContext, originalVideoFrame);
                    if (result == AVERROR(EAGAIN)) {
                        av_packet_unref(readingPacket);
                        av_frame_unref(originalVideoFrame);
                        continue;
                    } else if (result < 0) {
                        std::cout << "Frame recieving error" << std::endl;
                        cleanup(readingPacket, originalVideoFrame, audioFrame);
                        return EXIT_FAILURE;
                    } else {
                        AVFrame* resizedVideoFrame = av_frame_alloc();
                        resizedVideoFrame->format = AV_PIX_FMT_GRAY8;
                        resizedVideoFrame->width = this->frameWidth;
                        resizedVideoFrame->height = this->frameHeight;
                        av_frame_get_buffer(resizedVideoFrame, 0);

                        sws_scale(videoResizer, (uint8_t const * const *)originalVideoFrame->data, originalVideoFrame->linesize, 0, this->videoCodecContext->height, resizedVideoFrame->data, resizedVideoFrame->linesize);

                        videoLock.lock();
                        if (av_fifo_can_write(this->videoFifo) < 10) {
                            av_fifo_grow2(this->videoFifo, 10);
                        }

                        if (av_fifo_can_write(this->videoFifo) > 1) {
                            videoPacketCount++;
                            av_fifo_write(this->videoFifo, (void*)resizedVideoFrame->data[0], 1);
                            this->pts.push_back(originalVideoFrame->pts);
                        } 
                        videoLock.unlock();

                        av_frame_free(&resizedVideoFrame);
                        av_frame_unref(originalVideoFrame);
                    }

                } else if (readingPacket->stream_index == this->audioStream->index) {
                    result = avcodec_send_packet(this->audioCodecContext, readingPacket);
                    if (result < 0) {
                        if (result == AVERROR(EAGAIN)) {
                            std::cout << "Resending Packet..." << std::endl;
                            av_packet_unref(readingPacket);
                            continue;
                        } else {
                            std::cout << "Audio Frame Recieving Error " << std::endl;
                            cleanup(readingPacket, originalVideoFrame, audioFrame);
                            return EXIT_FAILURE;
                        }
                    }

                    audioPacketCount++;
                    result = avcodec_receive_frame(this->audioCodecContext, audioFrame);
                    if (result == AVERROR(EAGAIN)) {
                        av_packet_unref(readingPacket);
                        av_frame_unref(audioFrame);
                        continue;
                    } else if (result < 0) {
                        std::cout << "Frame recieving error " << result << std::endl;
                        cleanup(readingPacket, originalVideoFrame, audioFrame);
                        return EXIT_FAILURE;
                    } else {
                        audioLock.lock();
                        while (result == 0) {
                            AVFrame* resampledFrame = av_frame_alloc();
                            resampledFrame->sample_rate = audioFrame->sample_rate;
                            resampledFrame->ch_layout = audioFrame->ch_layout;
                            resampledFrame->format = AV_SAMPLE_FMT_FLT;

                            result = swr_convert_frame(audioResampler, resampledFrame, audioFrame);
                            if (result < 0) {
                                std::cout << "Unable to sample frame" << std::endl;
                            }

                            av_frame_unref(audioFrame);
                            av_audio_fifo_write(this->audioFifo, (void**)resampledFrame->data, resampledFrame->nb_samples);
                            av_frame_free(&resampledFrame);
                            result = avcodec_receive_frame(this->audioCodecContext, audioFrame);
                        }
                        audioLock.unlock();
                    }
                }

                av_packet_unref(readingPacket);
            }

            this->allPacketsRead = true;
            cleanup(readingPacket, originalVideoFrame, audioFrame);
            return EXIT_SUCCESS;
        }

    static void fullyLoad(VideoState* state) {
        const int MAX_PROGESS_BAR_WIDTH = 100;
        int windowWidth, windowHeight;
        char progressBar[MAX_PROGESS_BAR_WIDTH + 1];
        progressBar[MAX_PROGESS_BAR_WIDTH] = '\0';

        double percent;
        while (!state->allPacketsRead) {
            percent = (double)state->currentPacket / state->totalPackets;
            int percentDisplay = (int)(100 * percent);
            // erase();
            for (int i = 0; i < MAX_PROGESS_BAR_WIDTH; i++) {
                if (i < percent * MAX_PROGESS_BAR_WIDTH) {
                    progressBar[i] = '@';
                } else {
                    progressBar[i] = '*';
                }
            }

            erase();
            printw("Generating Frames and Audio: %d of %d: %d%%", state->currentPacket, state->totalPackets, percentDisplay);
            printw(progressBar);
            refresh();
            state->fetch_next(200);
        }
    }

};

int videoProgram(const char* fileName) {
    initscr();
    VideoState* videoState = new VideoState(fileName);
    // videoState->fullyLoad(videoState);
    // videoState->fullyLoad(videoState);c
    // std::thread loader(VideoState::fullyLoad, videoState);
    // loader.join();
    videoState->fetch_next(100);
    videoState->begin();

    videoState->free();
    delete videoState;


    return EXIT_SUCCESS;
}


int get_packet_stats(const char* fileName, int* videoPackets, int* audioPackets) {
    AVFormatContext* formatContext(nullptr);
    int success = EXIT_SUCCESS;

    success = avformat_open_input(&formatContext, fileName, nullptr, nullptr);
    if (success < 0) {
        std::cout << "Could not open file: " << fileName << std::endl;
        return success;
    }

    success = avformat_find_stream_info(formatContext, nullptr);
    if (success < 0) {
        std::cout << "Could not find stream info of " << fileName << std::endl;
        return success;
    }
    
    int videoStreamIndex = -1;
    int audioStreamIndex = -1;
    videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex == AVERROR_STREAM_NOT_FOUND) {
        std::cout << "Could not find videoStream" << std::endl;
    }

    audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex == AVERROR_STREAM_NOT_FOUND) {
        std::cout << "Could not find audioStream" << std::endl;
    }   


    AVPacket* readingPacket = av_packet_alloc();

    int videoPacketCount = 0;
    int audioPacketCount = 0;

    while (av_read_frame(formatContext, readingPacket) == 0) {
        if (readingPacket->stream_index == videoStreamIndex) {
            videoPacketCount++;
        } else if (readingPacket->stream_index == audioStreamIndex) {
            audioPacketCount++;
        }
        av_packet_unref(readingPacket);
    }

    *videoPackets = videoPacketCount;
    *audioPackets = audioPacketCount;
    avformat_free_context(formatContext);
    av_packet_free(&readingPacket);
    return EXIT_SUCCESS;
}