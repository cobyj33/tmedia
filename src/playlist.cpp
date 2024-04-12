#include "playlist.h"

#include "funcmac.h"

#include <effolkronium/random.hpp>
#include <fmt/format.h>

#include <cstddef>
#include <cassert>

#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>

#include <cassert>

#include <filesystem>

constexpr int NO_MOVE_AVAILABLE = -1;
int playlist_get_move(int current, int size, LoopType loop_type, PlaylistMvCmd move_cmd);

Playlist::Playlist() {
  this->m_loop_type = LoopType::NO_LOOP;
  this->m_qi = -1;
  this->m_shuffled = false;
}

Playlist::Playlist(const std::vector<std::filesystem::path>& entries, LoopType loop_type) {
  this->m_entries = entries;
  this->m_qi = entries.size() > 0 ? 0 : -1;
  this->m_loop_type = loop_type;
  this->m_shuffled = false;

  for (int i = 0; (std::size_t)i < entries.size(); i++) {
    this->m_q.push_back(i);
  }
}

int Playlist::index() const {
  if (this->m_entries.size() == 0) {
    throw std::runtime_error(fmt::format("[{}] Cannot access current "
    "index of empty playlist", FUNCDINFO));
  }

  assert(this->m_qi >= 0);
  assert(static_cast<std::size_t>(this->m_qi) < this->m_q.size());
  
  return this->m_q[this->m_qi];
}

const std::filesystem::path& Playlist::current() const {
  if (this->m_entries.size() == 0) {
    throw std::runtime_error(fmt::format("[{}] Cannot access current "
    "file of empty playlist", FUNCDINFO));
  }

  assert(this->m_qi >= 0);
  assert(static_cast<std::size_t>(this->m_qi) < this->m_q.size());

  return this->m_entries[this->index()];
}

void Playlist::insert(const std::filesystem::path& entry, std::size_t i) {
  // note we don't have to handle the empty playlist case with
  // this guard clause as well.
  if (i >= this->m_q.size()) return this->push_back(entry);

  this->m_entries.push_back(entry);
  this->m_q.insert(this->m_q.begin() + i, this->m_entries.size() - 1);
  this->m_qi += this->m_qi >= static_cast<int>(i);
}

void Playlist::push_back(const std::filesystem::path& entry) {
  this->m_qi = (this->m_qi > 0) * this->m_qi; // set m_qi to 0 if it is less than 0, or else keep it the same
  this->m_entries.push_back(entry);
  this->m_q.push_back(this->m_entries.size() - 1);
}

void Playlist::clear() {
  this->m_qi = -1;
  this->m_entries.clear();
  this->m_q.clear();
  this->m_shuffled = false;
}

void Playlist::remove_entry_at_entry_index(int entry_index) {
  assert(entry_index >= 0);
  assert(this->m_entries.size() > 0);
  assert(entry_index < static_cast<int>(this->m_entries.size()));

  // removes the found entry
  this->m_entries.erase(this->m_entries.begin() + entry_index);
  
  // remove the queue entry
  for (std::size_t qi = 0; qi < this->m_q.size(); qi++) {
    if (this->m_q[qi] == entry_index) {
      this->m_q.erase(this->m_q.begin() + qi);
      break;
    }
  }

  // decrement all higher queue indexes
  for (std::size_t qi = 0; qi < this->m_q.size(); qi++) {
    this->m_q[qi] -= this->m_q[qi] > entry_index;
  }
  // decrement queue index if it was higher
  this->m_qi -= this->m_qi > entry_index;

  assert(this->m_qi >= 0);
  assert((std::size_t)this->m_qi < this->m_q.size());
}

void Playlist::remove(std::size_t i) {
  if (this->m_entries.size() <= 1) return this->clear(); // edge case!

  if (i > this->m_q.size()) {
    throw std::runtime_error(fmt::format("[{}] Attempted to remove playlist"
    "index greater than the size of the playlist: {} from {}", FUNCDINFO, i,
    this->m_q.size()));
  } 

  int entry_index = this->m_q[i];
  this->remove_entry_at_entry_index(entry_index);
}

void Playlist::remove(const std::filesystem::path& entry) {
  assert(this->m_q.size() == this->m_entries.size());
  if (this->m_entries.size() <= 1) return this->clear();

  
  for (int i = 0; i < static_cast<int>(this->m_entries.size()); i++) {
    if (this->m_entries[i] == entry) return this->remove_entry_at_entry_index(i);
  }
}

const std::filesystem::path& Playlist::at(std::size_t i) const {
  if (i > this->m_q.size()) {
    throw std::runtime_error(fmt::format("[{}] Attempted to get playlist item"
    "at index greater than the size of the playlist: {} from {}", FUNCDINFO, i,
    this->m_q.size()));
  }

  return this->m_entries[this->m_q[i]];
}

const std::filesystem::path& Playlist::operator[](std::size_t i) {
  return this->m_entries[this->m_q[i]];
}

const std::vector<std::filesystem::path>& Playlist::view() const {
  return this->m_entries;
}

bool Playlist::has(const std::filesystem::path& entry) const {
  return std::find(this->m_entries.begin(), this->m_entries.end(), entry) != this->m_entries.end();
}

