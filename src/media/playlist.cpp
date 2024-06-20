#include <tmedia/media/playlist.h>

#include <tmedia/util/defines.h>

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

std::size_t playlist_get_move(std::size_t current, std::size_t size, LoopType loop_type, PlaylistMvCmd move_cmd);

Playlist::Playlist() {
  this->m_loop_type = LoopType::NO_LOOP;
  this->m_qi = Playlist::npos;
  this->m_shuffled = false;
}

Playlist::Playlist(const std::vector<PlaylistItem>& entries, LoopType loop_type) {
  this->m_entries = entries;
  this->m_qi = entries.size() > 0 ? 0 : Playlist::npos;
  this->m_loop_type = loop_type;
  this->m_shuffled = false;

  for (std::size_t i = 0; i < entries.size(); i++) {
    this->m_q.push_back(i);
  }
}

/**
 * Note that this returns the location of the current entry in m_entries,
 * not the current queue location of m_q
*/
int Playlist::index() const {
  if (this->empty()) {
    throw std::runtime_error(fmt::format("[{}] Cannot access current "
    "index of empty playlist", FUNCDINFO));
  }

  assert(this->m_qi != Playlist::npos);
  assert(this->m_qi < this->m_q.size());

  return this->m_q[this->m_qi];
}

const PlaylistItem& Playlist::current() const {
  if (this->empty()) {
    throw std::runtime_error(fmt::format("[{}] Cannot access current "
    "file of empty playlist", FUNCDINFO));
  }

  assert(this->m_qi != Playlist::npos);
  assert(this->m_qi < this->m_q.size());

  return this->m_entries[this->index()];
}

void Playlist::insert(const PlaylistItem& entry, std::size_t i) {
  // note we don't have to handle the empty playlist case with
  // this guard clause as well.
  if (i >= this->m_q.size() || this->empty()) return this->push_back(entry);

  this->m_entries.insert(this->m_entries.begin() + i, entry);

  for (std::size_t qi = 0; qi < this->m_q.size(); qi++) {
    if (this->m_q[qi] >= i) {
      this->m_q[qi]++;
    }
  }

  this->m_q.insert(this->m_q.begin() + i, i);
  this->m_qi += this->m_qi >= i;
}

void Playlist::push_back(const PlaylistItem& entry) {
  if (this->m_qi == Playlist::npos) {
    this->m_qi = 0;
  }

  this->m_entries.push_back(entry);
  this->m_q.push_back(this->m_entries.size() - 1);
}

void Playlist::clear() {
  this->m_qi = this->npos;
  this->m_entries.clear();
  this->m_q.clear();
  this->m_shuffled = false;
}

void Playlist::remove_at_entry_idx(std::size_t entry_index) {
  assert(this->m_entries.size() > 0);
  assert(entry_index < this->m_entries.size());

  // removes the found entry
  this->m_entries.erase(this->m_entries.begin() + entry_index);

  // remove the queue entry
  std::vector<std::size_t>::iterator erase_iter = std::find(this->m_q.begin(), this->m_q.end(), entry_index);
  if (erase_iter != this->m_q.end())
    this->m_q.erase(erase_iter);

  // decrement all higher queue indexes
  for (std::size_t qi = 0; qi < this->m_q.size(); qi++) {
    this->m_q[qi] -= this->m_q[qi] > entry_index;
  }
  // decrement current queue index if it was higher
  this->m_qi -= this->m_qi > entry_index;

  if (this->empty()) {
    this->clear(); // reset m_qi and shuffled state
  } else {
    assert(this->m_qi != Playlist::npos);
    assert(this->m_qi < this->m_q.size());
  }
}

void Playlist::remove(std::size_t i) {
  assert(this->m_q.size() == this->m_entries.size());
  if (i > this->m_entries.size()) {
    throw std::runtime_error(fmt::format("[{}] Attempted to remove playlist"
    "index greater than the size of the playlist: {} from {}", FUNCDINFO, i,
    this->m_q.size()));
  }

  if (this->m_entries.size() <= 1) return this->clear(); // edge case!

  this->remove_at_entry_idx(i);
}

void Playlist::remove(const PlaylistItem& entry) {
  assert(this->m_q.size() == this->m_entries.size());

  for (std::size_t i = 0; i < this->m_entries.size(); i++) {
    if (this->m_entries[i].path == entry.path)
      return (void)this->remove_at_entry_idx(i);
  }
}

const PlaylistItem& Playlist::at(std::size_t i) const {
  if (i > this->m_q.size() || this->empty()) {
    throw std::runtime_error(fmt::format("[{}] Attempted to get playlist item"
    "at index greater than the size of the playlist: {} from {}", FUNCDINFO, i,
    this->m_q.size()));
  }

  return this->m_entries[i];
}

const PlaylistItem& Playlist::operator[](std::size_t i) const {
  return this->m_entries[i];
}

const std::vector<PlaylistItem>& Playlist::view() const {
  return this->m_entries;
}

bool Playlist::has(const PlaylistItem& entry) const {
  return std::find_if(this->m_entries.begin(), this->m_entries.end(),
    [&entry](const PlaylistItem& item) { return item.path == entry.path; }) != this->m_entries.end();
}

