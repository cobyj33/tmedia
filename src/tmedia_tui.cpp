#include "tmedia.h"

#include "playlist.h"
#include "tmedia_tui_elems.h"
#include "tmcurses.h"
#include "formatting.h"
#include "metadata.h"
#include "funcmac.h"

#include <stdexcept>
#include <fmt/format.h>

extern "C" {
  #include <curses.h>
}

static constexpr int MIN_RENDER_COLS = 2;
static constexpr int MIN_RENDER_LINES = 2;

const char* loop_type_str_short(LoopType loop_type);
std::string get_media_file_display_name(const std::string& abs_path, MetadataCache& mchc);

void TMediaCursesRenderer::render(const TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot) {
  if (snapshot.frame.get_width() != this->last_frame_dims.width || snapshot.frame.get_height() != this->last_frame_dims.height) {
    erase();
  }

  if (COLS < MIN_RENDER_COLS || LINES < MIN_RENDER_LINES) {
    erase();
  } else if (COLS <= 20 || LINES < 10 || tmps.fullscreen) {
      this->render_tui_fullscreen(tmps, snapshot);
  } else if (COLS < 60) {
    this->render_tui_compact(tmps, snapshot);
  } else {
    this->render_tui_large(tmps, snapshot);
  }

  this->last_frame_dims = Dim2(snapshot.frame.get_width(), snapshot.frame.get_height());
}

void TMediaCursesRenderer::render_tui_fullscreen(const TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot) {
  render_pixel_data(snapshot.frame, 0, 0, COLS, LINES, tmps.vom, tmps.scaling_algorithm, tmps.ascii_display_chars);
}

void TMediaCursesRenderer::render_tui_compact(const TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot) {
  static constexpr int CURRENT_FILE_NAME_MARGIN = 5;
  render_pixel_data(snapshot.frame, 2, 0, COLS, LINES - 4, tmps.vom, tmps.scaling_algorithm, tmps.ascii_display_chars);

  wfill_box(stdscr, 1, 0, COLS, 1, '~');
  werasebox(stdscr, 0, 0, COLS, 1);
  const std::string current_playlist_index_str = fmt::format("({}/{})", tmps.playlist.index() + 1, tmps.playlist.size());
  const std::string current_playlist_media_str = get_media_file_display_name(tmps.playlist.current(), this->metadata_cache);
  const std::string current_playlist_file_display = (tmps.playlist.size() > 1 ? (current_playlist_index_str + " ") : "") + current_playlist_media_str;
  TMLabelStyle current_playlist_display_style(0, 0, COLS, TMAlign::CENTER, CURRENT_FILE_NAME_MARGIN, CURRENT_FILE_NAME_MARGIN);
  tm_mvwaddstr_label(stdscr, current_playlist_display_style, current_playlist_file_display);

  if (tmps.playlist.size() > 1) {
    if (tmps.playlist.can_move(PlaylistMvCmd::REWIND)) {
      TMLabelStyle style(0, 0, COLS, TMAlign::LEFT, 0, 0);
      tm_mvwaddstr_label(stdscr, style, "<");
    }

    if (tmps.playlist.can_move(PlaylistMvCmd::SKIP)) {
      TMLabelStyle style(0, 0, COLS, TMAlign::RIGHT, 0, 0);
      tm_mvwaddstr_label(stdscr, style, ">");
    }
  }

  if (snapshot.media_type == MediaType::VIDEO || snapshot.media_type == MediaType::AUDIO) {
    wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
    wprint_playback_bar(stdscr, LINES - 2, 0, COLS, snapshot.media_time_secs, snapshot.media_duration_secs);

    std::vector<std::string> bottom_labels;
    const std::string playing_str = snapshot.playing ? ">" : "||";
    const std::string loop_str = loop_type_str_short(tmps.playlist.loop_type()); 
    const std::string volume_str = tmps.muted ? "M" : (fmt::format("{}%", static_cast<int>(tmps.volume * 100)));
    const std::string shuffled_str = tmps.playlist.shuffled() ? "S" : "NS";

    bottom_labels.push_back(playing_str);
    if (tmps.playlist.size() > 1) bottom_labels.push_back(shuffled_str);
    bottom_labels.push_back(loop_str);
    if (snapshot.has_audio_output) bottom_labels.push_back(volume_str);
    werasebox(stdscr, LINES - 1, 0, COLS, 1);
    wprint_labels(stdscr, bottom_labels, LINES - 1, 0, COLS);
  }

}

