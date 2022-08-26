#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <icons.h>
#include <libavutil/channel_layout.h>
#include <video.h>
#include <ascii.h>
#include <ascii_data.h>
#include <doublelinkedlist.hpp>
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
#include <libavutil/samplefmt.h>
#include <libavfilter/avfilter.h>
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
        AVFifo* audioFrameFifo;

        DoubleLinkedList<VideoFrame*>* videoFrames;
        DoubleLinkedList<AVFrame*>* audioFrames;
        AVAudioFifo* tempAudio;

        SwsContext* videoResizer;
        SwrContext* audioResampler;
        SwrContext* inTimeAudioResampler;

        //Initialized
        ma_device* audioDevice;
        
        std::mutex videoMutex;
        std::mutex audioMutex;
        std::mutex fetchingMutex;
        std::mutex symbolMutex;
        std::mutex playbackMutex;
        std::mutex displaySettingsMutex;

        std::condition_variable playingChanged;

        //initialized in Init
        int frameCount;
        int totalPackets;
        int frameWidth;
        int frameHeight;
        double videoTimeBase;
        double audioTimeBase;
        int audioSampleRate;
        double frameRate;
        bool valid = false;

        //sets on Begin and End
        bool inUse = false;

        Playback* playback;
        VideoDisplaySettings displaySettings;

        const static int nb_displayModes = 4;
        const static VideoStateDisplayMode displayModes[nb_displayModes];
        int displayModeIndex = 0;

        ascii_image lastImage;
        std::vector<VideoSymbol> symbols;
        std::chrono::steady_clock::time_point timeOfLastTimeChange;
        const static std::chrono::milliseconds timeChangeWait;
        const static std::chrono::milliseconds playbackSpeedChangeWait;

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

            this->playback = playback_alloc();
            this->timeOfLastTimeChange = std::chrono::steady_clock::now();

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
            this->videoTimeBase = av_q2d(this->videoStream->time_base);
            this->frameRate = av_q2d(this->videoStream->avg_frame_rate);
            
            int audioStreamIndex = av_find_best_stream(this->formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &(this->audioDecoder), 0);
            if (audioStreamIndex < 0) {
                std::cout << "Could not get audio stream of: " << fileName << std::endl;
                return EXIT_FAILURE;
            }

            this->audioStream = this->formatContext->streams[audioStreamIndex];
            this->audioTimeBase = av_q2d(this->audioStream->time_base);

            std::cout << "Time Bases: Video: " << this->videoStream->time_base.num  << " / " << this->videoStream->time_base.den << "  |  Audio: " << this->audioStream->time_base.num << " / " << this->audioStream->time_base.den << std::endl;
            // exit(EXIT_SUCCESS); 

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

            if (this->audioResampler == NULL) {
                std::cerr << "Could not allocate audio resampler" << std::endl;
                return EXIT_FAILURE;
            }

            this->inTimeAudioResampler = swr_alloc();
            if (this->inTimeAudioResampler == NULL) {
                std::cerr << "Could not allocate in time audio resampler" << std::endl;
                return EXIT_FAILURE;
            }

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
            
            /* this->videoFrames.reserve(FRAME_RESERVE_SIZE); */
            /* this->audioFrames.reserve(FRAME_RESERVE_SIZE); */
            
            this->videoFrames = new DoubleLinkedList<VideoFrame*>();
            this->audioFrames = new DoubleLinkedList<AVFrame*>();

            this->tempAudio = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, this->audioStream->codecpar->ch_layout.nb_channels, FRAME_RESERVE_SIZE);
            if (this->tempAudio == NULL) {
                std::cout << "Could not initialize temp audio fifo" << std::endl;
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
            int result;
            bool debug, movie;
            int frameCount = 0;
            std::unique_lock<std::mutex> videoLock{state->videoMutex, std::defer_lock};
            std::unique_lock<std::mutex> symbolLock{state->symbolMutex, std::defer_lock};
            std::unique_lock<std::mutex> playbackLock{state->playbackMutex, std::defer_lock};

            std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
            std::chrono::nanoseconds time_paused = std::chrono::nanoseconds(0);
            std::chrono::nanoseconds speed_skipped_time = std::chrono::nanoseconds(0);
            VideoFrame* readingFrame = (VideoFrame*)malloc(sizeof(VideoFrame));
        
            int64_t counter = 0;
            while (state->inUse && (!state->playback->allPacketsRead || (state->playback->allPacketsRead && state->videoFrames->get_length() > 0 ) )) {
                debug = state->displaySettings.mode == VIDEO_DEBUG;
                movie = state->displaySettings.mode == MOVIE;
                counter++;
                if (state->playback->playing == false) {

                    std::chrono::steady_clock::time_point pauseTime = std::chrono::steady_clock::now();
                    while (state->playback->playing == false) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        if (!state->inUse) {
                            return;
                        }
                        
                        if (debug || movie) {
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

                    }
                    time_paused = time_paused + (std::chrono::steady_clock::now() - pauseTime);

                }

                videoLock.lock();
                playbackLock.lock();
                if (state->videoFrames->get_index() + 10 <= state->videoFrames->get_length()) {
                    readingFrame = state->videoFrames->get();
                    if (state->videoFrames->get_index() < state->videoFrames->get_length() - 1) {
                        state->videoFrames->set_index(state->videoFrames->get_index() + 1);
                    }
                } else {
                    while (!state->playback->allPacketsRead && state->videoFrames->get_index() + 10 >= state->videoFrames->get_length()) {
                        videoLock.unlock();
                        playbackLock.unlock();
                            state->fetch_next(10);
                        playbackLock.lock();
                        videoLock.lock();
                    }
                    playbackLock.unlock();
                    videoLock.unlock();
                    continue;
                }


                if (debug || movie) {
                    int windowWidth, windowHeight;
                    get_window_size(&windowWidth, &windowHeight);
                    int outputWidth, outputHeight;
                    get_output_size(readingFrame->pixelData->width, readingFrame->pixelData->height, windowWidth, debug ? std::max(windowHeight - 12, 2) : windowHeight, &outputWidth, &outputHeight);

                    erase();
                        ascii_image asciiImage = get_image(readingFrame->pixelData->pixels, readingFrame->pixelData->width, readingFrame->pixelData->height, outputWidth, outputHeight);
                        if (debug) {
                            printw("Counter: %ld\n", counter);
                            printw("Window Size: %d x %d", windowWidth, windowHeight);
                            printw("Frame Width: %d Frame Height: %d Window Width: %d Window Height: %d Output Width: %d Output Height: %d \n", state->frameWidth, state->frameHeight, windowWidth, windowHeight, outputWidth, outputHeight);
                            printw("ASCII Width: %d ASCII Height: %d\n", asciiImage.width, asciiImage.height);
                            printw("Packets: %d of %d\n", state->playback->currentPacket, state->totalPackets);
                            printw("Playing: %s \n", state->playback->playing ? "true" : "false");
                            printw("PTS: %ld\n Repeat-Pict: %d \n Skipped PTS: %ld\n", readingFrame->pts, readingFrame->repeat_pict, state->playback->skippedPTS);
                            printw("Pixel Data: Width: %d Height %d\n", readingFrame->pixelData->width, readingFrame->pixelData->height);
                            printw("Video List Length: %d, Audio List Length: %d\n", state->videoFrames->get_length(), state->audioFrames->get_length() );
                            printw("Video List Index: %d, Audio List Index: %d\n", state->videoFrames->get_index(), state->audioFrames->get_index());
                        }
                        
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

                        if (debug) {
                            for (int i = 0; i < asciiImage.height && i < windowHeight; i++) {
                                for (int j = 0; j < asciiImage.width && j < windowWidth; j++) {
                                    addch(asciiImage.lines[i][j]);
                                }
                                addch('\n');
                            }
                        } else if (movie) {
                            print_ascii_image(&asciiImage);
                        }

                        // overlap_ascii_images(&asciiImage, &testASCII);
                    refresh();
                    state->lastImage = asciiImage;
                }

                double nextFrameTimeSinceStartInSeconds;
                nextFrameTimeSinceStartInSeconds = readingFrame->pts * state->videoTimeBase;
                state->playback->time = readingFrame->pts * state->videoTimeBase;

                std::chrono::nanoseconds skipped_time = std::chrono::nanoseconds((int64_t)(state->playback->skippedPTS * state->videoTimeBase * SECONDS_TO_NANOSECONDS));
                std::chrono::nanoseconds frame_speed_skip_time = std::chrono::nanoseconds( (int64_t) ( ( (readingFrame->duration * state->videoTimeBase) - (readingFrame->duration * state->videoTimeBase) / state->playback->speed ) * SECONDS_TO_NANOSECONDS ) ); ;
                speed_skipped_time += frame_speed_skip_time; 


                //TODO: Check if adding speed_skipped_time actually does anything (since nextFrameTimeSinceStartInSeconds is altered by state->playback->speed)
                std::chrono::nanoseconds timeElapsed = std::chrono::steady_clock::now() - start_time - time_paused + skipped_time + speed_skipped_time; 
                std::chrono::nanoseconds timeOfNextFrame = std::chrono::nanoseconds((int64_t)(nextFrameTimeSinceStartInSeconds * SECONDS_TO_NANOSECONDS));
                std::chrono::nanoseconds waitDuration = timeOfNextFrame - timeElapsed + std::chrono::nanoseconds( (int)((double)(readingFrame->repeat_pict) / (2 * state->frameRate) * SECONDS_TO_NANOSECONDS) );
                waitDuration -= frame_speed_skip_time;
                std::chrono::time_point<std::chrono::steady_clock> continueTime = std::chrono::steady_clock::now() + waitDuration;

                if (debug) {
                    printw("time Elapsed: %f, master time: %f, timeOfNextFrame: %f, waitDuration: %f\n", (double)timeElapsed.count() / SECONDS_TO_NANOSECONDS, state->playback->time, (double)timeOfNextFrame.count() / SECONDS_TO_NANOSECONDS, (double)waitDuration.count() / SECONDS_TO_NANOSECONDS);
                    printw(" Speed Factor: %f, Time Skipped due to Speed: %f, Time Skipped due to Speed on Current Frame: %f\n", state->playback->speed, (double)speed_skipped_time.count() / SECONDS_TO_NANOSECONDS, (double)frame_speed_skip_time.count() / SECONDS_TO_NANOSECONDS);
                    refresh();
                }

                videoLock.unlock();
                playbackLock.unlock();
                if (waitDuration <= std::chrono::nanoseconds(0)) {
                    continue;
                }

                /* std::this_thread::sleep_for(waitDuration); */
                std::chrono::time_point<std::chrono::steady_clock> lastWaitTime = std::chrono::steady_clock::now();
                while (std::chrono::steady_clock::now() < continueTime) {
                    playbackLock.lock();
                    state->playback->time += (double)( (double)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lastWaitTime).count() / SECONDS_TO_NANOSECONDS );
                    playbackLock.unlock();
                    lastWaitTime = std::chrono::steady_clock::now();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

            }
            free(readingFrame);
        }

        static void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
            VideoState* state = static_cast<VideoState*>(pDevice->pUserData);
            /* printw("Target Target PTS: %f", state->playback->time / state->audioTimeBase); */
            /* refresh(); */
            std::lock_guard<std::mutex> audioLock(state->audioMutex);
            std::lock_guard<std::mutex> playbackLock(state->playbackMutex);
            const int stretchedSampleRate = (int)(state->audioCodecContext->sample_rate * state->playback->speed);
            const int64_t targetAudioPTS = (int64_t)(state->playback->time / state->audioTimeBase);
            bool debug = state->displaySettings.mode == AUDIO_DEBUG;
            bool wave = state->displaySettings.mode == WAVE;

            if (debug) {
                erase();
                addstr("AUDIO DEBUG: \n");
                printw("Playback: \n    Time: %f \n    Speed: %f \n Last Audio Play Time:   %f\n    Playing: %s\n", state->playback->time, state->playback->speed, state->playback->lastAudioPlayTime, state->playback->playing ? "true" : "false");
            }

            if (state->playback->lastAudioPlayTime == state->playback->time)  {
                if (debug) {
                    addstr("EXITED, TIME HAS NOT CHANGED\n");
                    refresh();
                }
                return;
            }

            if (state->inUse && state->playback->playing) {
                int samplesToSkip = 0;
                bool shouldStretchAudioSegment = state->playback->speed < 1.0;
                double stretchAmount = std::max(1.0, 1 / state->playback->speed);
                int bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
                int64_t lastPTS = state->audioFrames->get()->pts;
                bool result;

                if (shouldStretchAudioSegment) {
                    int result;
                    AVChannelLayout inLayout = state->audioCodecContext->ch_layout;
                    AVChannelLayout outLayout = inLayout;
                    result = swr_alloc_set_opts2(
                        &(state->inTimeAudioResampler),
                        &outLayout,
                        AV_SAMPLE_FMT_FLT,
                        state->audioCodecContext->sample_rate / stretchAmount,
                        &inLayout,
                        AV_SAMPLE_FMT_FLT,
                        state->audioCodecContext->sample_rate,
                        0,
                        nullptr);

                    if (result < 0) {
                        shouldStretchAudioSegment = false;
                        if (debug)
                            printw("COULD NOT RESET AUDIO RESAMPLER PARAMETERS\n");
                    } else {
                        result = swr_init(state->inTimeAudioResampler);
                        if (result < 0) {
                            printw("COULD NOT INITIALIZE AUDIO RESAMPLER\n");
                            shouldStretchAudioSegment = false;
                        }
                    }
                }


                while (true) {
                    int64_t currentIndexPTS = state->audioFrames->get()->pts;
                    if (currentIndexPTS > targetAudioPTS) {
                        result = state->audioFrames->set_index(state->audioFrames->get_index() - 1);
                        if (state->audioFrames->get()->pts < targetAudioPTS) {
                            break;
                        }
                    } else if (currentIndexPTS < targetAudioPTS) {
                        result = state->audioFrames->set_index(state->audioFrames->get_index() + 1);
                        if (state->audioFrames->get()->pts > targetAudioPTS) {
                            state->audioFrames->set_index(state->audioFrames->get_index() - 1);
                            break;
                        }
                    } else if (currentIndexPTS == targetAudioPTS) {
                        break;
                    }

                    if (state->audioFrames->get_index() == 0 || state->audioFrames->get_index() == state->audioFrames->get_length() - 1) {
                        break;
                    } else if (std::min(currentIndexPTS, lastPTS) <= targetAudioPTS && std::max(currentIndexPTS, lastPTS) >= targetAudioPTS) {
                        break;
                    } else if (!result) {
                        break;
                    }
                    lastPTS = currentIndexPTS;
                } 

                AVFrame* startingAudioFrame = state->audioFrames->get();
                samplesToSkip = std::round((state->playback->time - startingAudioFrame->pts * state->audioTimeBase) * state->audioCodecContext->sample_rate); 
                AVFrame* current = state->audioFrames->get();
                av_audio_fifo_reset(state->tempAudio);
                int samplesWritten = 0;
                int framesRead = 0;

                while (samplesWritten < frameCount + samplesToSkip && av_audio_fifo_space(state->tempAudio) > current->nb_samples) {
                    framesRead++;

                    if (shouldStretchAudioSegment) {
                        int samplesToWrite = (int)(current->nb_samples * stretchAmount);
                        int bytesToWrite = samplesToWrite * bytesPerSample;
                        float** originalValues;
                        float** finalValues;
                        float** resampledValues;
                        av_samples_alloc_array_and_samples((uint8_t***)(&originalValues), current->linesize, current->ch_layout.nb_channels, current->nb_samples * 3 / 2, AV_SAMPLE_FMT_FLT, 0);
                        av_samples_alloc_array_and_samples((uint8_t***)(&finalValues), current->linesize, current->ch_layout.nb_channels, samplesToWrite * 3 / 2, AV_SAMPLE_FMT_FLT, 0);
                        av_samples_alloc_array_and_samples((uint8_t***)(&resampledValues), current->linesize, current->ch_layout.nb_channels, current->nb_samples * 3 / 2, AV_SAMPLE_FMT_FLT, 0);

                        av_samples_copy((uint8_t**)originalValues, (uint8_t *const *)current->data, 0, 0, current->nb_samples, current->ch_layout.nb_channels, AV_SAMPLE_FMT_FLT);
                        
                        if (debug) {
                            printw("Current nb_samples: %d, Stretch Amount: %f, bytesPerSample: %d, inputtedBytes: %d, bytesToWrite: %d\n", current->nb_samples, stretchAmount, bytesPerSample, current->nb_samples * bytesPerSample, samplesToWrite * bytesPerSample);
                            printw("Frame %d:\n     ", framesRead);
                        }

                        /* if (debug) { */
                        /*     for (int i = 0; i < current->nb_samples; i++) { */
                        /*         printw("%f ", originalValues[0][i]); */
                        /*     } */
                        /* } */
                        
                        int channelCount = current->ch_layout.nb_channels;
                        float orignalSampleBlocks[current->nb_samples][channelCount];
                        float finalSampleBlocks[samplesToWrite][channelCount];
                        for (int i = 0; i < current->nb_samples * channelCount; i++) {
                            orignalSampleBlocks[i / channelCount][i % channelCount] = originalValues[0][i];
                        }
                        
                        float currentSampleBlock[channelCount];
                        float lastSampleBlock[channelCount];
                        float currentFinalSampleBlockIndex = 0;
                        for (int i = 0; i < current->nb_samples; i++) {
                            for (int cpy = 0; cpy < channelCount; cpy++) {
                                currentSampleBlock[cpy] = orignalSampleBlocks[i][cpy];
                            }

                            if (i != 0) {
                                for (int currentChannel = 0; currentChannel < channelCount; currentChannel++) {
                                    float sampleStep = (currentSampleBlock[currentChannel] - lastSampleBlock[currentChannel]) / stretchAmount; 
                                    float currentSampleValue = lastSampleBlock[currentChannel];

                                    for (int finalSampleBlockIndex = (int)(currentFinalSampleBlockIndex); finalSampleBlockIndex < (int)(currentFinalSampleBlockIndex + stretchAmount); finalSampleBlockIndex++) {
                                        finalSampleBlocks[finalSampleBlockIndex][currentChannel] = currentSampleValue;
                                        currentSampleValue += sampleStep;
                                    }

                                }
                            }

                            currentFinalSampleBlockIndex += stretchAmount;
                            for (int cpy = 0; cpy < channelCount; cpy++) {
                                lastSampleBlock[cpy] = currentSampleBlock[cpy];
                            }
                        }

                        for (int i = 0; i < samplesToWrite * channelCount; i++) {
                            finalValues[0][i] = finalSampleBlocks[i / channelCount][i / samplesToWrite];
                        }

                        
                        int conversionResult = swr_convert(state->inTimeAudioResampler, (uint8_t**)(resampledValues), current->nb_samples, (const uint8_t**)(finalValues), samplesToWrite);
                        if (conversionResult < 0) {
                            if (debug)
                                addstr("Could not correctly perform SWR resampling");
                            samplesWritten += av_audio_fifo_write(state->tempAudio, (void**)finalValues, samplesToWrite);
                        } else {
                            samplesWritten += av_audio_fifo_write(state->tempAudio, (void**)resampledValues, current->nb_samples);
                        }


                        if (wave) {
                            erase();
                            addstr("wave unimplemented currently");
                            refresh();
                        }

                         av_freep(&originalValues[0]);
                         av_freep(&finalValues[0]);
                    } else if (!shouldStretchAudioSegment) {
                         samplesWritten += av_audio_fifo_write(state->tempAudio, (void**)current->data, current->nb_samples);
                    }

                    if (state->audioFrames->get_index() < state->audioFrames->get_length() + 1) {
                        state->audioFrames->set_index(state->audioFrames->get_index() + 1);
                        current = state->audioFrames->get();
                    } else {
                        break;
                    }
                }

                /* if (state->playback->speed < 1.0) { */
                /*     pDevice->sampleRate = (int)(state->audioCodecContext->sample_rate); */
                /* } else { */
                /*     pDevice->sampleRate = (int)(state->audioCodecContext->sample_rate); */
                /* } */

                if (shouldStretchAudioSegment) {
                    samplesToSkip = std::round((state->playback->time - startingAudioFrame->pts * state->audioTimeBase) * state->audioCodecContext->sample_rate / stretchAmount); 
                }

                av_audio_fifo_peek_at(state->tempAudio, &pOutput, std::min((int)frameCount, samplesWritten), samplesToSkip );
                state->playback->lastAudioPlayTime = state->playback->time;

                if (debug) {
                    printw("Samples Written: %d, Samples Skipped: %d, Frames Read: %d Device Sample Rate: %d\n ", samplesWritten, samplesToSkip, framesRead, pDevice->sampleRate);
                }

            } else if (!state->inUse && debug) {
                addstr("Video State not in use\n");
            }

            refresh();
            (void)pInput;
        }

        static int audioPlaybackThread(VideoState* state) {
            ma_device_config config = ma_device_config_init(ma_device_type_playback);
            config.playback.format  = ma_format_f32;
            config.playback.channels = state->audioStream->codecpar->ch_layout.nb_channels;              
            config.sampleRate = state->audioStream->codecpar->sample_rate;           
            config.dataCallback = VideoState::audioDataCallback;   
            config.pUserData = state;   


            ma_result miniAudioLog;
            ma_device audioDevice;
            miniAudioLog = ma_device_init(NULL, &config, &audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                std::cout << "FAILED TO INITIALIZE AUDIO DEVICE: " << miniAudioLog << std::endl;
                return EXIT_FAILURE;  // Failed to initialize the device.
            }

            state->audioDevice = &audioDevice;

            std::this_thread::sleep_for(std::chrono::nanoseconds((int)(state->audioStream->start_time * state->audioTimeBase * SECONDS_TO_NANOSECONDS)));
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
            std::unique_lock<std::mutex> playbackLock(state->playbackMutex, std::defer_lock);
            playbackLock.lock();
            while (!state->playback->allPacketsRead && state->inUse) {
                playbackLock.unlock();
                if (state->videoFrames->get_length() < FRAME_RESERVE_SIZE && state->audioFrames->get_length() < FRAME_RESERVE_SIZE) {
                    state->fetch_next(100);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                playbackLock.lock();
            }
        }

        static void inputListeningThread(VideoState* state) {
            std::unique_lock<std::mutex> symbolLock(state->symbolMutex, std::defer_lock);
            std::unique_lock<std::mutex> playbackLock(state->playbackMutex, std::defer_lock);
            
            
            while (state->inUse) {

                int ch = getch();

                symbolLock.lock();
                if (ch == (int)(' ')) {
                    playbackLock.lock();
                    if (state->playback->playing) {
                        state->symbols.push_back(get_video_symbol(PAUSE_ICON));
                        playbackLock.unlock();
                        state->pause();
                    } else {
                        state->symbols.push_back(get_video_symbol(PLAY_ICON));
                        playbackLock.unlock();
                        state->restart();
                    }
                } else if (ch == (int)('d') || ch == (int)('D')) {
                    state->displayModeIndex = (state->displayModeIndex + 1) % state->nb_displayModes;  
                    state->displaySettings.mode = VideoState::displayModes[state->displayModeIndex];
                } else if (ch == KEY_LEFT) {
                    if ( (std::chrono::steady_clock::now() - state->timeOfLastTimeChange) > VideoState::timeChangeWait ) {
                        state->symbols.push_back(get_video_symbol(BACKWARD_ICON));
                        playbackLock.lock();
                        double targetTime = state->playback->time - 5;
                        playbackLock.unlock();
                        state->jumpToTime(targetTime);
                    } 
                } else if (ch == KEY_RIGHT) {
                    if ( (std::chrono::steady_clock::now() - state->timeOfLastTimeChange) > VideoState::timeChangeWait ) {
                        state->symbols.push_back(get_video_symbol(FORWARD_ICON));
                        playbackLock.lock();
                        double targetTime = state->playback->time + 5;
                        playbackLock.unlock();
                        state->jumpToTime(targetTime);
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
                } else if (ch == (int)('n') || ch == (int)('N')) {
                    if ( (std::chrono::steady_clock::now() - state->timeOfLastTimeChange) > VideoState::timeChangeWait ) {
                        playbackLock.lock();
                        state->playback->speed = std::min(5.0, state->playback->speed + PLAYBACK_SPEED_CHANGE_INTERVAL);
                        playbackLock.unlock();
                    }
                } else if (ch == (int)('m') || ch == (int)('M')) {
                    if ( (std::chrono::steady_clock::now() - state->timeOfLastTimeChange) > VideoState::timeChangeWait ) {
                        playbackLock.lock();
                        state->playback->speed = std::max(0.25, state->playback->speed - PLAYBACK_SPEED_CHANGE_INTERVAL);
                        playbackLock.unlock();
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
            swr_free(&(this->inTimeAudioResampler));
            sws_freeContext(this->videoResizer);

            /* VideoFrame* readingFrame; */
            /* while (av_fifo_can_read(this->videoFifo) > 0) { */
            /*     av_fifo_read(this->videoFifo, readingFrame, 1); */
            /*     video_frame_free(readingFrame); */
            /* } */

            videoFrames->set_index(0);
            for (int i = 0; i < videoFrames->get_length(); i++) {
                video_frame_free(videoFrames->get());
                videoFrames->set_index(videoFrames->get_index() + 1);
            }
            videoFrames->clear();
            delete videoFrames;


            audioFrames->set_index(0);
            for (int i = 0; i < audioFrames->get_length(); i++) {
                AVFrame* currentAudioFrame = audioFrames->get();
                av_frame_free(&currentAudioFrame);
                audioFrames->set_index(audioFrames->get_index() + 1);
            }
            audioFrames->clear();
            delete audioFrames;
            /* av_fifo_freep2(&(this->videoFifo)); */
            av_audio_fifo_free(this->tempAudio);

            avformat_free_context(this->formatContext);
            ma_device_uninit(this->audioDevice);
            
            playback_free(this->playback);

            this->valid = false;
        }

        void pause() {
            std::lock_guard<std::mutex> playbackLock{this->playbackMutex};
            if (this->inUse) {
                this->playback->playing = false;
            }
        }

        void restart() {
            std::lock_guard<std::mutex> playbackLock{this->playbackMutex};
            if (this->inUse) {
                this->playback->playing = true;
            }
        }

        void jumpToTime(double targetTime) {
            std::lock_guard<std::mutex> videoLock(this->videoMutex);
            std::lock_guard<std::mutex> audioLock(this->audioMutex);
            std::lock_guard<std::mutex> playbackLock{this->playbackMutex};

            VideoFrame* currentVideoFrame = this->videoFrames->get();

            double originalTime = this->playback->time;
            
            double lastTime = currentVideoFrame->pts * videoTimeBase;
            while (true) {
                currentVideoFrame = this->videoFrames->get();
                if (currentVideoFrame->pts * videoTimeBase == targetTime) {
                    break;
                }
                else if (currentVideoFrame->pts * videoTimeBase > targetTime) {
                    if (this->videoFrames->get_index() == 0) {
                        break;
                    }
                    
                    this->videoFrames->set_index(this->videoFrames->get_index() - 1);
                    if (this->videoFrames->get()->pts * videoTimeBase < targetTime) {
                        break;
                    }
                }
                else if (currentVideoFrame->pts * videoTimeBase < targetTime) {
                    if (this->videoFrames->get_index() == this->videoFrames->get_length() - 1) {
                        break;
                    }

                    this->videoFrames->set_index(this->videoFrames->get_index() + 1);
                    if (this->videoFrames->get()->pts * videoTimeBase > targetTime) {
                        this->videoFrames->set_index(this->videoFrames->get_index() - 1);
                        break;
                    }
                }
            }

            double timeMoved = (this->videoFrames->get()->pts * this->videoTimeBase) - originalTime;
            this->playback->skippedPTS += (int64_t)(timeMoved / videoTimeBase);
            timeOfLastTimeChange = std::chrono::steady_clock::now();
        }

        int fetch_next(int requestedPacketCount) {
            if (this->valid == false) {
                std::cout << "CANNOT FETCH FROM INVALID VIDEO STATE" << std::endl;
                return EXIT_FAILURE;
            }

            std::unique_lock<std::mutex> videoLock(this->videoMutex, std::defer_lock);
            std::unique_lock<std::mutex> audioLock(this->audioMutex, std::defer_lock);
            std::unique_lock<std::mutex> playbackLock{this->playbackMutex, std::defer_lock};
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
                playbackLock.lock();
                this->playback->currentPacket++;
                playbackLock.unlock();
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
                        resizedVideoFrame->pts = originalVideoFrame->pts;
                        resizedVideoFrame->repeat_pict = originalVideoFrame->repeat_pict;
                        resizedVideoFrame->duration = originalVideoFrame->duration;
                        av_frame_get_buffer(resizedVideoFrame, 1); //watch this alignment

                        sws_scale(videoResizer, (uint8_t const * const *)originalVideoFrame->data, originalVideoFrame->linesize, 0, this->videoCodecContext->height, resizedVideoFrame->data, resizedVideoFrame->linesize);

                        videoLock.lock();
                            /* if (av_fifo_can_write(this->videoFifo) < 10) { */
                            /*     av_fifo_grow2(this->videoFifo, 10); */
                            /* } */
                            videoPacketCount++;
                            VideoFrame* videoFrame = video_frame_alloc(resizedVideoFrame);
                            this->videoFrames->push_back(videoFrame);
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
                            resampledFrame->sample_rate = this->audioStream->codecpar->sample_rate;
                            resampledFrame->ch_layout = audioFrame->ch_layout;
                            resampledFrame->format = AV_SAMPLE_FMT_FLT;
                            resampledFrame->pts = audioFrame->pts;
                            resampledFrame->duration = audioFrame->duration;

                            result = swr_convert_frame(audioResampler, resampledFrame, audioFrame);
                            if (result < 0) {
                                std::cout << "Unable to sample frame" << std::endl;
                            }

                            this->audioFrames->push_back(resampledFrame);

                            /* av_audio_fifo_write(this->audioFifo, (void**)resampledFrame->data, resampledFrame->nb_samples); */
                            av_frame_unref(audioFrame);
                            /* av_frame_free(&resampledFrame); */
                            result = avcodec_receive_frame(this->audioCodecContext, audioFrame);
                        }
                        audioLock.unlock();
                    }
                }

                av_packet_unref(readingPacket);
            }

            playbackLock.lock();
            this->playback->allPacketsRead = true;
            playbackLock.unlock();
            cleanup(readingPacket, originalVideoFrame, audioFrame);
            return EXIT_SUCCESS;
        }

        static void fullyLoad(VideoState* state) {
        std::unique_lock<std::mutex> playbackLock{state->playbackMutex, std::defer_lock};
        const int MAX_PROGESS_BAR_WIDTH = 100;
        int windowWidth, windowHeight;
        char progressBar[MAX_PROGESS_BAR_WIDTH + 1];
        progressBar[MAX_PROGESS_BAR_WIDTH] = '\0';

        double percent;
        playbackLock.lock();
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
            playbackLock.unlock();
            state->fetch_next(200);
            playbackLock.lock();
        }

    }
};

