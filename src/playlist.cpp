#include "playlist.h"

#include <cstddef>

#include <vector>
#include <string>
#include <stdexcept>

int playlist_get_move(int current, int size, LoopType loop_type, PlaylistMoveCommand move_cmd);

Playlist::Playlist() {
  this->m_loop_type = LoopType::NO_LOOP;
  this->m_current_index = -1;
}

Playlist::Playlist(std::vector<std::string> media_files, LoopType loop_type) {
  this->m_files = media_files;
  this->m_current_index = 0;
  this->m_loop_type = loop_type;
}

int Playlist::index() const {
  return this->m_current_index;
}

std::size_t Playlist::size() const noexcept {
  return this->m_files.size();
}

std::string Playlist::current() {
  if (this->m_files.size() == 0) {
    throw std::runtime_error("[Playlist::current] Cannot access current file of empty playlist");
  }
  return this->m_files[this->m_current_index];
}

LoopType Playlist::loop_type() noexcept {
  return this->m_loop_type;
}

void Playlist::set_loop_type(LoopType loop_type) noexcept {
  this->m_loop_type = loop_type;
}

void Playlist::move(PlaylistMoveCommand move_cmd) {
  int next = playlist_get_move(this->m_current_index, this->m_files.size(), this->m_loop_type, move_cmd);
  if (next < 0) {
    throw std::runtime_error("[Playlist::move] can not commit move" + playlist_move_cmd_str(move_cmd)); 
  }

  if (move_cmd == PlaylistMoveCommand::REWIND || move_cmd == PlaylistMoveCommand::SKIP) {
    if (this->m_loop_type == LoopType::REPEAT_ONE) {
      this->m_loop_type = LoopType::REPEAT;
    }
  }

  this->m_current_index = next;
}

std::string Playlist::peek_move(PlaylistMoveCommand move_cmd) {
  int next = playlist_get_move(this->m_current_index, this->m_files.size(), this->m_loop_type, move_cmd);
  if (next < 0) {
    throw std::runtime_error("[Playlist::peek_move] can not commit move" + playlist_move_cmd_str(move_cmd)); 
  }

  return this->m_files[next];
}

bool Playlist::can_move(PlaylistMoveCommand move_cmd) const noexcept {
  return playlist_get_move(this->m_current_index, this->m_files.size(), this->m_loop_type, move_cmd) >= 0;
}






int playlist_get_rewind(int current, int size, LoopType loop_type);
int playlist_get_next(int current, int size, LoopType loop_type);
int playlist_get_skip(int current, int size, LoopType loop_type);

int playlist_get_move(int current, int size, LoopType loop_type, PlaylistMoveCommand move_cmd) {
  switch (move_cmd) {
    case PlaylistMoveCommand::NEXT: return playlist_get_next(current, size, loop_type);
    case PlaylistMoveCommand::SKIP: return playlist_get_skip(current, size, loop_type);
    case PlaylistMoveCommand::REWIND: return playlist_get_rewind(current, size, loop_type);
  }
  throw std::runtime_error("[playlist_get_move] could not find move rule for " + playlist_move_cmd_str(move_cmd));
}

int playlist_get_next(int current, int size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current + 1 == size ? -1 : current + 1;
    case LoopType::REPEAT: return current + 1 == size ? 0 : current + 1;
    case LoopType::REPEAT_ONE: return current;
  }
  throw std::runtime_error("[playlist_get_next] could not find next rule for " + loop_type_str(loop_type));
}

int playlist_get_skip(int current, int size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current + 1 == size ? -1 : current + 1;
    case LoopType::REPEAT: 
    case LoopType::REPEAT_ONE: return current + 1 == size ? 0 : current + 1;
  }
  throw std::runtime_error("[playlist_get_skip] could not find skip rule for " + loop_type_str(loop_type));
}

int playlist_get_rewind(int current, int size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current - 1 < 0 ? 0 : current - 1;
    case LoopType::REPEAT:
    case LoopType::REPEAT_ONE: return current - 1 < 0 ? size - 1 : current - 1;
  }
  throw std::runtime_error("[playlist_get_rewind] could not find rewind rule for " + loop_type_str(loop_type));
}

std::string loop_type_str(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "no loop";
    case LoopType::REPEAT: return "repeat";
    case LoopType::REPEAT_ONE: return "repeat one";
  }
  throw std::runtime_error("[loop_type_str] cannot find string repr of loop_type");
}

std::string playlist_move_cmd_str(PlaylistMoveCommand move_cmd) {
  switch (move_cmd) {
    case PlaylistMoveCommand::NEXT: return "next";
    case PlaylistMoveCommand::SKIP: return "skip";
    case PlaylistMoveCommand::REWIND: return "rewind";
  }
  throw std::runtime_error("[playlist_move_cmd_str] cannot find string repr of move_cmd");
}