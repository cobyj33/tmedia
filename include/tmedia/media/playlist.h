#ifndef TMEDIA_PLAYLIST_H
#define TMEDIA_PLAYLIST_H

#include <cstddef>

#include <tmedia/media/metadata.h>

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <array>

extern "C" {
#include <libavutil/avutil.h> // AVMEDIA_TYPE_NB
}

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

struct PlaylistItem {
  std::filesystem::path path;
  std::array<bool, AVMEDIA_TYPE_NB> requested_streams = {true, true, false, false, false};
  PlaylistItem() = default;
  explicit PlaylistItem(std::filesystem::path _path) : path(_path) {}
  explicit PlaylistItem(std::filesystem::path _path, std::array<bool, AVMEDIA_TYPE_NB>& _requested_streams) : path(_path) {
    for (std::size_t i = 0; i < requested_streams.size(); i++)
      this->requested_streams[i] = _requested_streams[i];
  }
};

class Playlist {
  private:
    std::vector<PlaylistItem> m_entries;
    std::vector<std::size_t> m_q;
    std::size_t m_qi;

    LoopType m_loop_type;
    bool m_shuffled;

    void remove_at_entry_idx(std::size_t i);
  public:
    static constexpr std::size_t npos = (std::size_t)-1;

    Playlist();
    Playlist(const std::vector<PlaylistItem>& entries, LoopType loop_type);

    [[gnu::always_inline]] inline bool shuffled() const noexcept {
      return this->m_shuffled;
    }

    [[gnu::always_inline]] inline LoopType loop_type() const noexcept {
      return this->m_loop_type;
    }

    [[gnu::always_inline]] inline void set_loop_type(LoopType loop_type) noexcept {
      this->m_loop_type = loop_type;
    }

    [[gnu::always_inline]] inline std::size_t size() const noexcept {
      return this->m_entries.size();
    }

    [[gnu::always_inline]] bool empty() const noexcept {
      return this->size() == 0;
    }

    void shuffle(bool keep_current_file_first);
    void unshuffle();

    int index() const;
    const PlaylistItem& current() const;

    void insert(const PlaylistItem& entry, std::size_t i);
    void push_back(const PlaylistItem& entry);
    void remove(std::size_t i);
    void remove(const PlaylistItem& entry);
    bool has(const PlaylistItem& entry) const;
    void clear();

    const std::vector<PlaylistItem>& view() const;

    const PlaylistItem& operator[](std::size_t i) const;
    const PlaylistItem& at(std::size_t i) const;

    /**
     * After a move, it is guaranteed that the next file will not be the
     * same as the previous file
    */
    void move(PlaylistMvCmd move_cmd);
    const PlaylistItem& peek_move(PlaylistMvCmd move_cmd) const;
    bool can_move(PlaylistMvCmd move_cmd) const noexcept;
};


#endif
