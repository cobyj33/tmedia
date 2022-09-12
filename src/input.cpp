#include "video.h"
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
    std::chrono::milliseconds timeChangeWait = std::chrono::milliseconds(TIME_CHANGE_WAIT_MILLISECONDS);
    std::chrono::milliseconds inputSleep = std::chrono::milliseconds(0);
    savetty();
    
    while (player->inUse) {
        resetty();
        inputSleep = std::chrono::milliseconds(500);
        int ch = getch();
        alterLock.lock();
        Playback* playback = player->timeline->playback;

        if (ch == (int)(' ')) {
            if (player->inUse) {
                playback->playing = !playback->playing;
            }
        } else if (ch == (int)('d') || ch == (int)('D')) {
            player->displaySettings->show_debug = !player->displaySettings->show_debug;
        } else if (ch == (int)('c') || ch == (int)('C')) {
            player->displaySettings->mode = get_next_display_mode(player->displaySettings->mode);
        } else if (ch == KEY_LEFT) {
            double targetTime = playback->time - TIME_CHANGE_AMOUNT;
            jump_to_time(player->timeline, targetTime);
            video_symbol_stack_push(player->displayCache->symbol_stack, get_video_symbol(BACKWARD_ICON));
            /* inputSleep = timeChangeWait; */
        } else if (ch == KEY_RIGHT) {
            double targetTime = playback->time + TIME_CHANGE_AMOUNT;
            jump_to_time(player->timeline, targetTime);
            video_symbol_stack_push(player->displayCache->symbol_stack, get_video_symbol(FORWARD_ICON));
            /* inputSleep = timeChangeWait; */
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
}
