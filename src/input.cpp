#include "video.h"
#include "renderer.h"
#include <chrono>
#include <media.h>
#include <mutex>
#include <ncurses.h>
#include <thread>
#include <macros.h>
#include <icons.h>
#include <video.h>

void input_thread(MediaPlayer* player, std::mutex* alterMutex) {
    std::unique_lock<std::mutex> alterLock{*alterMutex, std::defer_lock};
    std::chrono::milliseconds inputSleep = std::chrono::milliseconds(0);

    WINDOW* inputWindow = newwin(0, 0, 1, 1);
    nodelay(inputWindow, true);
    keypad(inputWindow, true);

    double jump_time_requested = 0;
    
    while (player->inUse) {
        inputSleep = std::chrono::milliseconds(1);
        int ch = wgetch(inputWindow);
        alterLock.lock();
        Playback* playback = player->timeline->playback;

        if (!player->inUse) {
            break;
        }

        if (jump_time_requested != 0 && (ch != KEY_LEFT && ch != KEY_RIGHT)) {
            double targetTime = playback->time + jump_time_requested;
            if (targetTime >= player->timeline->mediaData->duration) {
                player->inUse = false;
                alterLock.unlock();
                break;
            }

            jump_to_time(player->timeline, targetTime);
            video_symbol_stack_push(player->displayCache->symbol_stack, jump_time_requested < 0 ? get_video_symbol(BACKWARD_ICON) : get_video_symbol(FORWARD_ICON));
            jump_time_requested = 0;
        }

        if (ch == (int)(' ')) {
            playback->playing = !playback->playing;
        } else if (ch == (int)('d') || ch == (int)('D')) {
            player->displaySettings->show_debug = !player->displaySettings->show_debug;
        } else if (ch == (int)('c') || ch == (int)('C')) {
            player->displaySettings->mode = get_next_display_mode(player->displaySettings->mode);
        } else if (ch == KEY_LEFT) {
            jump_time_requested -= TIME_CHANGE_AMOUNT;
        } else if (ch == KEY_RIGHT) {
            jump_time_requested += TIME_CHANGE_AMOUNT;
        } else if (ch == KEY_UP) {
            playback->volume = std::min(1.0, playback->volume + VOLUME_CHANGE_AMOUNT);
            video_symbol_stack_push(player->displayCache->symbol_stack,get_symbol_from_volume(playback->volume));
        } else if (ch == KEY_DOWN) {
            playback->volume = std::min(1.0, playback->volume - VOLUME_CHANGE_AMOUNT);
            video_symbol_stack_push(player->displayCache->symbol_stack,get_symbol_from_volume(playback->volume));
        } else if (ch == (int)('n') || ch == (int)('N')) {
            playback->speed = std::min(5.0, playback->speed + PLAYBACK_SPEED_CHANGE_INTERVAL);
        } else if (ch == (int)('m') || ch == (int)('M')) {
            playback->speed = std::max(0.25, playback->speed - PLAYBACK_SPEED_CHANGE_INTERVAL);
        } else if (ch == (int)('x') || ch == (int)('X')) {
            player->inUse = !player->inUse;
        } 

        alterLock.unlock();
        std::this_thread::sleep_for(inputSleep);
    }

    delwin(inputWindow);
}
