/**
 * disk_manager.h
 *
 * Disk manager takes care of the allocation and deallocation of pages within a
 * database. It also performs read and write of pages to and from disk, and
 * provides a logical file layer within the context of a database management
 * system.
 */

#pragma once
#include <atomic>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include "config.h"
#include "debug.h"
#include "logger.h"

namespace gbp {

class DiskManager {
  friend class BufferPoolManager;
  friend class BufferPool;

 public:
  DiskManager(const std::string& db_file);
  DiskManager();
  ~DiskManager();

  inline int GetFileDescriptor(GBPfile_handle_type fd) {
    assert(fd < fd_oss_.size());
    return fd_oss_[fd].first;
  }

  void WritePage(fpage_id_type page_id, const char* page_data,
                 GBPfile_handle_type fd = 0);
  void ReadPage(fpage_id_type page_id, char* page_data,
                GBPfile_handle_type fd = 0) const;

  mpage_id_type AllocatePage();
  void DeallocatePage(mpage_id_type page_id);

  int GetNumFlushes() const;
  bool GetFlushState() const;
  inline void SetFlushLogFuture(std::future<void>* f) { flush_log_f_ = f; }
  inline bool HasFlushLogFuture() { return flush_log_f_ != nullptr; }
  size_t GetFileSize(OSfile_handle_type fd) const;
  int Resize(GBPfile_handle_type fd, size_t new_size);

#ifdef DEBUG
  void ReinitBitMap() {
    auto& bit_maps = debug::get_bitmaps();
    for (auto& bit_map : bit_maps) {
      bit_map.reset_all();
    }
    if (bit_maps.size() < file_sizes_.size()) {
      for (int file_idx = bit_maps.size(); file_idx < file_sizes_.size();
           file_idx++) {
        bit_maps.emplace_back(
            cell(file_sizes_[file_idx], PAGE_SIZE_BUFFER_POOL));
      }
    }
  }
#endif

 private:
  inline GBPfile_handle_type OpenFile(const std::string& file_name,
                                      int o_flag = O_RDWR | O_CREAT) {
    auto fd_os = ::open(file_name.c_str(), o_flag);
    fd_oss_.push_back(std::make_pair(fd_os, true));
    file_names_.push_back(file_name);
    file_sizes_.push_back(GetFileSize(fd_os));
#ifdef DEBUG
    debug::get_bitmaps().emplace_back(
        cell(file_sizes_[file_sizes_.size() - 1], PAGE_SIZE_BUFFER_POOL));
#endif

    return fd_oss_.size() - 1;
  }
  inline void CloseFile(GBPfile_handle_type fd) {
    auto fd_os = GetFileDescriptor(fd);
    close(fd_os);
    fd_oss_[fd].second = false;
  }

  // stream to write log file
  int log_io_;
  std::string log_name_;

  // stream to write db file
  std::vector<std::pair<int, bool>> fd_oss_;
  std::vector<std::string> file_names_;
  std::atomic<mpage_id_type> next_page_id_;
  int num_flushes_;
  bool flush_log_;
  std::future<void>* flush_log_f_;

  std::vector<size_t> file_sizes_;
};

}  // namespace gbp