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

static constexpr int MIN_RENDER_COLS = 5;
static constexpr int MIN_RENDER_LINES = 2;
static constexpr std::size_t METADATA_CACHE_MAX_SIZE = 30;

const char* loop_type_cstr_short(LoopType loop_type);
std::string_view get_media_file_display_name(const std::filesystem::path& abs_path, MetadataCache& mchc);
void render_pixel_data(PixelData& pixel_data, PixelData& scaling_buffer, int bounds_row, int bounds_col, int bounds_width, int bounds_height, VidOutMode output_mode, std::string_view ascii_char_map);
void render_bottom_bar(const TMediaProgramSnapshot& sshot, std::string_view playing_str, std::string_view loop_str, std::string_view volume_str, std::string_view shuffled_str);
void render_current_filename(const TMediaProgramState& tmps, TMediaRendererState& tmrs);

void render_tui_fullscreen(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);
void render_tui_compact(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);
void render_tui_large(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs);

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
    render_tui_compact(tmps, sshot, tmrs);
  } else {
    render_tui_large(tmps, sshot, tmrs);
  }

  tmrs.last_frame_dims = { sshot.frame->m_width, sshot.frame->m_height };
}

void render_tui_fullscreen(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  assert(sshot.frame != nullptr);
  if (sshot.should_render_frame) {
    render_pixel_data(*(sshot.frame), tmrs.scaling_buffer, 0, 0, COLS, LINES, tmps.vom, tmps.ascii_display_chars);
  }
  tmrs.req_frame_dim = { COLS, LINES };
  (void)tmrs;
}

void render_tui_compact(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  assert(sshot.frame != nullptr);
  if (sshot.should_render_frame) {
    render_pixel_data(*(sshot.frame), tmrs.scaling_buffer, 2, 0, COLS, LINES - 4, tmps.vom, tmps.ascii_display_chars);
  }
  tmrs.req_frame_dim = { COLS, LINES - 4 };

  render_current_filename(tmps, tmrs);

  wfill_box(stdscr, 1, 0, COLS, 1, '~');
  if (tmps.plist.size() > 1) {
    if (tmps.plist.can_move(PlaylistMvCmd::REWIND)) {
      tm_mvwaddstr_label(stdscr, TMLabelStyle(0, 0, COLS, TMAlign::LEFT, 0, 0), "<");
    }

    if (tmps.plist.can_move(PlaylistMvCmd::SKIP)) {
      tm_mvwaddstr_label(stdscr, TMLabelStyle(0, 0, COLS, TMAlign::RIGHT, 0, 0), ">");
    }
  }

  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    const std::string_view playing_str = sshot.playing ? ">" : "||";
    const std::string_view loop_str = loop_type_cstr_short(tmps.plist.loop_type()); 
    const std::string volume_str = sshot.has_audio_output ? (tmps.muted ? "M" : (fmt::format("{}%", static_cast<int>(tmps.volume * 100)))) : "";
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "S" : "NS";
    render_bottom_bar(sshot, playing_str, shuffled_str, loop_str, volume_str);
  }
}

