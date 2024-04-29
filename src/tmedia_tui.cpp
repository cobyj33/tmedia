#include <tmedia/tmedia.h>

#include <tmedia/tmcurses/tmcurses.h>
#include <tmedia/tmedia_tui_elems.h>
#include <tmedia/util/formatting.h>
#include <tmedia/media/metadata.h>
#include <tmedia/util/defines.h>

#include <stdexcept>
#include <fmt/format.h>

extern "C" {
  #include <curses.h>
}


const char* loop_type_cstr_short(LoopType loop_type);
std::string get_media_file_display_name(const std::string& abs_path, MetadataCache& mchc);
void render_pixel_data(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VidOutMode output_mode, const ScalingAlgo scaling_algorithm, std::string_view ascii_char_map);

void render_tui(TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  static constexpr int MIN_RENDER_COLS = 2;
  static constexpr int MIN_RENDER_LINES = 2;
  
  if (sshot.frame.get_width() != tmrs.last_frame_dims.width ||
      sshot.frame.get_height() != tmrs.last_frame_dims.height) {
    erase();
  }

  if (COLS < MIN_RENDER_COLS || LINES < MIN_RENDER_LINES) {
    erase();
  } else if (COLS <= 20 || LINES < 10 || tmps.fullscreen) {
    render_tui_fullscreen(tmps, sshot, tmrs);
  } else if (COLS < 60) {
    render_tui_compact(tmps, sshot, tmrs);
  } else {
    render_tui_large(tmps, sshot, tmrs);
  }

  tmrs.last_frame_dims = Dim2(sshot.frame.get_width(), sshot.frame.get_height());
}

void render_tui_fullscreen(TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  render_pixel_data(sshot.frame, 0, 0, COLS, LINES, tmps.vom, tmps.scaling_algorithm, tmps.ascii_display_chars);
  tmps.req_frame_dim = Dim2(COLS, LINES);
  (void)tmrs;
}

void render_tui_compact(TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  static constexpr int CURRENT_FILE_NAME_MARGIN = 5;
  render_pixel_data(sshot.frame, 2, 0, COLS, LINES - 4, tmps.vom, tmps.scaling_algorithm, tmps.ascii_display_chars);
  tmps.req_frame_dim = Dim2(COLS, LINES - 4);

  wfill_box(stdscr, 1, 0, COLS, 1, '~');
  werasebox(stdscr, 0, 0, COLS, 1);
  const std::string current_plist_index_str = fmt::format("({}/{})", tmps.plist.index() + 1, tmps.plist.size());
  const std::string current_plist_media_str = get_media_file_display_name(tmps.plist.current(), tmrs.metadata_cache);
  const std::string current_plist_file_display = (tmps.plist.size() > 1 ? (current_plist_index_str + " ") : "") + current_plist_media_str;
  TMLabelStyle current_plist_display_style(0, 0, COLS, TMAlign::CENTER, CURRENT_FILE_NAME_MARGIN, CURRENT_FILE_NAME_MARGIN);
  tm_mvwaddstr_label(stdscr, current_plist_display_style, current_plist_file_display);

  if (tmps.plist.size() > 1) {
    if (tmps.plist.can_move(PlaylistMvCmd::REWIND)) {
      TMLabelStyle style(0, 0, COLS, TMAlign::LEFT, 0, 0);
      tm_mvwaddstr_label(stdscr, style, "<");
    }

    if (tmps.plist.can_move(PlaylistMvCmd::SKIP)) {
      TMLabelStyle style(0, 0, COLS, TMAlign::RIGHT, 0, 0);
      tm_mvwaddstr_label(stdscr, style, ">");
    }
  }

  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
    wprint_playback_bar(stdscr, LINES - 2, 0, COLS, sshot.media_time_secs, sshot.media_duration_secs);

    std::vector<std::string_view> bottom_labels;
    const std::string_view playing_str = sshot.playing ? ">" : "||";
    const std::string_view loop_str = loop_type_cstr_short(tmps.plist.loop_type()); 
    const std::string volume_str = tmps.muted ? "M" : (fmt::format("{}%", static_cast<int>(tmps.volume * 100)));
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "S" : "NS";

    bottom_labels.push_back(playing_str);
    if (tmps.plist.size() > 1) bottom_labels.push_back(shuffled_str);
    bottom_labels.push_back(loop_str);
    if (sshot.has_audio_output) bottom_labels.push_back(volume_str);
    werasebox(stdscr, LINES - 1, 0, COLS, 1);
    wprint_labels(stdscr, bottom_labels, LINES - 1, 0, COLS);
  }

}

