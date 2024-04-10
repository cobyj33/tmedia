#ifndef TMEDIA_PLAYLIST_H
#define TMEDIA_PLAYLIST_H

#include <cstddef>

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>

enum class PlaylistMvCmd {
  NEXT,
  SKIP,
  REWIND
};

const char* playlist_move_cmd_cstr(PlaylistMvCmd move_cmd);

enum class LoopType {
  REPEAT_ONE,
  REPEAT,
  NO_LOOP
};

const char* loop_type_cstr(LoopType loop_type);
LoopType loop_type_from_str(std::string_view loop_type);

/**
 * Currently, the files passed in at construction are immutable, as files cannot
 * be added or removed.
 * 
 * Currently, The passed in files at construction aren't checked for if they are actual valid
 * media file paths, if they are valid paths, or even if they are paths at all. The
 * user must do this beforehand
 * 
 * The loop type is not guaranteed to stay constant in between set_loop_type commands.
 * For example, when skipping with move(PlaylistMvCmd::SKIP) while loop_type() == LoopType::REPEAT_ONE,
 * loop_type() will then be turned to REPEAT
*/

/**
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

class Playlist {
  private:
    // class PlaylistEntry {
    //   std::filesystem::path path;
    //   int orig_ind;
    //   PlaylistEntry() {}
    //   PlaylistEntry(const std::filesystem::path& path, int i) : path(path), orig_ind(i) {} 
    // };

    std::vector<std::filesystem::path> m_entries;

    std::vector<int> m_q;
    int m_qi;

    LoopType m_loop_type;
    bool m_shuffled;

  public:

    Playlist();
    Playlist(const std::vector<std::filesystem::path>& entries, LoopType loop_type);

    inline bool shuffled() const noexcept {
      return this->m_shuffled;
    }

    inline LoopType loop_type() const noexcept {
      return this->m_loop_type;
    }

    inline void set_loop_type(LoopType loop_type) noexcept {
      this->m_loop_type = loop_type;
    }

    inline std::size_t size() const noexcept {
      return this->m_entries.size();
    }

    void shuffle(bool keep_current_file_first);
    void unshuffle();

    int index() const;
    const std::filesystem::path& current() const;

    void insert(const std::filesystem::path& entry, std::size_t i);
    void push_back(const std::filesystem::path& entry);
    void remove(std::size_t i);
    void remove(const std::filesystem::path& entry);
    bool has(const std::filesystem::path& entry) const;
    void clear();

    const std::filesystem::path& at(std::size_t i) const;
    const std::vector<std::filesystem::path>& view() const;

    const std::filesystem::path& operator[](std::size_t i);

    void move(PlaylistMvCmd move_cmd);
    const std::filesystem::path& peek_move(PlaylistMvCmd move_cmd) const;
    bool can_move(PlaylistMvCmd move_cmd) const noexcept;
};


#endif