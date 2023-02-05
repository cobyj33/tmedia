#ifndef ASCII_VIDEO_DEBUG
#define ASCII_VIDEO_DEBUG
#include "boiler.h"
#include <string>
#include <vector>
#include <cstdarg>

// #define MEDIA_DEBUG_MESSAGE_BUFFER_SIZE 1000

// enum class DebugMessageType {
//     INFO,
//     WARNING,
//     ERROR,
//     SUCCESS
// };

// class DebugMessage {
//     private:
//         std::string message;
//         std::string source;

//         /**
//          * @brief The type of the message (Debug, Error, Warning, etc...)
//          */
//         DebugMessageType type;
//         std::string description;
//     public:
//         DebugMessage(std::string message, std::string source, DebugMessageType type, std::string description) {
//             this->message = std::string(message);
//             this->source = std::string(source);
//             this->type = type;
//             this->description = std::string(description);
//         }

//         std::string get_message();
//         std::string get_source();
//         DebugMessageType get_type();
//         std::string get_description();
// };

// /**
//  * @brief NOTE: NO UPPER LIMIT ON AMOUNT OF MESSAGES ANYMORE
//  */
// class MediaDebugInfo {
//     private:
//         std::vector<DebugMessage> messages;
//         int max_unique_messages;

//         int find_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc);
//         bool has_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc);
//         void vadd_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc, std::string& format, va_list args);

//     public:
//         MediaDebugInfo(int max_unique_messages);

//         int get_nb_messages();

//         void clear(MediaDebugInfo* debug, std::string& source, DebugMessageType type);
//         void clear();
//         void clear(DebugMessageType type);
//         void clear(std::string& source);

//         void add_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc, std::string& format, ...);
//         void remove_debug_message(std::string& source, DebugMessageType type, std::string& desc);
// };

// std::string get_debug_message_type_string(DebugMessageType type);
#endif
