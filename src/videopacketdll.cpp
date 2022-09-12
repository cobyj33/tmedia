#include <videopacketdll.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

typedef struct AVPacketNode {
    AVPacket* data;
    AVPacket** next;
    AVPacket** prev;
} AVPacketNode;

struct avpacketdllist {
    AVPacketNode* first;
    AVPacketNode* last;
    AVPacketNode* current;
    int length;
    int index;
};
