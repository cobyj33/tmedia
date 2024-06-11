#include <tmedia/tmedia.h>

#include <tmedia/tmcurses/tmcurses.h>
#include <tmedia/tmedia_tui_elems.h>
#include <tmedia/util/formatting.h>
#include <tmedia/media/metadata.h>
#include <tmedia/util/defines.h>

#include <array>
#include <stdexcept>
#include <fmt/format.h>
#include <cassert>

extern "C" {
  #include <curses.h>
}

/**
 * MIN_RENDER_COLS and MIN_RENDER_LINES are the minimum columns and lines
 * necessary for any tui elements to be rendered.
*/

static constexpr int MIN_RENDER_COLS = 5;
static constexpr int MIN_RENDER_LINES = 2;
static constexpr std::size_t METADATA_CACHE_MAX_SIZE = 30;

const char* loop_type_cstr_short(LoopType loop_type);
std::string_view vidoutmode_strv(VidOutMode vom);
std::string_view get_media_file_display_name(const std::filesystem::path& abs_path, MetadataCache& mchc);
void render_pixel_data(PixelData& pixel_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VidOutMode output_mode, std::string_view ascii_char_map);
void render_playback_info_bar(int line, std::string_view playing_str, std::string_view loop_str, std::string_view volume_str, std::string_view shuffled_str);
int render_current_filenamex(const TMediaProgramState& tmps, TMediaRendererState& tmrs, int line);
int render_current_filename(std::size_t index, std::size_t plist_size, std::string_view display_name, int line);

void try_render_frame(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs, int bounds_row, int bounds_col, int bounds_width, int bounds_height);
void render_current_filenamex_compact(const TMediaProgramState& tmps, TMediaRendererState& tmrs, int line);
int render_nexprev_files_large(const TMediaProgramState& tmps, TMediaRendererState& tmrs, int line);

/**
 * Functions for rendering different screens
 */
void render_tui_fullscreen(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);
void render_tui_compact(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);
void render_tui_compact_help(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);
void render_tui_large(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);
void render_tui_large_help(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);

void render_tui(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  assert(sshot.frame != nullptr);
  if (sshot.frame->m_width != tmrs.last_frame_dims.width ||
      sshot.frame->m_height != tmrs.last_frame_dims.height) {
    erase();
  }

  // naive way to manage the metadata cache size.
  // The metadata cache should only ever grow whenever listing
  // If that happens to change anytime later in tmedia's development,
  // then a better caching mechanism will of course have to be used.
  if (tmrs.metadata_cache.size() > METADATA_CACHE_MAX_SIZE) {
    tmrs.metadata_cache.clear();
  }

  if (COLS < MIN_RENDER_COLS || LINES < MIN_RENDER_LINES) {
    erase();
  } else if (COLS <= 20 || LINES < 10 || tmps.fullscreen) {
    render_tui_fullscreen(tmps, sshot, tmrs);
  } else if (COLS < 60) {
    if (tmps.show_ctrl_info)
      render_tui_compact_help(tmps, sshot, tmrs);
    else
      render_tui_compact(tmps, sshot, tmrs);
  } else {
    if (tmps.show_ctrl_info)
      render_tui_large_help(tmps, sshot, tmrs);
    else
      render_tui_large(tmps, sshot, tmrs);
  }

  tmrs.last_frame_dims = { sshot.frame->m_width, sshot.frame->m_height };
}

// As expected, just blits the entire frame onto the full terminal window.
void render_tui_fullscreen(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  try_render_frame(tmps, sshot, tmrs, 0, 0, COLS, LINES);
}

// Design: (see tui_design.md in the doc/ folder for prototype designs)
// <              01. Palace             >
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
//        VIDEO/AUDIO VISUALIZATION
//
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 00:00 / 26:54 @@@----------------------
// >          NL          NS          50%

