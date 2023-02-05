// #include <string>

// #include "formatting.h"
// #include "media.h"
// #include "debug.h"


// void vadd_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* message_desc, const char* format, va_list args);

// DebugMessage::DebugMessage(std::string message, std::string source, DebugMessageType type, std::string description) {
//     this->message = std::string(message);
//     this->source = std::string(source);
//     this->type = std::string(type);
//     this->description = std::string(description);
// }

// std::string DebugMessage::get_message() { return std::string(this->message); }
// std::string DebugMessage::get_source() { return std::string(this->source); }
// std::string DebugMessage::get_type() { return this->type; }
// std::string DebugMessage::get_description() { return std::string(this->description); }


// void MediaDebugInfo::add_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc, std::string& format, ...) {
//     int to_replace = this->find_debug_message(message_source, message_type, message_desc);
//     va_list args;
//     va_start(args, format);
//     std::string message = vget_formatted_string(format, args);
//     if (this->has_debug_message(message_source, message_type, message_desc)) {
//         int to_replace = this->find_debug_message(message_source, message_type, message_desc);
//         this->messages[to_replace] = DebugMessage(message, message_source, message_type, message_desc);
//     } else {
//         this->messages.push_back(DebugMessage(message, message_source, message_type, message_desc));
//     }
//     va_end(args);
// }

// void MediaDebugInfo::clear(std::string source, DebugMessageType type) {

// }

// void MediaDebugInfo::clear() {

// }

// void clear_media_debug(MediaDebugInfo* debug, const char* source, const char* type) {
//     DebugMessage* saved_messages[debug->nb_messages];
//     int nb_saved_messages = 0;
//     for (int i = 0; i < debug->nb_messages; i++) {
//         if (( strlen(source) == 0 || strcmp(debug->messages[i]->source, source) == 0) && (strlen(type) == 0 || strcmp(debug->messages[i]->type, type) == 0)) {
//             free(debug->messages[i]->message);
//             free(debug->messages[i]);
//         } else {
//             saved_messages[nb_saved_messages] = debug->messages[i];
//             nb_saved_messages++;
//         }
//     }

//     debug->nb_messages = nb_saved_messages;
//     for (int i = 0; i < nb_saved_messages; i++) {
//         debug->messages[i] = saved_messages[i];
//     }
// }

// void remove_debug_message(MediaDebugInfo* debug, std::string& source, DebugMessageType type, std::string& desc) {
//     int to_replace = find_debug_message(debug, source, type, desc);
//     if (to_replace != -1) {
//         free(debug->messages[to_replace]->message);
//         free(debug->messages[to_replace]);
//         for (int i = to_replace + 1; i < debug->nb_messages; i++) {
//             debug->messages[i - 1] = debug->messages[i];
//         }
//         debug->nb_messages--;
//     }
// }



// int MediaDebugInfo::find_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc) {
//     for (int i = 0; i < this->messages.size(); i++) {
//         DebugMessage current_message = this->messages[i];
//         std::string test;
//         if (message_source == current_message.get_source() && message_type == current_message.get_type() && message_desc == current_message.get_description()) {
//             return i;
//         }
//     }
//     throw std::runtime_error("No Debug Message could be found with source of \"" + message_source + "\", type of \"" + get_debug_message_type_string(message_type) + "\", and description of \"" + message_desc + "\".");
// }

// bool MediaDebugInfo::has_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc) {
//     try {
//         this->find_debug_message(message_source, message_type, message_desc);
//         return true;
//     } catch (std::runtime_error e) {
//         return false;
//     }
// }

// void MediaDebugInfo::vadd_debug_message(std::string& message_source, DebugMessageType message_type, std::string& message_desc, std::string& format, va_list args) {
//     int to_replace = find_debug_message(debug_info, message_source, message_type, message_desc);

//     va_list writing_args, reading_args;
//     va_copy(reading_args, args);
//     va_copy(writing_args, args);

//     size_t alloc_size = vsnprintf(NULL, 0, format, reading_args);
//     va_end(reading_args);
//     int capacity = (sizeof(char) * (alloc_size) + 1) * 2;
//     char* string = (char*)malloc(capacity);
//     if (string == NULL) {
//         va_end(writing_args);
//         return;
//     }

//     vsnprintf(string, alloc_size + 1, format, writing_args);
//     va_end(writing_args);

//     if (to_replace == -1) {
//         DebugMessage* debug_message = (DebugMessage*)malloc(sizeof(DebugMessage));
//         debug_message->source = message_source;
//         debug_message->type = message_type;
//         debug_message->desc = message_desc;

//         debug_message->message = string;
//         debug_message->size = alloc_size;
//         debug_message->capacity = capacity;
//         debug_info->messages[debug_info->nb_messages] = debug_message;
//         debug_info->nb_messages++;
//     } else {
//         free(debug_info->messages[to_replace]->message);
//         debug_info->messages[to_replace]->message = string;
//         debug_info->messages[to_replace]->size = alloc_size;
//         debug_info->messages[to_replace]->capacity = capacity;
//     }
// }

// std::string get_debug_message_type_string(DebugMessageType type) {
//     switch (type) {
//         case DebugMessageType::ERROR: return "error";
//         case DebugMessageType::INFO: return "info";
//         case DebugMessageType::SUCCESS: return "success";
//         case DebugMessageType::WARNING: return "warning";
//     }
//     throw std::runtime_error("Could not get debug message type string for debug message type" + std::to_string((int)type));
// }