const VideoStateDisplayMode VideoState::displayModes[4] = { MOVIE, VIDEO_DEBUG, AUDIO_DEBUG, WAVE };
const std::chrono::milliseconds VideoState::timeChangeWait = std::chrono::milliseconds(TIME_CHANGE_WAIT_MILLISECONDS);
const std::chrono::milliseconds VideoState::playbackSpeedChangeWait = std::chrono::milliseconds(PLAYBACK_SPEED_CHANGE_WAIT_MILLISECONDS);


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

VideoFrame* video_frame_alloc(AVFrame* avFrame) {
    VideoFrame* videoFrame = (VideoFrame*)malloc(sizeof(VideoFrame));
    pixel_data* pixelData = pixel_data_alloc(avFrame->width, avFrame->height);

    for (int i = 0; i < avFrame->width * avFrame->height; i++) {
        pixelData->pixels[i] = avFrame->data[0][i];
    }

    videoFrame->pixelData = pixelData;
    videoFrame->pts = avFrame->pts;
    videoFrame->repeat_pict = avFrame->repeat_pict;
    videoFrame->duration = avFrame->duration;
    return videoFrame;
}

void video_frame_free(VideoFrame* videoFrame) {
    pixel_data_free(videoFrame->pixelData);
    free(videoFrame);
}