void render_tui_compact(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  render_current_filenamex_compact(tmps, tmrs, 0);
  wfill_box(stdscr, 1, 0, COLS, 1, '~');
  try_render_frame(tmps, sshot, tmrs, 2, 0, COLS, LINES - 4);

  wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
  wprint_playback_bar(stdscr, LINES - 2, 0, COLS, sshot.media_time_secs, sshot.media_duration_secs, false);
  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    const std::string_view playing_str = sshot.playing ? ">" : "||";
    const std::string_view loop_str = loop_type_cstr_short(tmps.plist.loop_type());
    const std::string volume_str = sshot.has_audio_output ? (tmps.muted ? "M" : (fmt::format("{}%", static_cast<int>(tmps.volume * 100)))) : "";
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "S" : "NS";
    render_playback_info_bar(LINES - 1, playing_str, shuffled_str, loop_str, volume_str);
  }
}

// Help Design:
// < (P)        01. Palace          (N) >
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
//        VIDEO/AUDIO VISUALIZATION
//
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 00:00 / 26:54 @@@----------------------
// > (SPACE)  NL (L) NS (S)  50% (UP/DOWN)

void render_tui_compact_help(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  render_current_filenamex_compact(tmps, tmrs, 0);
  wfill_box(stdscr, 1, 0, COLS, 1, '~');
  try_render_frame(tmps, sshot, tmrs, 2, 0, COLS, LINES - 4);
  wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
  wprint_playback_bar(stdscr, LINES - 2, 0, COLS, sshot.media_time_secs, sshot.media_duration_secs, true);
  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    const std::string_view playing_str = sshot.playing ? "> (SPACE)" : "|| (SPACE)";
    const std::string loop_str = fmt::format("{} (L)", loop_type_cstr_short(tmps.plist.loop_type()));
    const std::string volume_str = sshot.has_audio_output ? (tmps.muted ? "M (M)" : (fmt::format("{}% (UP/DOWN)", static_cast<int>(tmps.volume * 100)))) : "";
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "S (S)" : "NS (S)";
    render_playback_info_bar(LINES - 1, playing_str, shuffled_str, loop_str, volume_str);
  }
}

// Large Design: (see tui_design.md in the doc/ folder for prototype designs)
//                                01. Palace
// < 01. Palace                                                    02. Peso >
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
//
//
//
//
//                         VIDEO/AUDIO VISUALIZATION
//
//
//
//
//
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 00:00 / 26:54 | @@@@------------------------------------------------------
// PLAYING           NOT LOOPED            SHUFFLE                VOLUME: 50%

void render_tui_large(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  const int BOTTOM_BAR_LINE_START = LINES - 3;
  try_render_frame(tmps, sshot, tmrs, 2, 0, COLS, BOTTOM_BAR_LINE_START - 1);

  {
    int top_line = 0;
    top_line = render_current_filenamex(tmps, tmrs, top_line);
    top_line = render_nexprev_files_large(tmps, tmrs, top_line);
    wfill_box(stdscr, top_line, 0, COLS, 1, '~');
  }

  int bottom_line = BOTTOM_BAR_LINE_START;
  wfill_box(stdscr, bottom_line++, 0, COLS, 1, '~');
  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    wprint_playback_bar(stdscr, bottom_line++, 0, COLS, sshot.media_time_secs, sshot.media_duration_secs, false);
    const std::string_view playing_str = sshot.playing ? "PLAYING" : "PAUSED";
    const std::string loop_str = str_capslock(loop_type_cstr(tmps.plist.loop_type()));
    const std::string volume_str = sshot.has_audio_output ? (tmps.muted ? "MUTED" : fmt::format("VOLUME: {}%", static_cast<int>(tmps.volume * 100))) : "NO SOUND";
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "SHUFFLED" : "NOT SHUFFLED";
    render_playback_info_bar(bottom_line++, playing_str, shuffled_str, loop_str, volume_str);
  }
}