void render_tui_large(const TMediaProgramState& tmps, const TMediaProgramSnapshot& sshot, TMediaRendererState& tmrs) {
  assert(sshot.frame != nullptr);
  if (sshot.should_render_frame) {
    render_pixel_data(*(sshot.frame), tmrs.scaling_buffer, 2, 0, COLS, LINES - 4, tmps.vom, tmps.ascii_display_chars);
  }
  tmrs.req_frame_dim = { COLS, LINES - 4 };

  render_current_filename(tmps, tmrs);

  if (tmps.plist.size() == 1) {
    wfill_box(stdscr, 1, 0, COLS, 1, '~');
  } else {
    static constexpr int MOVE_FILE_NAME_MIDDLE_MARGIN = 5;
    wfill_box(stdscr, 2, 0, COLS, 1, '~');

    if (tmps.plist.can_move(PlaylistMvCmd::REWIND)) {
      werasebox(stdscr, 1, 0, COLS / 2, 1);
      static constexpr int LEFT_ARROW_MARGIN = 3;
      const std::string_view rewind_media_file_display_string = get_media_file_display_name(tmps.plist.peek_move(PlaylistMvCmd::REWIND).path, tmrs.metadata_cache);
      TMLabelStyle left_arrow_string_style(1, 0, COLS / 2, TMAlign::LEFT, 0, 0);
      TMLabelStyle rewind_display_style(1, 0, COLS / 2, TMAlign::LEFT, LEFT_ARROW_MARGIN, MOVE_FILE_NAME_MIDDLE_MARGIN);
      tm_mvwaddstr_label(stdscr, rewind_display_style, rewind_media_file_display_string);
      tm_mvwaddstr_label(stdscr, left_arrow_string_style, "< ");
    }

    if (tmps.plist.can_move(PlaylistMvCmd::SKIP)) {
      static constexpr int RIGHT_ARROW_MARGIN = 3;
      werasebox(stdscr, 1, COLS / 2, COLS / 2, 1);
      const std::string_view skip_display_string = get_media_file_display_name(tmps.plist.peek_move(PlaylistMvCmd::SKIP).path, tmrs.metadata_cache);
      TMLabelStyle skip_display_string_style(1, COLS / 2, COLS / 2, TMAlign::RIGHT, MOVE_FILE_NAME_MIDDLE_MARGIN, RIGHT_ARROW_MARGIN);
      TMLabelStyle right_arrow_string_style(1, COLS / 2, COLS / 2, TMAlign::RIGHT, 0, 0);
      tm_mvwaddstr_label(stdscr, skip_display_string_style, skip_display_string);
      tm_mvwaddstr_label(stdscr, right_arrow_string_style, " >");
    }
  }

  if (sshot.media_type == MediaType::VIDEO || sshot.media_type == MediaType::AUDIO) {
    const std::string_view playing_str = sshot.playing ? "PLAYING" : "PAUSED";
    const std::string loop_str = str_capslock(loop_type_cstr(tmps.plist.loop_type())); 
    const std::string volume_str = sshot.has_audio_output ? (tmps.muted ? "MUTED" : fmt::format("VOLUME: {}%", static_cast<int>(tmps.volume * 100))) : "NO SOUND";
    const std::string_view shuffled_str = tmps.plist.shuffled() ? "SHUFFLED" : "NOT SHUFFLED";
    render_bottom_bar(sshot, playing_str, shuffled_str, loop_str, volume_str);
  }
}

std::string_view get_media_file_display_name(const std::filesystem::path& abs_path, MetadataCache& mchc) {
  mchc_cache_file(abs_path, mchc);

  // TMEDIA_DISPLAY_CACHE_KEY is the key used specifically by
  // get_media_file_display_name to cache the display name used by tmedia.

  // A call to get_media_file_display_name is guaranteed to cache
  // TMEDIA_DISPLAY_CACHE_KEY in the metadata cache for the file
  // abs_path

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

void render_bottom_bar(const TMediaProgramSnapshot& sshot, std::string_view playing_str, std::string_view shuffled_str, std::string_view loop_str, std::string_view volume_str) {
  wfill_box(stdscr, LINES - 3, 0, COLS, 1, '~');
  wprint_playback_bar(stdscr, LINES - 2, 0, COLS, sshot.media_time_secs, sshot.media_duration_secs);

  std::array<std::string_view, 4> bottom_labels;
  std::size_t nb_labels = 0;
  bottom_labels[nb_labels++] = playing_str;
  bottom_labels[nb_labels++] = shuffled_str;
  bottom_labels[nb_labels++] = loop_str;
  bottom_labels[nb_labels++] = volume_str;

  werasebox(stdscr, LINES - 1, 0, COLS, 1);
  wprint_labels(stdscr, bottom_labels.data(), nb_labels, LINES - 1, 0, COLS);
}

void render_current_filename(const TMediaProgramState& tmps, TMediaRendererState& tmrs) {
  static constexpr int CURRENT_FILE_NAME_MARGIN = 5;
  werasebox(stdscr, 0, 0, COLS, 2);
  std::array<std::string_view, 3> top_labels;
  const std::string current_plist_file_display = fmt::format("({}/{})", tmps.plist.index() + 1, tmps.plist.size());
  top_labels[0] = current_plist_file_display;
  top_labels[1] = " ";
  top_labels[2] = get_media_file_display_name(tmps.plist.current().path, tmrs.metadata_cache);
  wprint_labels_middle(stdscr, top_labels.begin(), top_labels.size(), 0, CURRENT_FILE_NAME_MARGIN, COLS - (CURRENT_FILE_NAME_MARGIN * 2));
}

const char* loop_type_cstr_short(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "NL";
    case LoopType::REPEAT: return "R";
    case LoopType::REPEAT_ONE: return "RO";
  }
  return "UNK";
}