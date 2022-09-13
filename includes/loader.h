#pragma once
#include <media.h>
#include <mutex>

void backgroundLoadingThread(MediaPlayer* player, std::mutex alterMutex);
void fetch_next(MediaData* media_data, int requestedPacketCount);
