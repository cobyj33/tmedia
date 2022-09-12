#include <ascii.h>
#include <renderer.h>
#include <icons.h>
#include <media.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <ncurses.h>

void render_thread(MediaPlayer* player, std::mutex* alterMutex) {
    std::unique_lock<std::mutex> alterLock(*alterMutex, std::defer_lock);
    int frames_rendered = 0;

    while (player->inUse) {
        alterLock.lock();
        frames_rendered++;
        add_debug_message(player->displayCache->debug_info, "Frames Rendered: %d", frames_rendered);
        render_screen(player);
        alterLock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void render_screen(MediaPlayer* player) {
    render_movie_screen(player);
    /* render_video_debug(player); */
    /* if (player->displaySettings->show_debug) { */
    /*     if (player->displaySettings->mode == VIDEO) { */
    /*         render_video_debug(player); */
    /*     } else if (player->displaySettings->mode == AUDIO) { */
    /*         render_audio_debug(player); */
    /*     } */
    /* } else { */
    /*     if (player->displaySettings->mode == VIDEO) { */
    /*         render_movie_screen(player); */
    /*     } else if (player->displaySettings->mode == AUDIO) { */
    /*         render_audio_screen(player); */
    /*     } */
    /* } */
}

void render_audio_screen(MediaPlayer *player) {
    erase();
    printw("AUDIO");
    refresh();
}

void render_audio_debug(MediaPlayer* player) {
    erase();
    for (int i = 0; i < player->displayCache->debug_info->nb_messages; i++) {
        printw("%s", player->displayCache->debug_info->messages[i]);
    }
    clear_media_debug(player->displayCache->debug_info);
    refresh();
}

void render_video_debug(MediaPlayer *player) {
    erase();
    for (int i = 0; i < player->displayCache->debug_info->nb_messages; i++) {
        printw("%s", player->displayCache->debug_info->messages[i]);
    }

    clear_media_debug(player->displayCache->debug_info);
    refresh();
}

void render_movie_screen(MediaPlayer* player) {
    erase();
    if (player->displayCache->image != nullptr) {
        VideoSymbolStack* symbol_stack = player->displayCache->symbol_stack;
        ascii_image textImage = get_ascii_image_bounded(player->displayCache->image, COLS, LINES);

        while (player->displayCache->symbol_stack->top >= 0) {
            VideoSymbol* currentSymbol = video_symbol_stack_peek(symbol_stack); 
            if (std::chrono::steady_clock::now() - currentSymbol->startTime < currentSymbol->lifeTime) {
                int currentSymbolFrame = get_video_symbol_current_frame(currentSymbol); 
                ascii_image symbolImage = get_ascii_image_bounded(currentSymbol->frameData[currentSymbolFrame], textImage.width, textImage.height);
                overlap_ascii_images(&textImage, &symbolImage);
                break;
            } else {
                video_symbol_stack_erase_pop(symbol_stack);
            }
        }

        if (!player->timeline->playback->playing) {
            VideoSymbol* pauseSymbol = get_video_symbol(PAUSE_ICON);
            ascii_image symbolImage = get_ascii_image_bounded(pauseSymbol->frameData[0], textImage.width, textImage.height);
            overlap_ascii_images(&textImage, &symbolImage);
            free_video_symbol(pauseSymbol);
        }

        print_ascii_image_full(&textImage);
    }
    refresh();
}

void print_ascii_image_full(ascii_image* textImage) {
    move(0, 0);
    int horizontalPaddingWidth = (COLS - textImage->width) / 2; 
    int verticalPaddingHeight = (LINES - textImage->height) / 2;
    char horizontalPadding[horizontalPaddingWidth + 1];
    horizontalPadding[horizontalPaddingWidth] = '\0';
    char lineAcross[COLS + 1];
    lineAcross[COLS] = '\0';

    for (int i = 0; i < horizontalPaddingWidth; i++) {
        horizontalPadding[i] = ' ';
    }

    for (int i = 0; i < COLS; i++) {
        lineAcross[i] = ' ';
    }

    for (int i = 0; i < verticalPaddingHeight; i++) {
        printw("%s\n", lineAcross);
    }

    for (int row = 0; row < textImage->height; row++) {
        printw("%s%s\n", horizontalPadding, textImage->lines[row]);
    }
}
