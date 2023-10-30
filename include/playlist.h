#ifndef ASCII_VIDEO_PLAYLIST_H
#define ASCII_VIDEO_PLAYLIST_H

#include <cstddef>

#include <optional>
#include <vector>
#include <string>

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

/**
 * Currently, the files passed in at construction are immutable. This is of course
 * 
 * Currently, The passed in files at construction aren't checked for if they are actual valid
 * media file paths, if they are valid paths, or even if they are paths at all. The
 * user must do this beforehand
 * 
 * The loop type is not guaranteed to stay constant in between set_loop_type commands.
 * For example, when skipping with move(PlaylistMoveCommand::SKIP) while loop_type() == LoopType::REPEAT_ONE,
 * loop_type() will then be turned to REPEAT
*/
class Playlist {
  private:
    LoopType m_loop_type;
    std::vector<std::string> m_files;

    int m_current_index;
  public:

    Playlist();
    Playlist(std::vector<std::string> media_files, LoopType loop_type);

    std::size_t size() const noexcept;
    void set_loop_type(LoopType loop_type) noexcept;
    LoopType loop_type() noexcept;

    int index() const;
    std::string current();

    void move(PlaylistMoveCommand move_cmd);
    std::string peek_move(PlaylistMoveCommand move_cmd);
    bool can_move(PlaylistMoveCommand move_cmd) const noexcept;
};


#endif