// Large Help Design:
//                                01. Palace
// < (P) 01. Palace                                            02. Peso (N) >
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//
//
//
//
//
//                         VIDEO/AUDIO VISUALIZATION
//
//
//
//
//
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 00:00 / 26:54 | 0@@@--1-----2-----3-----4-----5-----6-----7-----8-----9---
// PLAYING (SPACE)      NOT LOOPED (L)    SHUFFLE (S)   VOLUME: 50% (UP/DOWN)
//   Color Mode: Color & Bg (c/b/g)              Force Refresh (R)
//   Skip +-5 secs (LEFT/RIGHT)    Fullscreen (F)    Hide Control Info: (H)

void render_tui_large_help(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  const int BOTTOM_BAR_LINE_START = LINES - 5;
  try_render_frame(tmps, sshot, tmrs, 2, 0, COLS, BOTTOM_BAR_LINE_START - 1);

  {
    int top_line = 0;
    top_line = render_current_filenamex(tmps, tmrs, 0);
    top_line = render_nexprev_files_large(tmps, tmrs, top_line);
    wfill_box(stdscr, top_line, 0, COLS, 1, '~');
  }


  int line = BOTTOM_BAR_LINE_START;
  wfill_box(stdscr, line++, 0, COLS, 1, '~');
  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    wprint_playback_bar(stdscr, line++, 0, COLS, sshot.media_time_secs, sshot.media_duration_secs, true);
    const std::string_view playing_str = sshot.playing ? "PLAYING (SPACE)" : "PAUSED (SPACE)";
    const std::string loop_str = fmt::format("{} (L)", str_capslock(loop_type_cstr(tmps.plist.loop_type())));
    const std::string volume_str = sshot.has_audio_output ? (tmps.muted ? "MUTED (M)" : fmt::format("VOL: {}% (UP/DOWN)", static_cast<int>(tmps.volume * 100))) : "NO SOUND";
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "SHUFFLED (S)" : "NOT SHUFFLED (S)";
    render_playback_info_bar(line++, playing_str, shuffled_str, loop_str, volume_str);
  }

  std::array<std::string_view, 2> help_line_1;
  std::string vidoutstr = fmt::format("Color Mode: {} (C/B/G)", vidoutmode_strv(tmps.vom));
  help_line_1[0] = vidoutstr;
  help_line_1[1] = "Force Refresh (R)";
  werasebox(stdscr, line, 0, COLS, 1);
  wprint_labels(stdscr, help_line_1.data(), 2, line++, 0, COLS);

  std::array<std::string_view, 3> help_line_2;
  help_line_2[0] = "Skip 5 secs (LEFT/RIGHT)";
  help_line_2[1] = "Fullscreen (F)";
  help_line_2[2] = "Hide Control Info: (H)";
  werasebox(stdscr, line, 0, COLS, 1);
  wprint_labels(stdscr, help_line_2.data(), 3, line++, 0, COLS);
}

std::string_view get_media_file_display_name(const std::filesystem::path& abs_path, MetadataCache& mchc) {
  mchc_cache_file(abs_path, mchc);

  // TMEDIA_DISPLAY_CACHE_KEY is the key used specifically by
  // get_media_file_display_name to cache the display name used by tmedia.

  // A call to get_media_file_display_name is **guaranteed** to cache
  // TMEDIA_DISPLAY_CACHE_KEY in the metadata cache for the file
  // abs_path. this means that on subsequent calls to
  // get_media_file_display_name, get_media_file_display_name will always prefer
  // the TMEDIA_DISPLAY_CACHE_KEY value on the abs_path as the pre-calculated
  // display name.

  // intentionally small for SSO
  static constexpr std::string_view TMEDIA_DISPLAY_CACHE_KEY = "tmedia_disp";
  std::optional<std::string_view> display_name = mchc_get(abs_path.c_str(), TMEDIA_DISPLAY_CACHE_KEY, mchc);
  if (display_name.has_value()) {
    return display_name.value();
  }

  std::optional<std::string_view> artist = mchc_get(abs_path.c_str(), "artist", mchc);
  std::optional<std::string_view> title = mchc_get(abs_path.c_str(), "title", mchc);

  if (artist.has_value() && title.has_value()) {
    const std::string display_string = fmt::format("{} - {}", artist.value(), title.value());
    return mchc_add(abs_path.c_str(), TMEDIA_DISPLAY_CACHE_KEY, display_string, mchc);
  } else if (title.has_value()) {
    return mchc_add(abs_path.c_str(), TMEDIA_DISPLAY_CACHE_KEY, title.value(), mchc);
  }

  const std::string display_string = std::filesystem::path(abs_path).filename().string();
  return mchc_add(abs_path.c_str(), TMEDIA_DISPLAY_CACHE_KEY, display_string, mchc);
}

