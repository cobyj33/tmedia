#include <vector>

#include "media.h"
#include "playheadlist.hpp"

extern "C" {
    #include <libavutil/avutil.h>
}

#include <catch2/catch_test_macros.hpp>


/**
 * @brief Creates a list of packets with increasing PTS by 1 of length <length>
 * 
 * @param list 
 * @param length 
 * @param time_base 
 */
void create_mock_playhead_packet_pts_list(PlayheadList<AVPacket*>& list, int length, AVRational time_base) {
    for (int i = 0; i < length; i++) {
        AVPacket* packet = av_packet_alloc();
        packet->pts = i;
        packet->time_base = time_base;
        list.push_back(packet);
    }

}

/**
 * @brief Creates a list of frames with increasing PTS by 1 of length <length>
 * 
 * @param list 
 * @param length 
 * @param time_base 
 */
void create_mock_playhead_frame_pts_list(PlayheadList<AVFrame*>& list, int length, AVRational time_base) {
    for (int i = 0; i < length; i++) {
        AVFrame* frame = av_frame_alloc();
        frame->pts = i;
        frame->time_base = time_base;
        list.push_back(frame);
    }
}

TEST_CASE("Media", "[functions]") {

    SECTION("Moving Playhead List by PTS") {
        AVRational TIME_BASE;
        TIME_BASE.num = 1;
        TIME_BASE.den = 75;

        const double TIME_BASE_DECIMAL = av_q2d(TIME_BASE);
        const int LIST_LENGTH = 10000;

        PlayheadList<AVPacket*> packet_pts_list;
        create_mock_playhead_packet_pts_list(packet_pts_list, LIST_LENGTH, TIME_BASE);   
        PlayheadList<AVFrame*> frame_pts_list;
        create_mock_playhead_frame_pts_list(frame_pts_list, LIST_LENGTH, TIME_BASE);


        

        SECTION("Mock Creation Test") {
            REQUIRE(packet_pts_list.get_length() == LIST_LENGTH);
            REQUIRE(frame_pts_list.get_length() == LIST_LENGTH);
            packet_pts_list.set_index(20);
            REQUIRE(packet_pts_list.get()->pts == 20);

            frame_pts_list.set_index(20);
            REQUIRE(frame_pts_list.get()->pts == 20);
        }


        
        SECTION("Movement by PTS") {
            const int TARGET_PTS = LIST_LENGTH / 2;
            // move_frame_list_to_pts(frame_pts_list, TARGET_PTS);
            // REQUIRE(frame_pts_list.get()->pts == TARGET_PTS);

            move_packet_list_to_pts(packet_pts_list, TARGET_PTS);
            REQUIRE(packet_pts_list.get_index() == TARGET_PTS);
            REQUIRE(packet_pts_list.get()->pts == TARGET_PTS);
        }

        SECTION("Movement by Time") {
            const int TARGET_PTS = LIST_LENGTH / 2;
            double TARGET_TIME = TARGET_PTS * TIME_BASE_DECIMAL;
            // move_frame_list_to_time_sec(frame_pts_list, TARGET_TIME);
            // REQUIRE(frame_pts_list.get()->pts == TARGET_PTS);

            move_packet_list_to_time_sec(packet_pts_list, TARGET_TIME);
            REQUIRE(packet_pts_list.get()->pts == TARGET_PTS);
            REQUIRE(packet_pts_list.get_index() == TARGET_PTS);
        }

        clear_playhead_frame_list(frame_pts_list);
        clear_playhead_packet_list(packet_pts_list);
        
        SECTION("Test clearing") {
            REQUIRE(frame_pts_list.is_empty() == true);
            REQUIRE(frame_pts_list.get_index() == -1);
            REQUIRE(packet_pts_list.is_empty() == true);
            REQUIRE(packet_pts_list.get_index() == -1);
        }

    }
}