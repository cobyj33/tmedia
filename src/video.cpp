#include <cstdint>
#include <cstdlib>
#include <video.h>
#include <ascii.h>
#include <ascii_data.h>
#include <ncurses.h>

#include <ascii_constants.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

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


class VideoState {
    public: 
        //Initialized in Init
        AVFormatContext* formatContext = nullptr;
        AVStream* videoStream;
        AVStream* audioStream;
        const AVCodec* audioDecoder = nullptr;
        const AVCodec* videoDecoder = nullptr;
        AVCodecContext* audioCodecContext = nullptr;
        AVCodecContext* videoCodecContext = nullptr;
        AVFifo* videoFifo;
        AVAudioFifo* audioFifo;
        SwsContext* videoResizer;
        SwrContext* audioResampler;

        //Initialized
        ma_device* audioDevice;
        
        std::mutex videoMutex;
        std::mutex audioMutex;
        std::mutex fetchingMutex;
        std::mutex flagMutex;
        std::mutex symbolMutex;

        //initialized in Init
        std::deque<int64_t> pts;
        int frameCount;
        int totalPackets;
        int frameWidth;
        int frameHeight;
        double timeBase;
        double frameRate;
        bool valid = false;

        //sets on Begin and End
        bool inUse = false;

        Playback* playback;

        ascii_image lastImage;
        std::vector<VideoSymbol> symbols;

        const char* fileName;

        VideoState() { }

        VideoState(const char* fileName) {
            this->init(fileName);
        }

        void reset() {
            this->free_video_state();
            this->init(this->fileName);
            this->valid = this->init(fileName) == EXIT_SUCCESS ? true : false;
        }

        int init(const char* fileName) {
            int result;

            this->playback = (Playback*)malloc(sizeof(Playback));

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
            
            this->videoFifo = av_fifo_alloc2(5000, frameWidth * frameHeight, AV_FIFO_FLAG_AUTO_GROW);

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
            this->valid = true;
            return EXIT_SUCCESS;
        }

        void begin() {
            if (this->valid == false) {
                std::cout << "CANNOT BEGIN INVALID VIDEO STATE" << std::endl;
                return;
            }

            this->inUse = true;
            this->playback->playing = true;
            std::thread video(videoPlaybackThread, this);
            std::thread audio(audioPlaybackThread, this);
            std::thread bufferLoader(backgroundLoadingThread, this);
            std::thread inputThread(inputListeningThread, this);
            
            // backgroundLoadingThread(this);

            audio.join();
            video.join();
            bufferLoader.join();
            inputThread.join();

            this->inUse = false;
            this->playback->playing = false;
        }

        void end() {
            this->inUse = false;
            this->free_video_state();
        }

        private:

