#ifndef TMEDIA_PLAYLIST_H
#define TMEDIA_PLAYLIST_H

#include <cstddef>

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>

enum class PlaylistMoveCommand {
  NEXT,
  SKIP,
  REWIND
};

std::string playlist_move_cmd_str(PlaylistMoveCommand move_cmd);

enum class LoopType {
  REPEAT_ONE,
  REPEAT,
  NO_LOOP
};

std::string loop_type_str(LoopType loop_type);
LoopType loop_type_from_str(std::string_view loop_type);
bool loop_type_str_is_valid(std::string_view loop_type);

/**
 * Currently, the files passed in at construction are immutable, as files cannot
 * be added or removed.
 * 
 * Currently, The passed in files at construction aren't checked for if they are actual valid
 * media file paths, if they are valid paths, or even if they are paths at all. The
 * user must do this beforehand
 * 
 * The loop type is not guaranteed to stay constant in between set_loop_type commands.
 * For example, when skipping with move(PlaylistMoveCommand::SKIP) while loop_type() == LoopType::REPEAT_ONE,
 * loop_type() will then be turned to REPEAT
*/

/**
 * 
 * 
 * A playlist can additionally enqueue and dequeue files for immediate playback.
 * 
 * 
 * Desired Behavior:
 * 
 * default:
 * 
 * loop->no_loop (default state):
 *   next:
 *     If there is no next file in the playlist, then the next command is invalid
 *     If there is a next file, continue onto that next file
 *   skip:
 *     If there is no next file in the playlist, then the skip command is invalid
 *     If there is a next file, skip to that next file
 *   rewind:
 *     If there is no previous value, then remain on the current file
 *     
 * loop->repeat: 
 *   next:
 *     If there is no next file in the playlist, then the next command is invalid
 *     If there is a next file, continue onto that next file
 *   skip:
 *     If there is no next file in the playlist, then the skip command is invalid
 *     If there is a next file, skip to that next file
 *   rewind:
 *     If there is no previous value, then remain on the current file
 * 
 * loop->repeat_one: 
 *   next:
 *     Simply repeat the current file
 *   skip:
 *     Simply repeat the current file!
 *   rewind:
 *     Simply repeat the current file!
 *  
 *   * note that the 
 * 
 * 
 * shuffle -> on:
 *  loop->no_loop: 
 *  loop->repeat: 
 *  loop->repeat_one:
*/


// class Playlist {
//   private:
//     std::vector<std::filesystem::path> m_files;

//     std::vector<int> m_queue_indexes;
//     int m_queue_index;
//     circular_stack<int, 100> played;
//     

//     LoopType m_loop_type;
//     bool m_shuffled;
//   public:

//     Playlist();
//     Playlist(std::vector<std::string> media_files, LoopType loop_type);

//     bool shuffled() const;
//     void shuffle(bool keep_current_file_first);
//     void unshuffle();

//     std::size_t size() const noexcept;
//     void set_loop_type(LoopType loop_type) noexcept;
//     LoopType loop_type() const noexcept;

//     int index() const;
//     std::string current() const;

//     void move(PlaylistMoveCommand move_cmd);
//     std::string peek_move(PlaylistMoveCommand move_cmd) const;
//     bool can_move(PlaylistMoveCommand move_cmd) const noexcept;
// };

class IPlaylist {
  virtual void size() = 0;
  virtual void loop_type() = 0;
  virtual void set_loop_type() = 0;
  virtual int index() const = 0;
  virtual std::filesystem::path current() const = 0;

  virtual void move(PlaylistMoveCommand move_cmd) = 0;
  virtual std::filesystem::path peek_move(PlaylistMoveCommand move_cmd) const = 0;
  virtual bool can_move(PlaylistMoveCommand move_cmd) const noexcept = 0;
};

// class UnshuffledPlaylist {

// };

template <typename T>
class Playlist {
  private:
    std::vector<T> m_entries;

    std::vector<int> m_queue_indexes;
    int m_queue_index;

    LoopType m_loop_type;
    bool m_shuffled;
  public:

    Playlist();
    Playlist(std::vector<T> entries, LoopType loop_type);

    bool shuffled() const;
    void shuffle(bool keep_current_file_first);
    void unshuffle();

    std::size_t size() const noexcept;
    void set_loop_type(LoopType loop_type) noexcept;
    LoopType loop_type() const noexcept;

    int index() const;
    T current() const;

    void move(PlaylistMoveCommand move_cmd);
    T peek_move(PlaylistMoveCommand move_cmd) const;
    bool can_move(PlaylistMoveCommand move_cmd) const noexcept;
};


#endif