void render_tui_large(TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  static constexpr int CURRENT_FILE_NAME_MARGIN = 5;
  render_pixel_data(sshot.frame, 2, 0, COLS, LINES - 4, tmps.vom, tmps.scaling_algorithm, tmps.ascii_display_chars);
  tmps.req_frame_dim = Dim2(COLS, LINES - 4);
  
  werasebox(stdscr, 0, 0, COLS, 2);
  const std::string current_plist_index_str = fmt::format("({}/{})", tmps.plist.index() + 1, tmps.plist.size());
  const std::string current_plist_media_str = get_media_file_display_name(tmps.plist.current(), tmrs.metadata_cache);
  const std::string current_plist_file_display = (tmps.plist.size() > 1 ? (current_plist_index_str + " ") : "") + current_plist_media_str;
  TMLabelStyle current_plist_display_style(0, 0, COLS, TMAlign::CENTER, CURRENT_FILE_NAME_MARGIN, CURRENT_FILE_NAME_MARGIN);
  tm_mvwaddstr_label(stdscr, current_plist_display_style, current_plist_file_display);

  if (tmps.plist.size() == 1) {
    wfill_box(stdscr, 1, 0, COLS, 1, '~');
  } else {
    static constexpr int MOVE_FILE_NAME_MIDDLE_MARGIN = 5;
    wfill_box(stdscr, 2, 0, COLS, 1, '~');
    if (tmps.plist.can_move(PlaylistMvCmd::REWIND)) {
      werasebox(stdscr, 1, 0, COLS / 2, 1);
      const std::string rewind_media_file_display_string = get_media_file_display_name(tmps.plist.peek_move(PlaylistMvCmd::REWIND), tmrs.metadata_cache);
      const std::string rewind_display_string = fmt::format("< {}", rewind_media_file_display_string);
      TMLabelStyle rewind_display_style(1, 0, COLS / 2, TMAlign::LEFT, 0, MOVE_FILE_NAME_MIDDLE_MARGIN);
      tm_mvwaddstr_label(stdscr, rewind_display_style, rewind_display_string);
    }

    if (tmps.plist.can_move(PlaylistMvCmd::SKIP)) {
      static constexpr int RIGHT_ARROW_MARGIN = 3;
      werasebox(stdscr, 1, COLS / 2, COLS / 2, 1);
      const std::string skip_display_string = get_media_file_display_name(tmps.plist.peek_move(PlaylistMvCmd::SKIP), tmrs.metadata_cache);
      TMLabelStyle skip_display_string_style(1, COLS / 2, COLS / 2, TMAlign::RIGHT, MOVE_FILE_NAME_MIDDLE_MARGIN, RIGHT_ARROW_MARGIN);
      TMLabelStyle right_arrow_string_style(1, COLS / 2, COLS / 2, TMAlign::RIGHT, 0, 0);
      tm_mvwaddstr_label(stdscr, skip_display_string_style, skip_display_string);
      tm_mvwaddstr_label(stdscr, right_arrow_string_style, " >");
    }
  }

  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
    wprint_playback_bar(stdscr, LINES - 2, 0, COLS, sshot.media_time_secs, sshot.media_duration_secs);

    std::vector<std::string_view> bottom_labels;
    const std::string_view playing_str = sshot.playing ? "PLAYING" : "PAUSED";
    const std::string loop_str = str_capslock(loop_type_cstr(tmps.plist.loop_type())); 
    const std::string volume_str = tmps.muted ? "MUTED" : fmt::format("VOLUME: {}%", static_cast<int>(tmps.volume * 100));
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "SHUFFLED" : "NOT SHUFFLED";

    bottom_labels.push_back(playing_str);
    if (tmps.plist.size() > 1) bottom_labels.push_back(shuffled_str);
    bottom_labels.push_back(loop_str);
    if (sshot.has_audio_output) bottom_labels.push_back(volume_str);
    werasebox(stdscr, LINES - 1, 0, COLS, 1);
    wprint_labels(stdscr, bottom_labels, LINES - 1, 0, COLS);
  }
}

const char* loop_type_cstr_short(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "NL";
    case LoopType::REPEAT: return "R";
    case LoopType::REPEAT_ONE: return "RO";
  }
  return "UNK";
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

  return std::filesystem::path(abs_path).filename().string();
}

void render_pixel_data(const PixelData& pixel_data, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VidOutMode output_mode, const ScalingAlgo scaling_algorithm, std::string_view ascii_char_map) {
  if (!tmcurses_has_colors()) // if there are no colors, just don't print colors :)
    output_mode = VidOutMode::PLAIN;

  switch (output_mode) {
    case VidOutMode::PLAIN: return render_pixel_data_plain(pixel_data, bounds_row, bounds_col, bounds_width, bounds_height, scaling_algorithm, ascii_char_map);
    case VidOutMode::COLOR:
    case VidOutMode::GRAY: return render_pixel_data_color(pixel_data, bounds_row, bounds_col, bounds_width, bounds_height, scaling_algorithm, ascii_char_map);
    case VidOutMode::COLOR_BG:
    case VidOutMode::GRAY_BG: return render_pixel_data_bg(pixel_data, bounds_row, bounds_col, bounds_width, bounds_height, scaling_algorithm);
  }
}