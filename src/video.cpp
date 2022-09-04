#include <asm-generic/errno-base.h>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <icons.h>
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

        /* DoubleLinkedList<VideoFrame*>* videoFrames; */
        /* DoubleLinkedList<AVFrame*>* audioFrames; */
        DoubleLinkedList<AVPacket*>* videoPackets;
        DoubleLinkedList<AVPacket*>* audioPackets;
        AVAudioFifo* tempAudio;

        SwsContext* videoResizer;
        SwrContext* audioResampler;
        SwrContext* inTimeAudioResampler;

        //Initialized
        ma_device* audioDevice;
        
        std::mutex alterMutex;

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
            
            this->videoPackets = new DoubleLinkedList<AVPacket*>();
            this->audioPackets = new DoubleLinkedList<AVPacket*>();

            this->tempAudio = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, this->audioStream->codecpar->ch_layout.nb_channels, 10000);
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
            bool debug, movie, isDisplaying;
            int frameCount = 0;
            std::unique_lock<std::mutex> alterLock{state->alterMutex, std::defer_lock};

            std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
            std::chrono::nanoseconds time_paused = std::chrono::nanoseconds(0);
            std::chrono::nanoseconds speed_skipped_time = std::chrono::nanoseconds(0);
            AVFrame* readingFrame;
        
            int64_t counter = 0;
            while (state->inUse && (!state->playback->allPacketsRead || (state->playback->allPacketsRead && state->videoPackets->get_length() > 0 ) )) {
                alterLock.lock();

                debug = state->displaySettings.mode == VIDEO_DEBUG;
                movie = state->displaySettings.mode == MOVIE;
                isDisplaying = debug || movie;

                if (isDisplaying) {
                    erase();
                }

                counter++;
                if (state->playback->playing == false) {
                    alterLock.unlock();
                    std::chrono::steady_clock::time_point pauseTime = std::chrono::steady_clock::now();
                    while (state->playback->playing == false) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        if (!state->inUse) {
                            return;
                        }
                        
                        if (debug || movie) {
                            alterLock.lock();
                            erase();
                            for (int i = 0; i < state->symbols.size(); i++) {
                                int symbolOutputWidth, symbolOutputHeight;
                                get_output_size(state->symbols[i].pixelData->width, state->symbols[i].pixelData->height, state->lastImage.width, state->lastImage.height, &symbolOutputWidth, &symbolOutputHeight);
                                ascii_image symbolImage = get_image(state->symbols[i].pixelData->pixels, state->symbols[i].pixelData->width, state->symbols[i].pixelData->height, symbolOutputWidth, symbolOutputHeight);
                                overlap_ascii_images(&state->lastImage, &symbolImage);
                            }

                            print_ascii_image(&state->lastImage);
                            refresh();
                            alterLock.unlock();
                        }
                    }
                    alterLock.lock();
                    time_paused = time_paused + (std::chrono::steady_clock::now() - pauseTime);
                }

                if (state->videoPackets->get_index() + 10 <= state->videoPackets->get_length()) {
                    int decodeResult;
                    readingFrame = VideoState::decodeVideoPacket(state, state->videoPackets->get(), &decodeResult);
                    int repeats = 1;
                    while (decodeResult == AVERROR(EAGAIN) && state->videoPackets->get_index() + 1 < state->videoPackets->get_length()) {
                        if (debug) {
                            erase();
                            printw("Feeding packet #%d", repeats);
                            refresh();
                        }

                        repeats++;
                        state->videoPackets->set_index(state->videoPackets->get_index() + 1);
                        av_frame_free(&readingFrame);
                        readingFrame = VideoState::decodeVideoPacket(state, state->videoPackets->get(), &decodeResult);
                    }

                    if (debug) {
                        printw("Fed %d packets to decode\n", repeats);
                    }
                    

                    if (readingFrame == nullptr) {
                        alterLock.unlock();
                        if (debug) {
                            addstr("ERROR: NULL POINTED VIDEO FRAME");
                        }

                        std::this_thread::sleep_for(std::chrono::nanoseconds((int64_t)(1 / state->frameRate * SECONDS_TO_NANOSECONDS )));
                        continue;
                    }
                    

                    if (state->videoPackets->get_index() + 1 < state->videoPackets->get_length()) {
                        state->videoPackets->set_index(state->videoPackets->get_index() + 1);
                    }
                } else {
                    while (!state->playback->allPacketsRead && state->videoPackets->get_index() + 10 >= state->videoPackets->get_length()) {
                            state->fetch_next(10);
                    }
                    alterLock.unlock();
                    continue;
                }


                if (debug || movie) {
                    int windowWidth, windowHeight;
                    get_window_size(&windowWidth, &windowHeight);
                    int outputWidth, outputHeight;
                    get_output_size(readingFrame->width, readingFrame->height, windowWidth, debug ? std::max(windowHeight - 12, 2) : windowHeight, &outputWidth, &outputHeight);

                        ascii_image asciiImage = get_image(readingFrame->data[0], readingFrame->width, readingFrame->height, outputWidth, outputHeight);
                        if (debug) {
                            printw("Counter: %ld\n", counter);
                            printw("Window Size: %d x %d", windowWidth, windowHeight);
                            printw("Frame Width: %d Frame Height: %d Window Width: %d Window Height: %d Output Width: %d Output Height: %d \n", state->frameWidth, state->frameHeight, windowWidth, windowHeight, outputWidth, outputHeight);
                            printw("ASCII Width: %d ASCII Height: %d\n", asciiImage.width, asciiImage.height);
                            printw("Packets: %d of %d\n", state->playback->currentPacket, state->totalPackets);
                            printw("Playing: %s \n", state->playback->playing ? "true" : "false");
                            printw("PTS: %ld\n Repeat-Pict: %d \n Skipped PTS: %ld\n", readingFrame->pts, readingFrame->repeat_pict, state->playback->skippedPTS);
                            printw("Pixel Data: Width: %d Height %d\n", readingFrame->width, readingFrame->height);
                            printw("Video List Length: %d, Audio List Length: %d\n", state->videoPackets->get_length(), state->audioPackets->get_length() );
                            printw("Video List Index: %d, Audio List Index: %d\n", state->videoPackets->get_index(), state->audioPackets->get_index());
                        }
                        
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

                if (isDisplaying) {
                    refresh();
                }

                alterLock.unlock();
                av_frame_free(&readingFrame);
                if (waitDuration <= std::chrono::nanoseconds(0)) {
                    continue;
                }

                /* std::this_thread::sleep_for(waitDuration); */
                std::chrono::time_point<std::chrono::steady_clock> lastWaitTime = std::chrono::steady_clock::now();
                while (std::chrono::steady_clock::now() < continueTime) {
                    alterLock.lock();
                    state->playback->time += (double)( (double)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lastWaitTime).count() / SECONDS_TO_NANOSECONDS );
                    alterLock.unlock();
                    lastWaitTime = std::chrono::steady_clock::now();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

            }
        }

        static void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
            VideoState* state = static_cast<VideoState*>(pDevice->pUserData);
            /* printw("Target Target PTS: %f", state->playback->time / state->audioTimeBase); */
            /* refresh(); */
            std::lock_guard<std::mutex> alterLock{state->alterMutex};
            const int stretchedSampleRate = (int)(state->audioCodecContext->sample_rate * state->playback->speed);

            const double MAX_ASYNC_TIME_SECONDS = 0.15;
            if (std::abs(state->playback->audioTime  - state->playback->time) > MAX_ASYNC_TIME_SECONDS) {
                state->playback->audioTime = state->playback->time;
            }
            


            state->playback->audioTime += (double)frameCount / (state->audioCodecContext->sample_rate * state->playback->speed);
            const int64_t targetAudioPTS = (int64_t)(state->playback->audioTime / state->audioTimeBase);
            bool debug = state->displaySettings.mode == AUDIO_DEBUG;
            bool wave = state->displaySettings.mode == WAVE;

            if (debug) {
                erase();
                addstr("AUDIO DEBUG: \n");
                printw("Playback: \n  Master Time: %f \n Audio Time: %f \n Unsync Amount: %f \n    Speed: %f \n Last Audio Play Time:   %f\n    Playing: %s\n", state->playback->time, state->playback->audioTime, std::abs(state->playback->audioTime - state->playback->time), state->playback->speed, state->playback->lastAudioPlayTime, state->playback->playing ? "true" : "false");
            }

            if (state->playback->lastAudioPlayTime == state->playback->audioTime)  {
                if (debug) {
                    addstr("EXITED, TIME HAS NOT CHANGED\n");
                    refresh();
                }
                return;
            } else if (!state->inUse) {
                if (debug) {
                    addstr("Video state not in use");
                    refresh();
                }
                return;
            }

            int samplesToSkip = 0;
            bool shouldShrinkAudioSegment = state->playback->speed > 1.0;
            bool shouldStretchAudioSegment = state->playback->speed < 1.0;
            double stretchAmount = 1 / state->playback->speed;
            int bytesPerSample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
            int64_t lastPTS = state->audioPackets->get()->pts;
            bool result;

            if (stretchAmount != 1.0) {
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
                    shouldShrinkAudioSegment = false;
                    if (debug)
                        printw("COULD NOT RESET AUDIO RESAMPLER PARAMETERS\n");
                } else {
                    result = swr_init(state->inTimeAudioResampler);
                    if (result < 0) {
                        printw("COULD NOT INITIALIZE AUDIO RESAMPLER\n");
                        shouldStretchAudioSegment = false;
                        shouldShrinkAudioSegment = false;
                    }
                }
            }


            // Finds correct audio segment to begin at according to presentational time stamps of the audio frames
            while (true) {
                int64_t currentIndexPTS = state->audioPackets->get()->pts;
                if (currentIndexPTS > targetAudioPTS) {
                    result = state->audioPackets->set_index(state->audioPackets->get_index() - 1);
                    if (state->audioPackets->get()->pts < targetAudioPTS) {
                        break;
                    }
                } else if (currentIndexPTS < targetAudioPTS) {
                    result = state->audioPackets->set_index(state->audioPackets->get_index() + 1);
                    if (state->audioPackets->get()->pts > targetAudioPTS) {
                        state->audioPackets->set_index(state->audioPackets->get_index() - 1);
                        break;
                    }
                } else if (currentIndexPTS == targetAudioPTS) {
                    break;
                }

                if (state->audioPackets->get_index() == 0 || state->audioPackets->get_index() == state->audioPackets->get_length() - 1) {
                    break;
                } else if (std::min(currentIndexPTS, lastPTS) <= targetAudioPTS && std::max(currentIndexPTS, lastPTS) >= targetAudioPTS) {
                    break;
                } else if (!result) {
                    break;
                }
                lastPTS = currentIndexPTS;
            } 

            std::vector<AVFrame*> audioFrames = VideoState::decodeAudioPacket(state, state->audioPackets->get());
            if (audioFrames.size() <= 0) {
                if (debug) {
                    addstr("NO OUTPUT FROM REQUESTING AUDIO PACKET DATA");
                }
                return;
            }

            int audioFrameIndex = 0;
            samplesToSkip = std::round((state->playback->audioTime - audioFrames[audioFrameIndex]->pts * state->audioTimeBase) * state->audioCodecContext->sample_rate / stretchAmount); 
            AVFrame* currentAudioFrame = audioFrames[audioFrameIndex];
            av_audio_fifo_reset(state->tempAudio);
            int samplesWritten = 0;
            int framesRead = 0;

            while (samplesWritten < frameCount + samplesToSkip && av_audio_fifo_space(state->tempAudio) > currentAudioFrame->nb_samples) {
                framesRead++;
                if (debug) {
                    printw("Frame %d:\n     ", framesRead);
                }

                if (state->playback->speed != 1.0) {
                    int samplesToWrite = (int)(currentAudioFrame->nb_samples * stretchAmount);
                    int bytesToWrite = samplesToWrite * bytesPerSample;
                    float** originalValues;
                    float** finalValues;
                    float** resampledValues;
                    av_samples_alloc_array_and_samples((uint8_t***)(&originalValues), currentAudioFrame->linesize, currentAudioFrame->ch_layout.nb_channels, currentAudioFrame->nb_samples * 3 / 2, AV_SAMPLE_FMT_FLT, 0);
                    av_samples_alloc_array_and_samples((uint8_t***)(&finalValues), currentAudioFrame->linesize, currentAudioFrame->ch_layout.nb_channels, samplesToWrite * 3 / 2, AV_SAMPLE_FMT_FLT, 0);
                    av_samples_alloc_array_and_samples((uint8_t***)(&resampledValues), currentAudioFrame->linesize, currentAudioFrame->ch_layout.nb_channels, currentAudioFrame->nb_samples * 3 / 2, AV_SAMPLE_FMT_FLT, 0);

                    av_samples_copy((uint8_t**)originalValues, (uint8_t *const *)currentAudioFrame->data, 0, 0, currentAudioFrame->nb_samples, currentAudioFrame->ch_layout.nb_channels, AV_SAMPLE_FMT_FLT);
                    
                    if (debug) {
                        printw("Current nb_samples: %d, Stretch Amount: %f, bytesPerSample: %d, inputtedBytes: %d, bytesToWrite: %d\n", currentAudioFrame->nb_samples, stretchAmount, bytesPerSample, currentAudioFrame->nb_samples * bytesPerSample, samplesToWrite * bytesPerSample);
                    }

                    /* if (debug) { */
                    /*     for (int i = 0; i < currentAudioFrame->nb_samples; i++) { */
                    /*         printw("%f ", originalValues[0][i]); */
                    /*     } */
                    /* } */
                    
                    int channelCount = currentAudioFrame->ch_layout.nb_channels;
                    float orignalSampleBlocks[currentAudioFrame->nb_samples][channelCount];
                    float finalSampleBlocks[samplesToWrite][channelCount];

                    for (int i = 0; i < currentAudioFrame->nb_samples * channelCount; i++) {
                        orignalSampleBlocks[i / channelCount][i % channelCount] = originalValues[0][i];
                    }
                    
                    if (shouldStretchAudioSegment) {
                        float currentSampleBlock[channelCount];
                        float lastSampleBlock[channelCount];
                        float currentFinalSampleBlockIndex = 0;

                        if (debug) {
                            printw("Stretching audio segment by factor of %f\n", stretchAmount);
                        }

                        for (int i = 0; i < currentAudioFrame->nb_samples; i++) {
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

                    } else if (shouldShrinkAudioSegment) {
                        float currentOriginalSampleBlockIndex = 0;
                        float stretchStep = state->playback->speed;

                        if (debug) {
                            printw("Shrinking audio segment by factor of %f\n", stretchAmount);
                        }

                        for (int i = 0; i < samplesToWrite; i++) {
                            float average = 0.0;
                            for (int ichannel = 0; ichannel < channelCount; ichannel++) {
                               average = 0.0;
                               for (int origi = (int)(currentOriginalSampleBlockIndex); origi < (int)(currentOriginalSampleBlockIndex + stretchStep); origi++) {
                                   average += orignalSampleBlocks[origi][ichannel];
                               }
                               average /= stretchStep;
                               finalSampleBlocks[i][ichannel] = average;
                            }

                            currentOriginalSampleBlockIndex += stretchStep;
                        }

                    }

                    
                    for (int i = 0; i < samplesToWrite * channelCount; i++) {
                        /* finalValues[0][i] = finalSampleBlocks[i / channelCount][i / samplesToWrite]; */
                        finalValues[0][i] = finalSampleBlocks[i / channelCount][i % channelCount];
                    }

                    int conversionResult = swr_convert(state->inTimeAudioResampler, (uint8_t**)(resampledValues), currentAudioFrame->nb_samples, (const uint8_t**)(finalValues), samplesToWrite);
                    if (conversionResult < 0) {
                        if (debug)
                            addstr("Could not correctly perform SWR resampling\n");
                        samplesWritten += av_audio_fifo_write(state->tempAudio, (void**)finalValues, samplesToWrite);
                    } else {
                        samplesWritten += av_audio_fifo_write(state->tempAudio, (void**)resampledValues, currentAudioFrame->nb_samples);
                    }


                    if (wave) {
                        erase();
                        addstr("wave unimplemented currently");
                        refresh();
                    }

                     av_freep(&originalValues[0]);
                     av_freep(&finalValues[0]);
                     av_freep(&resampledValues[0]);
                } else if (!shouldStretchAudioSegment && !shouldShrinkAudioSegment) {
                    if (debug) {
                        printw("Feeding unaltered audio data\n");
                    }
                     samplesWritten += av_audio_fifo_write(state->tempAudio, (void**)currentAudioFrame->data, currentAudioFrame->nb_samples);
                }

                av_frame_free(&currentAudioFrame);
                if (audioFrameIndex + 1 < audioFrames.size()) {
                    audioFrameIndex++;
                    currentAudioFrame = audioFrames[audioFrameIndex];
                } else {
                    audioFrameIndex = 0;
                    if (state->audioPackets->get_index() + 1 < state->audioPackets->get_length()) {
                        state->audioPackets->set_index( state->audioPackets->get_index() + 1 );
                        audioFrames = VideoState::decodeAudioPacket(state, state->audioPackets->get());

                        if (audioFrames.size() <= 0) {
                            if (debug) {
                                addstr("NO OUTPUT FROM REQUESTING AUDIO PACKET DATA");
                            }
                            return;
                        }

                        currentAudioFrame = audioFrames[audioFrameIndex];
                    } else {
                        break;
                    }
                }

            }

            const int audioSamplesToOutput = std::min((int)frameCount, samplesWritten);
            av_audio_fifo_peek_at(state->tempAudio, &pOutput, audioSamplesToOutput, samplesToSkip );
            state->playback->lastAudioPlayTime = state->playback->audioTime;

            if (debug) {
                printw("Samples Written: %d, Samples Requested: %d, Samples Skipped: %d, Frames Read: %d Device Sample Rate: %d\n ", samplesWritten, (int)frameCount, samplesToSkip, framesRead, pDevice->sampleRate);
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
            std::unique_lock<std::mutex> alterLock{state->alterMutex, std::defer_lock};

            while (!state->playback->allPacketsRead && state->inUse) {
                alterLock.lock();
                    if (state->videoPackets->get_length() < PACKET_RESERVE_SIZE && state->audioPackets->get_length() < PACKET_RESERVE_SIZE) {
                        state->fetch_next(20);
                    }
                alterLock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        static void inputListeningThread(VideoState* state) {
            std::unique_lock<std::mutex> alterLock{state->alterMutex, std::defer_lock};
            
            while (state->inUse) {

                int ch = getch();
                alterLock.lock();

                if (ch == (int)(' ')) {
                    if (state->inUse) {
                            if (state->playback->playing) {
                                state->symbols.push_back(get_video_symbol(PAUSE_ICON));
                            } else {
                                state->symbols.push_back(get_video_symbol(PLAY_ICON));
                            }
                            state->playback->playing = !state->playback->playing;
                    }
                } else if (ch == (int)('d') || ch == (int)('D')) {
                    state->displayModeIndex = (state->displayModeIndex + 1) % state->nb_displayModes;  
                    state->displaySettings.mode = VideoState::displayModes[state->displayModeIndex];
                } else if (ch == KEY_LEFT) {
                    if ( (std::chrono::steady_clock::now() - state->timeOfLastTimeChange) > VideoState::timeChangeWait ) {
                        state->symbols.push_back(get_video_symbol(BACKWARD_ICON));
                        double targetTime = state->playback->time - 10;
                        state->jumpToTime(targetTime);
                    } 
                } else if (ch == KEY_RIGHT) {
                    if ( (std::chrono::steady_clock::now() - state->timeOfLastTimeChange) > VideoState::timeChangeWait ) {
                        state->symbols.push_back(get_video_symbol(FORWARD_ICON));
                        double targetTime = state->playback->time + 10;
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
                        state->playback->speed = std::min(5.0, state->playback->speed + PLAYBACK_SPEED_CHANGE_INTERVAL);
                    }
                } else if (ch == (int)('m') || ch == (int)('M')) {
                    if ( (std::chrono::steady_clock::now() - state->timeOfLastTimeChange) > VideoState::timeChangeWait ) {
                        state->playback->speed = std::max(0.25, state->playback->speed - PLAYBACK_SPEED_CHANGE_INTERVAL);
                    }
                }

                alterLock.unlock();
            
            }
        }

        static AVFrame* decodeVideoPacket(VideoState* state, AVPacket* videoPacket, int* result) {
            if (videoPacket->stream_index != state->videoStream->index) {
                std::cerr << "CANNOT DECODE VIDEO PACKET THAT IS NOT FROM VIDEO STREAM: Packet Index: [" << videoPacket->stream_index << "] Stream Index: [" << state->videoStream->index << "] " << std::endl;
                return nullptr;
            }

            *result = avcodec_send_packet(state->videoCodecContext, videoPacket);
            if (*result < 0) {
                if (*result == AVERROR(EAGAIN) ) {
                    std::cout << "Video Packet reading Error: " << result << std::endl;
                    return nullptr;
                } else if (*result != AVERROR(EAGAIN) ) {
                    std::cout << "Video Packet reading Error: " << result << std::endl;
                    return nullptr;
                }
            }

            AVFrame* originalVideoFrame = av_frame_alloc();
            *result = avcodec_receive_frame(state->videoCodecContext, originalVideoFrame);
            if (*result == AVERROR(EAGAIN)) {
                av_frame_free(&originalVideoFrame);
                return nullptr;
            } else if (*result < 0) {
                std::cerr << "FATAL ERROR WHILE READING VIDEO PACKET: " << *result << std::endl;
                av_frame_free(&originalVideoFrame);
                return nullptr;
            } else {
                AVFrame* resizedVideoFrame = av_frame_alloc();
                resizedVideoFrame->format = AV_PIX_FMT_GRAY8;
                resizedVideoFrame->width = state->frameWidth;
                resizedVideoFrame->height = state->frameHeight;
                resizedVideoFrame->pts = originalVideoFrame->pts;
                resizedVideoFrame->repeat_pict = originalVideoFrame->repeat_pict;
                resizedVideoFrame->duration = originalVideoFrame->duration;
                av_frame_get_buffer(resizedVideoFrame, 1); //watch this alignment

                sws_scale(state->videoResizer, (uint8_t const * const *)originalVideoFrame->data, originalVideoFrame->linesize, 0, state->videoCodecContext->height, resizedVideoFrame->data, resizedVideoFrame->linesize);

                av_frame_free(&originalVideoFrame);
                return resizedVideoFrame;
            }
            
            return nullptr;
        }

        static std::vector<AVFrame*> decodeAudioPacket(VideoState* state, AVPacket* audioPacket) {
            std::vector<AVFrame*> frames;
            int result;
            if (audioPacket->stream_index != state->audioStream->index) {
                std::cerr << "CANNOT DECODE AUDIO PACKET THAT IS NOT FROM VIDEO STREAM: Packet Index: [" << audioPacket->stream_index << "] Stream Index: [" << state->audioStream->index << "] " << std::endl;
                return frames;
            }

            result = avcodec_send_packet(state->audioCodecContext, audioPacket);
            if (result < 0) {
                std::cerr << "ERROR WHILE SENDING AUDIO PACKET: " << result << std::endl; 
                return frames;
            }

            AVFrame* audioFrame = av_frame_alloc();
            result = avcodec_receive_frame(state->audioCodecContext, audioFrame);
            if (result < 0) {
                std::cerr << "ERROR WHILE RECEIVING AUDIO FRAMES: " << result << std::endl; 
                av_frame_free(&audioFrame);
                return frames;
            } else {
                while (result == 0) {
                    AVFrame* resampledFrame = av_frame_alloc();
                    resampledFrame->sample_rate = state->audioStream->codecpar->sample_rate;
                    resampledFrame->ch_layout = audioFrame->ch_layout;
                    resampledFrame->format = AV_SAMPLE_FMT_FLT;
                    resampledFrame->pts = audioFrame->pts;
                    resampledFrame->duration = audioFrame->duration;

                    result = swr_convert_frame(state->audioResampler, resampledFrame, audioFrame);
                    if (result < 0) {
                        std::cout << "Unable to resample audio frame" << std::endl;
                    }

                    frames.push_back(resampledFrame);

                    av_frame_unref(audioFrame);
                    result = avcodec_receive_frame(state->audioCodecContext, audioFrame);
                }
                av_frame_free(&audioFrame);
                return frames;
            }

            return frames;
        }

        public:

        void free_video_state() {
            avcodec_free_context(&(this->videoCodecContext));
            avcodec_free_context(&(this->audioCodecContext));
            swr_free(&(this->audioResampler));
            swr_free(&(this->inTimeAudioResampler));
            sws_freeContext(this->videoResizer);

            videoPackets->set_index(0);
            for (int i = 0; i < videoPackets->get_length(); i++) {
                AVPacket* currentPacket = videoPackets->get();
                av_packet_free(&currentPacket);
                videoPackets->set_index(videoPackets->get_index() + 1);
            }
            videoPackets->clear();
            delete videoPackets;


            audioPackets->set_index(0);
            for (int i = 0; i < audioPackets->get_length(); i++) {
                AVPacket* currentPacket = audioPackets->get();
                av_packet_free(&currentPacket);
                audioPackets->set_index(audioPackets->get_index() + 1);
            }
            audioPackets->clear();
            delete audioPackets;
            /* av_fifo_freep2(&(this->videoFifo)); */
            av_audio_fifo_free(this->tempAudio);

            avformat_free_context(this->formatContext);
            ma_device_uninit(this->audioDevice);
            
            playback_free(this->playback);

            this->valid = false;
        }

        //grab video, audio, and playback mutexes
        void jumpToTime(double targetTime) {
            const int64_t targetVideoPTS = targetTime / this->videoTimeBase;
            const double originalTime = this->playback->time;

            if (targetTime == this->playback->time) {
                return;
             } else if (targetTime < this->playback->time) {
                avcodec_flush_buffers(this->videoCodecContext);
                double testTime = std::max(0.0, targetTime - 30);
                while (this->videoPackets->get()->pts * this->videoTimeBase > testTime && this->videoPackets->get_index() > 0) {
                    this->videoPackets->set_index(this->videoPackets->get_index() - 1);
                }
            }

            AVFrame* readingFrame = nullptr;
            int64_t finalPTS = 0;
            int readingStatus = 0;

            readingFrame = VideoState::decodeVideoPacket(this, this->videoPackets->get(), &readingStatus);
            while (finalPTS < targetVideoPTS ) {
                if (readingStatus < 0 && readingStatus != AVERROR(EAGAIN)) {
                    printw("READING ERROR IN JUMP TO TIME: %d\n", readingStatus);
                    refresh();
                    break;
                }

                av_frame_free(&readingFrame);
                readingFrame = VideoState::decodeVideoPacket(this, this->videoPackets->get(), &readingStatus);
                while (readingStatus == AVERROR(EAGAIN) && this->videoPackets->get_index() + 1 < this->videoPackets->get_length()) {
                    this->videoPackets->set_index(this->videoPackets->get_index() + 1 );
                    av_frame_free(&readingFrame);
                    readingFrame = VideoState::decodeVideoPacket(this, this->videoPackets->get(), &readingStatus);
                }

                if (readingFrame != nullptr) {
                    finalPTS = readingFrame->pts;
                    if (finalPTS < targetVideoPTS) {
                        this->videoPackets->set_index(this->videoPackets->get_index() + 1 );
                        av_frame_free(&readingFrame);
                        readingFrame = VideoState::decodeVideoPacket(this, this->videoPackets->get(), &readingStatus);
                    }
                } else {
                    break;
                }
            }

            finalPTS = readingFrame == nullptr ? this->videoPackets->get()->pts : readingFrame->pts;
            double timeMoved = (finalPTS * this->videoTimeBase) - originalTime;
            this->playback->skippedPTS += (int64_t)(timeMoved / videoTimeBase);
            this->playback->time = finalPTS * this->videoTimeBase;
            this->playback->audioTime = this->playback->time;

            this->timeOfLastTimeChange = std::chrono::steady_clock::now();
            av_frame_free(&readingFrame);
        }

        void fetch_next(int requestedPacketCount) {
            if (this->valid == false) {
                std::cout << "CANNOT FETCH FROM INVALID VIDEO STATE" << std::endl;
                return;
            }

            AVPacket* readingPacket = av_packet_alloc();
            int result;
            int videoPacketCount = 0;
            int audioPacketCount = 0;

            while (av_read_frame(this->formatContext, readingPacket) == 0) {
                this->playback->currentPacket++;
                if (videoPacketCount >= requestedPacketCount) {
                    av_packet_free(&readingPacket);
                    return;
                }

                if (readingPacket->stream_index == this->videoStream->index) {
                    AVPacket* createdPacket = av_packet_alloc();
                    av_packet_ref(createdPacket, readingPacket);
                    this->videoPackets->push_back(createdPacket);
                    videoPacketCount++;
                } else if (readingPacket->stream_index == this->audioStream->index) {
                    AVPacket* createdPacket = av_packet_alloc();
                    av_packet_ref(createdPacket, readingPacket);
                    this->audioPackets->push_back(createdPacket);
                    audioPacketCount++;
                }

                av_packet_unref(readingPacket);
            }

            this->playback->allPacketsRead = true;
            av_packet_free(&readingPacket);
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
        videoState->fetch_next(500);
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
    playback->audioTime = 0.0;
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
