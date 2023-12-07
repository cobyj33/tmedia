#include "playlist.h"

#include <effolkronium/random.hpp>

#include <cstddef>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

int playlist_get_move(int current, int size, LoopType loop_type, PlaylistMoveCommand move_cmd);

Playlist::Playlist() {
  this->m_loop_type = LoopType::NO_LOOP;
  this->m_queue_index = -1;
  this->m_shuffled = false;
}

Playlist::Playlist(std::vector<std::string> media_files, LoopType loop_type) {
  this->m_files = media_files;
  this->m_queue_index = 0;
  this->m_loop_type = loop_type;
  this->m_shuffled = false;

  for (int i = 0; (std::size_t)i < media_files.size(); i++) {
    this->m_queue_indexes.push_back(i);
  }
}

int Playlist::index() const {
  return this->m_queue_indexes[this->m_queue_index];
}

std::size_t Playlist::size() const noexcept {
  return this->m_files.size();
}

std::string Playlist::current() const {
  if (this->m_files.size() == 0) {
    throw std::runtime_error("[Playlist::current] Cannot access current file of empty playlist");
  }
  return this->m_files[this->index()];
}

LoopType Playlist::loop_type() const noexcept {
  return this->m_loop_type;
}

void Playlist::set_loop_type(LoopType loop_type) noexcept {
  this->m_loop_type = loop_type;
}

void Playlist::move(PlaylistMoveCommand move_cmd) {
  int next = playlist_get_move(this->m_queue_index, this->m_queue_indexes.size(), this->m_loop_type, move_cmd);
  if (next < 0) {
    throw std::runtime_error("[Playlist::move] can not commit move" + playlist_move_cmd_str(move_cmd)); 
  }

  // if skipping or rewinding, kick out of repeat_one mode
  if (move_cmd == PlaylistMoveCommand::REWIND || move_cmd == PlaylistMoveCommand::SKIP) {
    if (this->m_loop_type == LoopType::REPEAT_ONE) {
      this->m_loop_type = LoopType::REPEAT;
    }
  }

  this->m_queue_index = next;

  // reshuffle our index queue so that the next song is not at the end and our shuffle remains fresh
  // unfortunate edge case, if we are going to the next file, and we are shuffled, and we are at the second to last song, we need to 
  if ((move_cmd == PlaylistMoveCommand::NEXT || move_cmd == PlaylistMoveCommand::SKIP) &&
  this->m_shuffled && (std::size_t)this->m_queue_index == this->m_queue_indexes.size() - 2) {
    this->shuffle(true);
  }
}

std::string Playlist::peek_move(PlaylistMoveCommand move_cmd) const {
  int next = playlist_get_move(this->m_queue_index, this->m_queue_indexes.size(), this->m_loop_type, move_cmd);
  if (next < 0) {
    throw std::runtime_error("[Playlist::peek_move] can not commit move" + playlist_move_cmd_str(move_cmd)); 
  }

  return this->m_files[this->m_queue_indexes[next]];
}

bool Playlist::can_move(PlaylistMoveCommand move_cmd) const noexcept {
  return playlist_get_move(this->m_queue_index, this->m_queue_indexes.size(), this->m_loop_type, move_cmd) >= 0;
}

bool Playlist::shuffled() const {
  return this->m_shuffled;
}

void Playlist::shuffle(bool keep_current_file_first) {
  if (this->m_queue_indexes.size() > 1) {
    if (keep_current_file_first) {
      int tmp = this->m_queue_indexes[0];
      this->m_queue_indexes[0] = this->m_queue_indexes[this->m_queue_index];
      this->m_queue_indexes[this->m_queue_index] = tmp;
      effolkronium::random_thread_local::shuffle(this->m_queue_indexes.begin() + 1, this->m_queue_indexes.end());
    } else {
      effolkronium::random_thread_local::shuffle(this->m_queue_indexes);
    }
    this->m_queue_index = 0;
  }

  this->m_shuffled = true;
}

void Playlist::unshuffle() {
  this->m_queue_index = this->m_queue_indexes[this->m_queue_index];
  std::sort(this->m_queue_indexes.begin(), this->m_queue_indexes.end());
  this->m_shuffled = false;
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

const std::pair<std::string_view, LoopType> loop_type_strs[] = {
  {"no-loop", LoopType::NO_LOOP},
  {"repeat", LoopType::REPEAT},
  {"repeat-one", LoopType::REPEAT_ONE},
};

std::string loop_type_str(LoopType loop_type) {
  for (const std::pair<std::string_view, LoopType>& pair : loop_type_strs) {
    if (pair.second == loop_type) {
      return std::string(pair.first);
    }
  }

  throw std::runtime_error("[loop_type_str] cannot find string repr of loop_type");
}

LoopType loop_type_from_str(std::string_view loop_type_str) {
  for (const std::pair<std::string_view, LoopType>& pair : loop_type_strs) {
    if (pair.first == loop_type_str) {
      return pair.second;
    }
  }

  throw std::runtime_error("[loop_type_from_str] cannot find repr of loop type string: " + std::string(loop_type_str));
}

std::string playlist_move_cmd_str(PlaylistMoveCommand move_cmd) {
  switch (move_cmd) {
    case PlaylistMoveCommand::NEXT: return "next";
    case PlaylistMoveCommand::SKIP: return "skip";
    case PlaylistMoveCommand::REWIND: return "rewind";
  }
  throw std::runtime_error("[playlist_move_cmd_str] cannot find string repr of move_cmd");
}

bool loop_type_str_is_valid(std::string_view loop_type_str) {
  for (const std::pair<std::string_view, LoopType>& pair : loop_type_strs) {
    if (pair.first == loop_type_str) {
      return true;
    }
  }
  return false;
}