void Playlist::move(PlaylistMvCmd move_cmd) {
  if (this->empty()) {
    throw std::runtime_error(fmt::format("[{}] can not commit move on empty "
    "playlist {}.", FUNCDINFO, playlist_move_cmd_cstr(move_cmd)));
  }

  std::size_t next = playlist_get_move(this->m_qi, this->m_q.size(), this->m_loop_type, move_cmd);
  if (next == Playlist::npos) {
    throw std::runtime_error(fmt::format("[{}] can not commit move {}.",
    FUNCDINFO, playlist_move_cmd_cstr(move_cmd)));
  }

  // if skipping or rewinding, kick out of repeat_one mode
  if ((move_cmd == PlaylistMvCmd::REWIND || move_cmd == PlaylistMvCmd::SKIP) &&
      this->m_loop_type == LoopType::LOOP_ONE) {
    this->m_loop_type = LoopType::LOOP;
  }

  this->m_qi = next;

  // reshuffle our index queue so that the next song is not at the end and our shuffle remains fresh
  // unfortunate edge case, if we are going to the next file, and we are shuffled, and we are at the second to last song, we need to
  if ((move_cmd == PlaylistMvCmd::NEXT || move_cmd == PlaylistMvCmd::SKIP) &&
      this->m_shuffled && this->m_qi == this->m_q.size() - 2) {
    this->shuffle(true);
  }
}

const PlaylistItem& Playlist::peek_move(PlaylistMvCmd move_cmd) const {
  if (this->empty()) {
    throw std::runtime_error(fmt::format("[{}] can not commit move on empty "
    "playlist {}.", FUNCDINFO, playlist_move_cmd_cstr(move_cmd)));
  }

  std::size_t next = playlist_get_move(this->m_qi, this->m_q.size(), this->m_loop_type, move_cmd);
  if (next == Playlist::npos) {
    throw std::runtime_error(fmt::format("[{}] can not commit move {}",
    FUNCDINFO, playlist_move_cmd_cstr(move_cmd)));
  }

  return this->m_entries[this->m_q[next]];
}

bool Playlist::can_move(PlaylistMvCmd move_cmd) const noexcept {
  if (this->empty()) return false;
  return playlist_get_move(this->m_qi, this->m_q.size(), this->m_loop_type, move_cmd) != Playlist::npos;
}

void Playlist::shuffle(bool keep_current_file_first) {
  if (this->empty()) {
    this->m_shuffled = true;
    return;
  }

  if (keep_current_file_first) {
    std::iter_swap(this->m_q.begin(), this->m_q.begin() + this->m_qi);
    effolkronium::random_thread_local::shuffle(this->m_q.begin() + 1, this->m_q.end());
  } else {
    effolkronium::random_thread_local::shuffle(this->m_q);
  }
  this->m_qi = 0;
  this->m_shuffled = true;
}

void Playlist::unshuffle() {
  if (this->empty()) {
    this->m_shuffled = false;
    return;
  }

  this->m_qi = this->m_q[this->m_qi];
  std::sort(this->m_q.begin(), this->m_q.end());
  this->m_shuffled = false;
}

std::size_t playlist_get_rewind(std::size_t current, std::size_t size, LoopType loop_type);
std::size_t playlist_get_next(std::size_t current, std::size_t size, LoopType loop_type);
std::size_t playlist_get_skip(std::size_t current, std::size_t size, LoopType loop_type);

std::size_t playlist_get_move(std::size_t current, std::size_t size, LoopType loop_type, PlaylistMvCmd move_cmd) {
  switch (move_cmd) {
    case PlaylistMvCmd::NEXT: return playlist_get_next(current, size, loop_type);
    case PlaylistMvCmd::SKIP: return playlist_get_skip(current, size, loop_type);
    case PlaylistMvCmd::REWIND: return playlist_get_rewind(current, size, loop_type);
  }
  return Playlist::npos;
}

std::size_t playlist_get_next(std::size_t current, std::size_t size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current + 1 == size ? Playlist::npos : current + 1;
    case LoopType::LOOP: return current + 1 == size ? 0 : current + 1;
    case LoopType::LOOP_ONE: return current;
  }
  return Playlist::npos;
}

std::size_t playlist_get_skip(std::size_t current, std::size_t size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current + 1 == size ? Playlist::npos : current + 1;
    case LoopType::LOOP:
    case LoopType::LOOP_ONE: return current + 1 == size ? 0 : current + 1;
  }
  return Playlist::npos;
}

std::size_t playlist_get_rewind(std::size_t current, std::size_t size, LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return current == 0 ? 0 : current - 1;
    case LoopType::LOOP:
    case LoopType::LOOP_ONE: return current == 0 ? size - 1 : current - 1;
  }
  return Playlist::npos;
}

const char* loop_type_cstr(LoopType loop_type) {
  switch (loop_type) {
    case LoopType::NO_LOOP: return "no-loop";
    case LoopType::LOOP: return "repeat";
    case LoopType::LOOP_ONE: return "repeat-one";
  }
  return "unknown";
}

LoopType loop_type_from_str(std::string_view str) {
  if (str == "no-loop") {
    return LoopType::NO_LOOP;
  } else if (str == "repeat") {
    return LoopType::LOOP;
  } else if (str == "repeat-one") {
    return LoopType::LOOP_ONE;
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