Playback* playback_alloc() {
    Playback* playback = (Playback*)malloc(sizeof(Playback));
    playback->time = 0.0;
    playback->lastAudioPlayTime = 0.0;
    playback->speed = 1.0;
    playback->currentPacket = 0;
    playback->allPacketsRead = false;
    playback->playing = false;
    playback->skippedPTS = 0;
    return playback; 
}

void playback_free(Playback* playback) {
    free(playback);
}

/* AudioFrame* audio_frame_alloc(AVFrame* frame) { */
/*     AudioFrame* audioFrame = (AudioFrame*)malloc(sizeof(AudioFrame)); */
/*     for (int i = 0; i < frame->ch_layout.nb_channels; i++) { */
/*         audioFrame->data[i] = (uint8_t*)malloc(frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format)); */
/*     } */

/*     audioFrame->ch_layout = frame->ch_layout; */
/*     audioFrame->nb_samples = frame->nb_samples; */
/*     audioFrame->sample_fmt = (AVSampleFormat)frame->format; */
/*     audioFrame->pts = frame->pts; */
    
/*     for (int ibyte = 0; ibyte < frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format); ibyte++) { */
/*         for (int ichannel = 0; ichannel < frame->ch_layout.nb_channels; ichannel++) { */
/*             audioFrame->data[ichannel][ibyte] = frame->data[ichannel][ibyte]; */
/*         } */
/*     } */

/*     return audioFrame; */
/* } */

/* void audio_frame_free(AudioFrame* audioFrame) { */
/*     for (int i = 0; i < audioFrame->ch_layout.nb_channels; i++) { */
/*         free(audioFrame->data[i]); */
/*     } */
/*     free(audioFrame); */
/* } */