void Playlist::move(PlaylistMvCmd move_cmd) {
  int next = playlist_get_move(this->m_qi, this->m_q.size(), this->m_loop_type, move_cmd);
  if (next == NO_MOVE_AVAILABLE) {
    throw std::runtime_error(fmt::format("[{}] can not commit move {}.",
    FUNCDINFO, playlist_move_cmd_cstr(move_cmd))); 
  }

  // if skipping or rewinding, kick out of repeat_one mode
  if ((move_cmd == PlaylistMvCmd::REWIND || move_cmd == PlaylistMvCmd::SKIP) &&
      this->m_loop_type == LoopType::REPEAT_ONE) {
    this->m_loop_type = LoopType::REPEAT;
  }

  this->m_qi = next;

  // reshuffle our index queue so that the next song is not at the end and our shuffle remains fresh
  // unfortunate edge case, if we are going to the next file, and we are shuffled, and we are at the second to last song, we need to 
  if ((move_cmd == PlaylistMvCmd::NEXT || move_cmd == PlaylistMvCmd::SKIP) &&
      this->m_shuffled && (std::size_t)this->m_qi == this->m_q.size() - 2) {
    this->shuffle(true);
  }
}

const std::filesystem::path& Playlist::peek_move(PlaylistMvCmd move_cmd) const {
  int next = playlist_get_move(this->m_qi, this->m_q.size(), this->m_loop_type, move_cmd);
  if (next == NO_MOVE_AVAILABLE) {
    throw std::runtime_error(fmt::format("[{}] can not commit move {}",
    FUNCDINFO, playlist_move_cmd_cstr(move_cmd))); 
  }

  return this->m_entries[this->m_q[next]];
}

bool Playlist::can_move(PlaylistMvCmd move_cmd) const noexcept {
  return playlist_get_move(this->m_qi, this->m_q.size(), this->m_loop_type, move_cmd) >= 0;
}

void Playlist::shuffle(bool keep_current_file_first) {
  if (this->m_q.size() == 0) return;

  if (keep_current_file_first) {
    int tmp = this->m_q[0];
    this->m_q[0] = this->m_q[this->m_qi];
    this->m_q[this->m_qi] = tmp;
    effolkronium::random_thread_local::shuffle(this->m_q.begin() + 1, this->m_q.end());
  } else {
    effolkronium::random_thread_local::shuffle(this->m_q);
  }
  this->m_qi = 0;
  this->m_shuffled = true;
}

void Playlist::unshuffle() {
  if (this->m_q.size() == 0) return;

  this->m_qi = this->m_q[this->m_qi];
  std::sort(this->m_q.begin(), this->m_q.end());
  this->m_shuffled = false;
}

int playlist_get_rewind(int current, int size, LoopType loop_type);
int playlist_get_next(int current, int size, LoopType loop_type);
int playlist_get_skip(int current, int size, LoopType loop_type);

int playlist_get_move(int current, int size, LoopType loop_type, PlaylistMvCmd move_cmd) {
  switch (move_cmd) {
    case PlaylistMvCmd::NEXT: return playlist_get_next(current, size, loop_type);
    case PlaylistMvCmd::SKIP: return playlist_get_skip(current, size, loop_type);
    case PlaylistMvCmd::REWIND: return playlist_get_rewind(current, size, loop_type);
  }
  return NO_MOVE_AVAILABLE;
}

int playlist_get_next(int current, int size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current + 1 == size ? NO_MOVE_AVAILABLE : current + 1;
    case LoopType::REPEAT: return current + 1 == size ? 0 : current + 1;
    case LoopType::REPEAT_ONE: return current;
  }
  return NO_MOVE_AVAILABLE;
}

int playlist_get_skip(int current, int size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current + 1 == size ? NO_MOVE_AVAILABLE : current + 1;
    case LoopType::REPEAT: 
    case LoopType::REPEAT_ONE: return current + 1 == size ? 0 : current + 1;
  }
  return NO_MOVE_AVAILABLE;
}

int playlist_get_rewind(int current, int size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current - 1 < 0 ? 0 : current - 1;
    case LoopType::REPEAT:
    case LoopType::REPEAT_ONE: return current - 1 < 0 ? size - 1 : current - 1;
  }
  return NO_MOVE_AVAILABLE;
}

const char* loop_type_cstr(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "no-loop";
    case LoopType::REPEAT: return "repeat";
    case LoopType::REPEAT_ONE: return "repeat-one";
  }
  return "unknown";
}

LoopType loop_type_from_str(std::string_view str) {
  if (str == "no-loop") {
    return LoopType::NO_LOOP;
  } else if (str == "repeat") {
    return LoopType::REPEAT;
  } else if (str == "repeat-one") {
    return LoopType::REPEAT_ONE;
  }

  throw std::runtime_error(fmt::format("[{}] cannot find repr of loop "
                                  "type string: {}", FUNCDINFO, str));
}

const char* playlist_move_cmd_cstr(PlaylistMvCmd move_cmd) {
  switch (move_cmd) {
    case PlaylistMvCmd::NEXT: return "next";
    case PlaylistMvCmd::SKIP: return "skip";
    case PlaylistMvCmd::REWIND: return "rewind";
  }
  return "unknown";
}