void render_pixel_data(PixelData& pixel_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VidOutMode output_mode, std::string_view ascii_char_map) {
  if (!tmcurses_has_colors()) // if there are no colors, just don't print colors :)
    output_mode = VidOutMode::PLAIN;

  switch (output_mode) {
    case VidOutMode::PLAIN: return render_pixel_data_plain(pixel_data, scaling_buffer, bounds_row, bounds_col, bounds_width, bounds_height, ascii_char_map);
    case VidOutMode::COLOR:
    case VidOutMode::GRAY: return render_pixel_data_color(pixel_data, scaling_buffer, bounds_row, bounds_col, bounds_width, bounds_height, ascii_char_map);
    case VidOutMode::COLOR_BG:
    case VidOutMode::GRAY_BG: return render_pixel_data_bg(pixel_data, scaling_buffer, bounds_row, bounds_col, bounds_width, bounds_height);
  }
}

void render_playback_info_bar(int line, std::string_view playing_str, std::string_view shuffled_str, std::string_view loop_str, std::string_view volume_str) {
  std::array<std::string_view, 4> bottom_labels;
  std::size_t nb_labels = 0;
  bottom_labels[nb_labels++] = playing_str;
  bottom_labels[nb_labels++] = shuffled_str;
  bottom_labels[nb_labels++] = loop_str;
  bottom_labels[nb_labels++] = volume_str;

  werasebox(stdscr, line, 0, COLS, 1);
  wprint_labels(stdscr, bottom_labels.data(), nb_labels, line, 0, COLS);
}

/**
 * Renders the current filename at the middle of the top line of the terminal
 * screen.
 */
int render_current_filename(std::size_t index, std::size_t plist_size, std::string_view display_name, int line) {
  static constexpr int CURRENT_FILE_NAME_MARGIN = 5;
  werasebox(stdscr, 0, 0, COLS, 2);
  std::array<std::string_view, 3> top_labels;
  const std::string current_plist_file_display = fmt::format("({}/{})", index, plist_size);
  top_labels[0] = current_plist_file_display;
  top_labels[1] = " ";
  top_labels[2] = display_name;
  wprint_labels_middle(stdscr, top_labels.begin(), top_labels.size(), line, CURRENT_FILE_NAME_MARGIN, COLS - (CURRENT_FILE_NAME_MARGIN * 2));
  return line + 1;
}

int render_current_filenamex(const TMediaProgramState& tmps, TMediaRendererState& tmrs, int line) {
  std::string_view disp_name = get_media_file_display_name(tmps.plist.current().path, tmrs.metadata_cache);;
  return render_current_filename(tmps.plist.index() + 1, tmps.plist.size(), disp_name, line);
}

const char* loop_type_cstr_short(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "NL";
    case LoopType::REPEAT: return "R";
    case LoopType::REPEAT_ONE: return "RO";
  }
  return "UNK";
}