        static void videoPlaybackThread(VideoState* state) {
            // ascii_image testASCII;
            // testASCII.width = 10;
            // testASCII.height = 10;
            // for (int row = 0; row < 10; row++) {
            //     for (int col = 0; col < 10; col++) {
            //         testASCII.lines[row][col] = '%';
            //     }
            // }


            int result;
            uint8_t dataBuffer[FRAME_DATA_SIZE * 3 / 2];
            int frameCount = 0;
            std::unique_lock<std::mutex> videoLock{state->videoMutex, std::defer_lock};
            std::unique_lock<std::mutex> symbolLock{state->symbolMutex, std::defer_lock};

            std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
            std::chrono::nanoseconds time_paused = std::chrono::nanoseconds(0);
            videoLock.lock();
            while (state->inUse && (!state->playback->allPacketsRead || (state->playback->allPacketsRead && av_fifo_can_read(state->videoFifo) > 1))   ) {
                
                if (state->playback->playing == false) {
                    std::chrono::steady_clock::time_point pauseTime = std::chrono::steady_clock::now();
                    while (state->playback->playing == false) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        if (!state->inUse) {
                            return;
                        }
                        
                        erase();
                        symbolLock.lock();
                        for (int i = 0; i < state->symbols.size(); i++) {
                            int symbolOutputWidth, symbolOutputHeight;
                            get_output_size(state->symbols[i].pixelData->width, state->symbols[i].pixelData->height, state->lastImage.width, state->lastImage.height, &symbolOutputWidth, &symbolOutputHeight);
                            ascii_image symbolImage = get_image(state->symbols[i].pixelData->pixels, state->symbols[i].pixelData->width, state->symbols[i].pixelData->height, symbolOutputWidth, symbolOutputHeight);
                            overlap_ascii_images(&state->lastImage, &symbolImage);
                        }
                        symbolLock.unlock();

                        print_ascii_image(&state->lastImage);
                        refresh();

                    }
                    time_paused = time_paused + (std::chrono::steady_clock::now() - pauseTime);
                }


                if (av_fifo_can_read(state->videoFifo) >= 10) {
                    result = av_fifo_read(state->videoFifo, (void*)dataBuffer, 1);
                    videoLock.unlock();
                } else {
                    while (!state->playback->allPacketsRead && av_fifo_can_read(state->videoFifo) < 10) {
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
                erase();
                    ascii_image asciiImage = get_image(dataBuffer, state->frameWidth, state->frameHeight, outputWidth, outputHeight);
                    // exit(EXIT_SUCCESS); 
                    // asciiImage.height = std::max(1, asciiImage.height - 4);
                    // printw("Frame Width: %d Frame Height: %d Window Width: %d Window Height: %d Output Width: %d Output Height: %d \n", state->frameWidth, state->frameHeight, windowWidth, windowHeight, outputWidth, outputHeight);
                    // printw("ASCII Width: %d ASCII Height: %d\n", asciiImage.width, asciiImage.height);
                    // printw("Packets: %d of %d", state->playback->currentPacket, state->playback->allPacketsRead);
                    // printw("Playing: %s", state->playback->playing ? "true" : "false");
                    
                    symbolLock.lock();
                    for (int i = 0; i < state->symbols.size(); i++) {
                        int symbolOutputWidth, symbolOutputHeight;
                        get_output_size(state->symbols[i].pixelData->width, state->symbols[i].pixelData->height, asciiImage.width, asciiImage.height, &symbolOutputWidth, &symbolOutputHeight);
                        ascii_image symbolImage = get_image(state->symbols[i].pixelData->pixels, state->symbols[i].pixelData->width, state->symbols[i].pixelData->height, symbolOutputWidth, symbolOutputHeight);
                        overlap_ascii_images(&asciiImage, &symbolImage);
                        state->symbols[i].framesShown++;
                        if (state->symbols[i].framesShown >= state->symbols[i].framesToDelete) {
                            state->symbols.erase(state->symbols.begin() + i);
                        }
                    }
                    symbolLock.unlock();
                    // overlap_ascii_images(&asciiImage, &testASCII);

                    print_ascii_image(&asciiImage);
                    // for (int i = 0; i < asciiImage.height && i < windowHeight; i++) {
                    //     for (int j = 0; j < asciiImage.width && j < windowWidth; j++) {
                    //         addch(asciiImage.lines[i][j]);
                    //     }
                    //     addch('\n');
                    // }
                refresh();

                double nextFrameTimeSinceStartInSeconds;
                if (state->pts.size() > 0) {
                    nextFrameTimeSinceStartInSeconds = (state->pts.front() * state->timeBase);
                    state->playback->currentPTS = state->pts.front();
                    state->pts.pop_front();
                } else {
                    nextFrameTimeSinceStartInSeconds = 1.0 / state->frameRate;
                    // nextFrameTimeSinceStartInSeconds = 1.0;
                }

                // std::cout << "Time base: " << state->videoStream->time_base.num << " / " << state->videoStream->time_base.den << std::endl;
                // std::cout << "Frame Time: " << nextFrameTimeSinceStartInSeconds << std::endl;

                std::chrono::nanoseconds timeElapsed = std::chrono::steady_clock::now() - start_time - time_paused + std::chrono::nanoseconds((int64_t)(state->playback->skippedPTS * state->timeBase * SECONDS_TO_NANOSECONDS));
                std::chrono::nanoseconds waitDuration =  std::chrono::nanoseconds((int64_t)(nextFrameTimeSinceStartInSeconds * SECONDS_TO_NANOSECONDS)) - timeElapsed;
                state->lastImage = asciiImage;
                std::this_thread::sleep_for(waitDuration);
                videoLock.lock();
            }

        }


        static void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
            VideoState* state = static_cast<VideoState*>(pDevice->pUserData);
            std::unique_lock<std::mutex> audioLock(state->audioMutex);
            if (state->inUse && state->playback->playing) {
                av_audio_fifo_read(state->audioFifo, &pOutput, std::min((int)frameCount, av_audio_fifo_size(state->audioFifo)));

                while (av_audio_fifo_size(state->audioFifo) < frameCount && !state->playback->allPacketsRead) {
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
            // miniAudioLog = ma_device_start(state->audioDevice);
            // if (miniAudioLog != MA_SUCCESS) {
            //     std::cout << "Failed to start playback: " << miniAudioLog << std::endl;
            //     ma_device_uninit(state->audioDevice);
            //     return EXIT_FAILURE;
            // };

            while (state->inUse) {
                if (state->playback->playing == false && ma_device_get_state(state->audioDevice) == ma_device_state_started) {
                    miniAudioLog = ma_device_stop(state->audioDevice);
                    if (miniAudioLog != MA_SUCCESS) {
                        std::cout << "Failed to stop playback: " << miniAudioLog << std::endl;
                        ma_device_uninit(state->audioDevice);
                        return EXIT_FAILURE;
                    };
                } else if (state->playback->playing && ma_device_get_state(state->audioDevice) == ma_device_state_stopped) {
                    miniAudioLog = ma_device_start(state->audioDevice);
                    if (miniAudioLog != MA_SUCCESS) {
                        std::cout << "Failed to start playback: " << miniAudioLog << std::endl;
                        ma_device_uninit(state->audioDevice);
                        return EXIT_FAILURE;
                    };
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            return EXIT_SUCCESS;
        }

        static void backgroundLoadingThread(VideoState* state) {
            while (!state->playback->allPacketsRead && state->inUse) {
                if (av_fifo_can_write(state->videoFifo) > 100 && av_audio_fifo_space(state->audioFifo) > 100) {
                    state->fetch_next(100);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        static void inputListeningThread(VideoState* state) {
            std::unique_lock<std::mutex> symbolLock(state->symbolMutex, std::defer_lock);
            while (state->inUse) {

                int ch = getch();

                symbolLock.lock();
                if (ch == (int)(' ')) {
                    if (state->playback->playing) {
                        state->symbols.push_back(get_video_symbol(PAUSE_ICON));
                        state->pause();
                    } else {
                        state->symbols.push_back(get_video_symbol(PLAY_ICON));
                        state->restart();
                    }
                } else if (ch == KEY_LEFT) {
                    if (state->pts.size() > 0) {
                        state->symbols.push_back(get_video_symbol(BACKWARD_ICON));
                        state->jumpToTime((int)(state->pts.front() - ( 5 / state->timeBase )  ));
                    }
                } else if (ch == KEY_RIGHT) {
                    if (state->pts.size() > 0) {
                        state->symbols.push_back(get_video_symbol(FORWARD_ICON));
                        state->jumpToTime((int)(state->pts.front() + ( 5 / state->timeBase )  ));
                    }
                } else if (ch == KEY_UP) {
                    if (state->audioDevice != nullptr) {
                        state->audioDevice->masterVolumeFactor = std::min(1.0, state->audioDevice->masterVolumeFactor + 0.05);
                        state->symbols.push_back(get_symbol_from_volume(state->audioDevice->masterVolumeFactor));
                    }
                } else if (ch == KEY_DOWN) {
                    if (state->audioDevice != nullptr) {
                        state->audioDevice->masterVolumeFactor = std::max(0.0, state->audioDevice->masterVolumeFactor - 0.05);
                        state->symbols.push_back(get_symbol_from_volume(state->audioDevice->masterVolumeFactor));
                    }
                }
                symbolLock.unlock();
            
            }
        }

        public:


        void free_video_state() {
            avcodec_free_context(&(this->videoCodecContext));
            avcodec_free_context(&(this->audioCodecContext));
            swr_free(&(this->audioResampler));
            sws_freeContext(this->videoResizer);

            av_fifo_freep2(&(this->videoFifo));
            av_audio_fifo_free(this->audioFifo);

            avformat_free_context(this->formatContext);
            ma_device_uninit(this->audioDevice);
            
            free(this->playback);
            pts.clear();

            this->valid = false;
        }

        void pause() {
            if (this->inUse) {
                this->playback->playing = false;
            }
        }

        void restart() {
            if (this->inUse) {
                this->playback->playing = true;
            }
        }

        void jumpToTime(int pts) {
            std::lock_guard<std::mutex> videoLock(this->videoMutex);
            std::lock_guard<std::mutex> audioLock(this->audioMutex);

            if (pts > this->playback->currentPTS) {
                erase();
                printw("Current PTS: %d, Next PTS: %d", this->playback->currentPTS, pts);
                refresh();

                int framesToJump = 0;
                while (this->pts.size() > 0) {
                    if (this->pts.front() < pts) {
                        framesToJump++;
                        this->pts.pop_front();
                    } else {
                        break;
                    }
                }

                double secondsJumped = (pts - this->playback->currentPTS) * this->timeBase;

                av_fifo_drain2(this->videoFifo, std::min((int)av_fifo_can_read(this->videoFifo), framesToJump) );
                av_audio_fifo_drain(this->audioFifo, (int)(secondsJumped * this->audioCodecContext->sample_rate));
                this->playback->skippedPTS += pts - this->playback->currentPTS;

                this->playback->currentPTS = this->pts.size() > 0 ? this->pts.front() : pts;
            }
            // AVPacket* jumpingPacket = av_packet_alloc();
            // bool lastPlayingState = this->playback->playing;
            // int currentPTS = this->playback->currentPTS;
            // int lastPTS = this->playback->currentPTS;
            // std::unique_lock<std::mutex> symbolLock(this->symbolMutex, std::defer_lock);
        
            // if (pts < this->playback->currentPTS) {
            //     this->free_video_state();
            //     this->init(this->fileName);
            //     if (!this->valid) {
            //         std::cout << "COULD NOT REINITIALIZE VIDEO STATE ON JUMPING TO TIME" << std::endl;
            //         this->free_video_state();
            //         exit(EXIT_SUCCESS);
            //     }
            //     if (pts <= 0) {
            //         goto cleanup;
            //     }
            // }


            // while (av_read_frame(this->formatContext, jumpingPacket) == 0) { //TODO: CHECK PACKET ERROR
            //     this->playback->currentPacket++;
            //     if (jumpingPacket->stream_index == this->videoStream->index) {
            //         currentPTS = jumpingPacket->pts;
            //         if (currentPTS == pts || (lastPTS >= pts && currentPTS <= pts)) {
            //             goto cleanup;
            //         }
            //         lastPTS = currentPTS;
            //     }
            //     av_packet_unref(jumpingPacket);
            // }

            // this->playback->allPacketsRead = true;
            // cleanup:
            //     this->playback->playing = lastPlayingState;
            //     this->playback->currentPTS = currentPTS;
            //     av_packet_free(&jumpingPacket);
        }

        int fetch_next(int requestedPacketCount) {
            if (this->valid == false) {
                std::cout << "CANNOT FETCH FROM INVALID VIDEO STATE" << std::endl;
                return EXIT_FAILURE;
            }
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
                this->playback->currentPacket++;
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
                        av_frame_get_buffer(resizedVideoFrame, 1); //watch this alignment

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

                            av_audio_fifo_write(this->audioFifo, (void**)resampledFrame->data, resampledFrame->nb_samples);
                            av_frame_unref(audioFrame);
                            av_frame_free(&resampledFrame);
                            result = avcodec_receive_frame(this->audioCodecContext, audioFrame);
                        }
                        audioLock.unlock();
                    }
                }

                av_packet_unref(readingPacket);
            }

            this->playback->allPacketsRead = true;
            cleanup(readingPacket, originalVideoFrame, audioFrame);
            return EXIT_SUCCESS;
        }

    static void fullyLoad(VideoState* state) {
        const int MAX_PROGESS_BAR_WIDTH = 100;
        int windowWidth, windowHeight;
        char progressBar[MAX_PROGESS_BAR_WIDTH + 1];
        progressBar[MAX_PROGESS_BAR_WIDTH] = '\0';

        double percent;
        while (!state->playback->allPacketsRead) {
            percent = (double)state->playback->currentPacket / state->totalPackets;
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
            printw("Generating Frames and Audio: %d of %d: %d%%", state->playback->currentPacket, state->totalPackets, percentDisplay);
            printw("%s", progressBar);
            refresh();
            state->fetch_next(200);
        }
    }
};

int videoProgram(const char* fileName) {
    init_icons();
    VideoState* videoState = new VideoState(fileName);

    if (videoState->valid) {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, true);
        videoState->fetch_next(1000);
        videoState->begin();
    } else {
        std::cout << "INVALID VIDEO STATE... EXITING" << std::endl;
    }
    
    videoState->free_video_state();
    delete videoState;
    free_icons();
    endwin();

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