void TMediaCursesRenderer::render_tui_large(const TMediaProgramState& tmps, const TMediaProgramSnapshot& snapshot) {
  static constexpr int CURRENT_FILE_NAME_MARGIN = 5;
  render_pixel_data(snapshot.frame, 2, 0, COLS, LINES - 4, tmps.vom, tmps.scaling_algorithm, tmps.ascii_display_chars);

  werasebox(stdscr, 0, 0, COLS, 2);
  const std::string current_playlist_index_str = fmt::format("({}/{})", tmps.playlist.index() + 1, tmps.playlist.size());
  const std::string current_playlist_media_str = get_media_file_display_name(tmps.playlist.current(), this->metadata_cache);
  const std::string current_playlist_file_display = (tmps.playlist.size() > 1 ? (current_playlist_index_str + " ") : "") + current_playlist_media_str;
  TMLabelStyle current_playlist_display_style(0, 0, COLS, TMAlign::CENTER, CURRENT_FILE_NAME_MARGIN, CURRENT_FILE_NAME_MARGIN);
  tm_mvwaddstr_label(stdscr, current_playlist_display_style, current_playlist_file_display);

  if (tmps.playlist.size() == 1) {
    wfill_box(stdscr, 1, 0, COLS, 1, '~');
  } else {
    static constexpr int MOVE_FILE_NAME_MIDDLE_MARGIN = 5;
    wfill_box(stdscr, 2, 0, COLS, 1, '~');
    if (tmps.playlist.can_move(PlaylistMvCmd::REWIND)) {
      werasebox(stdscr, 1, 0, COLS / 2, 1);
      const std::string rewind_media_file_display_string = get_media_file_display_name(tmps.playlist.peek_move(PlaylistMvCmd::REWIND), this->metadata_cache);
      const std::string rewind_display_string = fmt::format("< {}", rewind_media_file_display_string);
      TMLabelStyle rewind_display_style(1, 0, COLS / 2, TMAlign::LEFT, 0, MOVE_FILE_NAME_MIDDLE_MARGIN);
      tm_mvwaddstr_label(stdscr, rewind_display_style, rewind_display_string);
    }

    if (tmps.playlist.can_move(PlaylistMvCmd::SKIP)) {
      static constexpr int RIGHT_ARROW_MARGIN = 3;
      werasebox(stdscr, 1, COLS / 2, COLS / 2, 1);
      const std::string skip_display_string = get_media_file_display_name(tmps.playlist.peek_move(PlaylistMvCmd::SKIP), this->metadata_cache);
      TMLabelStyle skip_display_string_style(1, COLS / 2, COLS / 2, TMAlign::RIGHT, MOVE_FILE_NAME_MIDDLE_MARGIN, RIGHT_ARROW_MARGIN);
      TMLabelStyle right_arrow_string_style(1, COLS / 2, COLS / 2, TMAlign::RIGHT, 0, 0);
      tm_mvwaddstr_label(stdscr, skip_display_string_style, skip_display_string);
      tm_mvwaddstr_label(stdscr, right_arrow_string_style, " >");
    }
  }

  if (snapshot.media_type == MediaType::VIDEO || snapshot.media_type == MediaType::AUDIO) {
    wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
    wprint_playback_bar(stdscr, LINES - 2, 0, COLS, snapshot.media_time_secs, snapshot.media_duration_secs);

    std::vector<std::string> bottom_labels;
    const std::string playing_str = snapshot.playing ? "PLAYING" : "PAUSED";
    const std::string loop_str = str_capslock(loop_type_str(tmps.playlist.loop_type())); 
    const std::string volume_str = tmps.muted ? "MUTED" : fmt::format("VOLUME: {}%", static_cast<int>(tmps.volume * 100));
    const std::string shuffled_str = tmps.playlist.shuffled() ? "SHUFFLED" : "NOT SHUFFLED";

    bottom_labels.push_back(playing_str);
    if (tmps.playlist.size() > 1) bottom_labels.push_back(shuffled_str);
    bottom_labels.push_back(loop_str);
    if (snapshot.has_audio_output) bottom_labels.push_back(volume_str);
    werasebox(stdscr, LINES - 1, 0, COLS, 1);
    wprint_labels(stdscr, bottom_labels, LINES - 1, 0, COLS);
  }
}

const char* loop_type_str_short(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "NL";
    case LoopType::REPEAT: return "R";
    case LoopType::REPEAT_ONE: return "RO";
  }
  throw std::runtime_error(fmt::format("[{}] Could not identify loop_type", FUNCDINFO));
}

std::string get_media_file_display_name(const std::string& abs_path, MetadataCache& mchc) {
  mchc_cache(abs_path, mchc);
  bool has_artist = mchc_has(abs_path, "artist", mchc);
  bool has_title = mchc_has(abs_path, "title", mchc);

  if (has_artist && has_title) {
    return fmt::format("{} - {}", mchc_get(abs_path, "artist", mchc), mchc_get(abs_path, "title", mchc));
  } else if (has_title) {
    return std::string(mchc_get(abs_path, "title", mchc));
  }
  return to_filename(abs_path);
}