void try_render_frame(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs, int bounds_row, int bounds_col, int bounds_width, int bounds_height) {
  assert(sshot.frame != nullptr);
  if (sshot.should_render_frame) {
    render_pixel_data(*(sshot.frame), tmrs.scaling_buffer, bounds_row, bounds_col, bounds_width, bounds_height, tmps.vom, tmps.ascii_display_chars);
  }
  tmrs.req_frame_dim = { bounds_width, bounds_height };
}

void render_current_filenamex_compact(const TMediaProgramState& tmps, TMediaRendererState& tmrs, int line) {
  render_current_filenamex(tmps, tmrs, line);

  if (tmps.plist.size() > 1) {
    if (tmps.plist.can_move(PlaylistMvCmd::REWIND)) {
      const std::string_view left_arrow = tmps.show_ctrl_info ? "< (P) " : "< ";
      const TMLabelStyle style = TMLabelStyle(line, 0, COLS, TMAlign::LEFT, 0, 0);
      tm_mvwaddstr_label(stdscr, style, left_arrow);
    }

    if (tmps.plist.can_move(PlaylistMvCmd::SKIP)) {
      const std::string_view right_arrow = tmps.show_ctrl_info ? " (N) > " : " >";
      const TMLabelStyle style = TMLabelStyle(line, 0, COLS, TMAlign::RIGHT, 0, 0);
      tm_mvwaddstr_label(stdscr, style, right_arrow);
    }
  }
}

int render_nexprev_files_large(const TMediaProgramState& tmps, TMediaRendererState& tmrs, int line) {
  if (tmps.plist.size() == 1) return line;
  static constexpr int MOVE_FILE_NAME_MIDDLE_MARGIN = 5;

  if (tmps.plist.can_move(PlaylistMvCmd::REWIND)) {
    werasebox(stdscr, line, 0, COLS / 2, 1);
    const std::string_view left_arrow = tmps.show_ctrl_info ? "< (P) " : "< ";
    const std::string_view rewind_media_file_display_string = get_media_file_display_name(tmps.plist.peek_move(PlaylistMvCmd::REWIND).path, tmrs.metadata_cache);
    TMLabelStyle left_arrow_string_style(line, 0, COLS / 2, TMAlign::LEFT, 0, 0);
    TMLabelStyle rewind_display_style(line, 0, COLS / 2, TMAlign::LEFT, left_arrow.length(), MOVE_FILE_NAME_MIDDLE_MARGIN);
    tm_mvwaddstr_label(stdscr, rewind_display_style, rewind_media_file_display_string);
    tm_mvwaddstr_label(stdscr, left_arrow_string_style, left_arrow);
  }

  if (tmps.plist.can_move(PlaylistMvCmd::SKIP)) {
    werasebox(stdscr, line, COLS / 2, COLS / 2, 1);
    const std::string_view right_arrow = tmps.show_ctrl_info ? " (N) > " : " >";
    const std::string_view skip_display_string = get_media_file_display_name(tmps.plist.peek_move(PlaylistMvCmd::SKIP).path, tmrs.metadata_cache);
    TMLabelStyle skip_display_string_style(line, COLS / 2, COLS / 2, TMAlign::RIGHT, MOVE_FILE_NAME_MIDDLE_MARGIN, right_arrow.length());
    TMLabelStyle right_arrow_string_style(line, COLS / 2, COLS / 2, TMAlign::RIGHT, 0, 0);
    tm_mvwaddstr_label(stdscr, skip_display_string_style, skip_display_string);
    tm_mvwaddstr_label(stdscr, right_arrow_string_style, right_arrow);
  }

  return line + 1;
}

std::string_view vidoutmode_strv(VidOutMode vom) {
  switch (vom) {
    case VidOutMode::COLOR: return "COLOR";
    case VidOutMode::GRAY: return "GRAY";
    case VidOutMode::COLOR_BG: return "COLOR & BG";
    case VidOutMode::GRAY_BG: return "GRAY & BG";
    case VidOutMode::PLAIN: return "PLAIN";
  }
  return "N/A";
}
