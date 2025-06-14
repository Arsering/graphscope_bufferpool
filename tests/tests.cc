#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <bitset>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include <assert.h>
#include <ctime>
#include <random>
#include <string_view>
#include "tests.h"

using namespace gbp;

namespace test {
std::atomic<bool> mark_stop = true;

void set_cpu_affinity() {
  static std::atomic<size_t> cpu_id = 0;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id.fetch_add(1), &cpuset);

  pid_t pid = getpid();  // 获取当前进程的 PID

  if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset) != 0) {
    perror("sched_setaffinity");
    exit(EXIT_FAILURE);
  }
}
std::vector<std::vector<size_t>>& get_trace_global() {
  static std::vector<std::vector<size_t>> traces;
  return traces;
}

void write_mmap(char* data_file_mmaped, size_t file_size_inByte, size_t io_size,
                size_t start_offset, size_t thread_id) {
  assert(file_size_inByte % sizeof(size_t) == 0);
  assert(start_offset % sizeof(size_t) == 0);

  size_t io_num = file_size_inByte / sizeof(size_t);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> rnd(0, io_num);

  size_t curr_io_fileoffset, ret;
  for (size_t io_id = 0; io_id < io_num; io_id++) {
    curr_io_fileoffset = start_offset + io_id * sizeof(size_t);
    size_t data = curr_io_fileoffset / sizeof(size_t);
    memcpy(data_file_mmaped + curr_io_fileoffset, &data, sizeof(size_t));
    assert(*reinterpret_cast<size_t*>(data_file_mmaped + curr_io_fileoffset) ==
           data);

    gbp::PerformanceLogServer::GetPerformanceLogger()
        .GetClientWriteThroughputByte()
        .fetch_add(sizeof(size_t));
  }
}

void read_mmap(char* data_file_mmaped, size_t file_size_inByte,
               size_t io_size_in, size_t start_offset, size_t thread_id) {
  std::ofstream latency_log(gbp::get_log_dir() + "/" +
                            std::to_string(thread_id) + ".log");
  latency_log << "read_mmap" << std::endl;
  size_t io_num = file_size_inByte / sizeof(size_t) - 10;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> rnd(0, io_num);
  std::uniform_int_distribution<uint64_t> rnd_io_size(1, 1024 * 5);

  size_t curr_io_fileoffset, ret, offset_tmp;
  size_t st, io_id, io_size;
  int count = 1;
  while (count != 0) {
    count--;
    // size_t query_count = get_trace_global()[thread_id].size();
    size_t query_count = 50000000;

    // for (io_id = 0; io_id < io_num; io_id += io_size_in / sizeof(size_t)) {
    while (query_count != 0) {
      // query_count--;
      // io_id = rnd(gen);
      // io_id = ZipfianGenerator::GetGen().generate() *
      //             (io_num / ZipfianGenerator::GetGen().GetN()) +
      //         rnd(gen) % (io_num / ZipfianGenerator::GetGen().GetN());
      // io_id = fileoffsetgenerator::GetGen().generate_offset() /
      // sizeof(size_t);
      // io_id = rnd(gen);
      //   // io_size = rnd_io_size(gen) * sizeof(size_t);
      //   io_size = 8 * 512;
      io_size = io_size_in;
      io_id = io_id / 512 * 512;

      curr_io_fileoffset = start_offset + io_id * sizeof(size_t);
      // curr_io_fileoffset =
      //     get_trace_global()[thread_id][query_count] - 139874067804160;
      io_size = std::min(io_size, file_size_inByte - curr_io_fileoffset);

#ifdef DEBUG_1
      st = gbp::GetSystemTime();
#endif
      {
        if constexpr (true) {
          for (size_t i = 0; i < io_size / sizeof(size_t); i++) {
            // if ((*reinterpret_cast<size_t*>(data_file_mmaped +
            //                                 curr_io_fileoffset +
            //                                 i * sizeof(size_t)) !=
            //      (curr_io_fileoffset / sizeof(size_t) + i)))
            //   std::cout << *reinterpret_cast<size_t*>(data_file_mmaped +
            //                                           curr_io_fileoffset +
            //                                           i * sizeof(size_t))
            //             << " | " << (curr_io_fileoffset / sizeof(size_t) + i)
            //             << std::endl;
            assert(*reinterpret_cast<size_t*>(data_file_mmaped +
                                              curr_io_fileoffset +
                                              i * sizeof(size_t)) ==
                   (curr_io_fileoffset / sizeof(size_t) + i));
          }
        }
      }
#ifdef DEBUG_1
      st = gbp::GetSystemTime() - st;
      latency_log << st << std::endl;
#endif
      gbp::PerformanceLogServer::GetPerformanceLogger()
          .GetClientReadThroughputByte()
          .fetch_add(io_size);
    }
  }
  latency_log.flush();
  latency_log.close();

  std::cout << "thread " << thread_id << " exits" << std::endl;
}

void read_bufferpool(size_t start_offset, size_t file_size_inByte,
                     size_t io_size_in, size_t thread_id) {
  assert(io_size_in % sizeof(size_t) == 0);
  std::ofstream latency_log(gbp::get_log_dir() + "/" +
                            std::to_string(thread_id) + ".log");
  latency_log << "read_bufferpool" << std::endl;
  size_t io_num = (file_size_inByte - io_size_in) / sizeof(size_t) - 10;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> rnd(0, io_num);
  std::uniform_int_distribution<uint64_t> rnd_io_size(1, 1024 * 5);

  auto& bpm = gbp::BufferPoolManager::GetGlobalInstance();

  size_t curr_io_fileoffset, ret, io_size;
  size_t st, io_id;
  size_t batch_size = 80;
  std::vector<std::future<BufferBlock>> block_container(batch_size);
  std::vector<gbp::batch_request_type> requests(batch_size);
  std::vector<size_t> io_file_offsets(batch_size);
  int count = 1;

  while (count != 0) {
    // count--;

    // size_t query_count = get_trace_global()[thread_id].size();

    // for (io_id = 0; io_id < io_num; io_id += io_size_in / sizeof(size_t)) {
    // io_size = sizeof(size_t);

    io_id = rnd(gen);

    // io_id = ZipfianGenerator::GetGen().generate() *
    //             (io_num / ZipfianGenerator::GetGen().GetN()) +
    //         rnd(gen) % (io_num / ZipfianGenerator::GetGen().GetN());
    // io_id = fileoffsetgenerator::GetGen().generate_offset() /
    // sizeof(size_t);
    //   // io_size = rnd_io_size(gen) * sizeof(size_t);
    //   io_size = 9 * 512;
    io_id = io_id / 512 * 512;
    curr_io_fileoffset = start_offset + io_id * sizeof(size_t);
    // curr_io_fileoffset =
    //     get_trace_global()[thread_id][query_count] - 139874067804160;
    io_size = io_size_in;
    io_size = std::min(io_size, file_size_inByte - curr_io_fileoffset);

    // st = gbp::GetSystemTime();
    size_t id_in_batch = 0;
    while (batch_size != id_in_batch) {
      io_id = rnd(gen);
      io_size = io_size_in;
      io_id = io_id / 512 * 512;
      curr_io_fileoffset = start_offset + io_id * sizeof(size_t);
      io_size = std::min(io_size, file_size_inByte - curr_io_fileoffset);

      // block_container[id_in_batch] =
      //     bpm.GetBlockAsync(curr_io_fileoffset, io_size);
      //     io_file_offsets[id_in_batch] = curr_io_fileoffset;

      requests[id_in_batch] = {curr_io_fileoffset, io_size, 0};

      // {
      //   // auto block = bpm.GetBlockAsync1(curr_io_fileoffset, io_size, 0);
      //   auto block = bpm.GetBlockSync(curr_io_fileoffset, io_size);

      //   // auto block =
      //   //     bpm.GetBlockWithDirectCacheSync(curr_io_fileoffset,
      //   // io_size);
      //   if constexpr (true) {
      //     // auto ret_new = bpm.GetObject(curr_io_fileoffset, io_size);
      //     // auto iter = gbp::BufferBlockIter<size_t>(ret_new);
      //     for (size_t i = 0; i < block.Size() / sizeof(size_t); i++) {
      //       // assert(gbp::BufferBlock::Ref<size_t>(block, i) ==
      //       //        (curr_io_fileoffset / sizeof(size_t) + i));
      //       // assert(*(iter.current()) ==
      //       //        (curr_io_fileoffset / sizeof(size_t) + i));
      //       // iter.next();
      //     }
      //     // assert(iter.current() == nullptr);
      //   }
      // }
      id_in_batch++;
    }

    // for (auto req_idx = 0; req_idx < batch_size; req_idx++) {
    //   // auto block = bpm.GetBlockBatch1(requests[req_idx].file_offset_,
    //   //                                 requests[req_idx].block_size_,
    //   //                                 requests[req_idx].fd_);
    //   auto block = bpm.GetBlockSync(requests[req_idx].file_offset_,
    //                                 requests[req_idx].block_size_,
    //                                 requests[req_idx].fd_);
    //   for (size_t i = 0; i < block.Size() / sizeof(size_t); i++) {
    //     assert(gbp::BufferBlock::Ref<size_t>(block, i) ==
    //            (requests[req_idx].file_offset_ / sizeof(size_t) + i));
    //   }
    // }

    std::vector<BufferBlock> results;
    results.reserve(batch_size);
    bpm.GetBlockBatch(requests, results);
    for (size_t i = 0; i < batch_size; i++) {
      auto& item = results[i];
      for (size_t j = 0; j < item.GetSize() / sizeof(size_t); j++) {
        assert(gbp::BufferBlock::Ref<size_t>(item, j) ==
               (requests[i].file_offset_ / sizeof(size_t) + j));
      }
    }

    // for (size_t i = 0; i < batch_size; i++) {
    //   auto item = block_container[i].get();
    //   for (size_t j = 0; j < item.GetSize() / sizeof(size_t); j++) {
    //     assert(gbp::BufferBlock::Ref<size_t>(item, j) ==
    //            (io_file_offsets[i] / sizeof(size_t) + j));
    //   }
    // }

    // st = gbp::GetSystemTime() - st;
    // latency_log << st << std::endl;
    // latency_log << st << " | " << gbp::get_counter(1) << " | "
    //             << gbp::get_counter(2) << " | " << gbp::get_counter(11)
    //             << " | " << gbp::get_counter(12) << std::endl;

    gbp::PerformanceLogServer::GetPerformanceLogger()
        .GetClientReadThroughputByte()
        .fetch_add(io_size * batch_size);
  }
  latency_log.flush();
  latency_log.close();
  GBPLOG << "thread " << thread_id << " exits" << std::endl;
}

void write_bufferpool(size_t start_offset, size_t file_size_inByte,
                      size_t io_size, size_t thread_id) {
  // gbp::debug::get_thread_id() = thread_id;
  assert(io_size % sizeof(size_t) == 0);
  size_t io_num = file_size_inByte / io_size;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> rnd(0, io_num);

  auto& bpm = gbp::BufferPoolManager::GetGlobalInstance();
  char* out_buf = (char*) aligned_alloc(512, io_size);
  // ::memset(out_buf, 0, io_size);
  // {
  //   char* str = "abcdefg";
  //   size_t buf_size = 0;
  //   while (buf_size < io_size)
  //   {
  //     buf_size += ::snprintf(out_buf + buf_size, io_size - buf_size, "%s",
  //       str);
  //   }
  // }
  char* buf = (char*) aligned_alloc(512, io_size);

  size_t curr_io_fileoffset, ret;
  volatile size_t sum = 0;
  size_t st, io_id, page_id;
  for (io_id = 0; io_id < io_num; io_id++) {
    curr_io_fileoffset = start_offset + io_id * io_size;

    // while (true)
    // {
    //   io_id = rnd(gen);
    //   curr_io_fileoffset = io_id * io_size;
    auto ret_obj = bpm.GetBlockSync(curr_io_fileoffset, io_size);
    size_t buf_offset =
        4096 -
        (curr_io_fileoffset % 4096 == 0 ? 4096 : curr_io_fileoffset % 4096);
    while (buf_offset < io_size) {
      page_id = (curr_io_fileoffset + buf_offset) / 4096;
      // memcpy(out_buf + buf_offset, &page_id, sizeof(size_t));
      gbp::BufferBlock::UpdateContent<size_t>(
          [&](size_t& content) { content = page_id; }, ret_obj,
          buf_offset / sizeof(size_t));

      // ::snprintf(out_buf + buf_offset, io_size - buf_offset, "%lu",
      // page_id);
      buf_offset += 4096;
    }
    // auto ret = bpm.SetObject(out_buf, curr_io_fileoffset, io_size, 0,
    // false);

    // if (*reinterpret_cast<size_t*>(ret.Data()) != io_id)
    // std::cout << *reinterpret_cast<size_t*>(out_buf) << " | " << io_id <<
    // std::endl;
    gbp::PerformanceLogServer::GetPerformanceLogger()
        .GetClientWriteThroughputByte()
        .fetch_add(io_size);
  }
  // std::cout << "thread_id = " << thread_id << std::endl;
}

std::string random_str(size_t len) {
  std::string test_str = "abcdefghi";
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint64_t> rnd(0, 5);
  std::uniform_int_distribution<uint64_t> len_rnd(10, len);

  size_t rnd_len = len_rnd(gen);
  std::string ret(rnd_len, 'c');
  for (int i = 0; i < rnd_len; i++) {
    size_t tmp = rnd(gen);
    ret.data()[i] = test_str[rnd(gen)];
  }
  assert(ret.size() == rnd_len);
  return ret;
}

void randwrite_bufferpool(size_t start_offset, size_t file_size_inByte,
                          size_t io_size, size_t thread_id) {
  size_t io_num = file_size_inByte / io_size;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> rnd(0, io_num - 10);

  auto& bpm = gbp::BufferPoolManager::GetGlobalInstance();
  char* out_buf = (char*) aligned_alloc(512, io_size);
  ::memset(out_buf, 1, io_size);
  char* out_buf_1 = (char*) aligned_alloc(512, io_size);
  ::memset(out_buf_1, 1, io_size);

  std::string test_str_1, test_str_2;
  size_t curr_io_fileoffset, ret;
  volatile size_t sum = 0;
  size_t st, io_id, page_id;
  // for (io_id = 0; io_id < io_num; io_id++)
  // {
  //   curr_io_fileoffset = start_offset + io_id * io_size;

  while (true) {
    io_id = rnd(gen);
    curr_io_fileoffset = io_id * io_size;
    test_str_1 = random_str(io_size);
    bpm.SetBlock(test_str_1.data(), curr_io_fileoffset, test_str_1.size(), 0,
                 false);
    test_str_2 = random_str(io_size);
    // test_str_2 = test_str_1;
    bpm.SetBlock(test_str_2.data(), curr_io_fileoffset + test_str_1.size(),
                 test_str_2.size(), 0, false);
    auto ret_str_1 = bpm.GetBlockSync(curr_io_fileoffset, test_str_1.size());
    auto ret_str_2 = bpm.GetBlockSync(curr_io_fileoffset + test_str_1.size(),
                                      test_str_2.size());

    bpm.GetBlock(out_buf_1, curr_io_fileoffset, test_str_1.size());
    assert(strncmp(test_str_1.data(), out_buf_1, test_str_1.size()) == 0);
    assert(ret_str_1 == test_str_1);
    assert(ret_str_2 == test_str_2);

    if (test_str_1 > test_str_2) {
      assert(ret_str_1 > ret_str_2);
    } else if (test_str_1 < test_str_2) {
      assert(ret_str_1 < ret_str_2);
    } else {
      assert(ret_str_1 == ret_str_2);
    }
    assert(ret_str_1 == ret_str_1);

    auto aaa = random_str(io_size);
    if (test_str_1 > aaa) {
      assert(ret_str_1 > aaa);
    } else if (test_str_1 < aaa) {
      assert(ret_str_1 < aaa);
    } else {
      assert(ret_str_1 == aaa);
    }
    // if (*reinterpret_cast<size_t*>(ret.Data()) != io_id)
    //   std::cout << *reinterpret_cast<size_t*>(ret.Data()) << " | " << io_id
    //   << std::endl;
    gbp::PerformanceLogServer::GetPerformanceLogger()
        .GetClientWriteThroughputByte()
        .fetch_add(io_size);
  }
}

void read_pread(gbp::IOBackend* io_backend, size_t file_size_inByte,
                size_t io_size_in, size_t start_offset, size_t thread_id) {
  std::ofstream latency_log(gbp::get_log_dir() + "/" +
                            std::to_string(thread_id) + ".log");
  latency_log << "read_pread" << std::endl;
  size_t io_num = file_size_inByte / sizeof(size_t);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> rnd(0, io_num);
  std::uniform_int_distribution<uint64_t> rnd_io_size(1, 1024 * 5);
  std::uniform_int_distribution<uint64_t> rnd_t(0, 100 - 1);
  size_t curr_io_fileoffset, ret, offset_tmp;
  size_t st, io_id, io_size;

  char* buf = (char*) aligned_alloc(4096, io_size_in * 10);
  size_t query_count = 1000LU;
  int count = 1;

  while (count != 0) {
    count--;
    // for (io_id = 0; io_id < io_num; io_id += io_size_in / sizeof(size_t)) {
    while (query_count != 0) {
      //   // query_count--;
      io_id = rnd(gen);
      io_id = io_id / 512 * 512;
      //   // io_size = rnd_io_size(gen) * sizeof(size_t);
      //   io_size = 8 * 512;
      io_size = io_size_in;

      curr_io_fileoffset = start_offset + io_id * sizeof(size_t);
      io_size = std::min(io_size, file_size_inByte - curr_io_fileoffset);
      st = gbp::GetSystemTime();
      {
        for (size_t offset_tmp = 0; offset_tmp < io_size; offset_tmp += 4096) {
          io_backend->Read(curr_io_fileoffset + offset_tmp, buf + offset_tmp,
                           4096, 0);
        }
        // io_backend->Read(curr_io_fileoffset, buf, io_size, 0);
        if constexpr (true) {
          for (size_t i = 0; i < io_size / sizeof(size_t); i++) {
            assert(*reinterpret_cast<size_t*>(buf + i * sizeof(size_t)) ==
                   (curr_io_fileoffset / sizeof(size_t) + i));
            break;
          }
        }
      }
      st = gbp::GetSystemTime() - st;
      latency_log << st << std::endl;

      gbp::PerformanceLogServer::GetPerformanceLogger()
          .GetClientReadThroughputByte()
          .fetch_add(io_size);
    }
  }
  latency_log.flush();
  latency_log.close();

  std::cout << "thread " << thread_id << " exits" << std::endl;
}

void write_pwrite(gbp::IOBackend* io_backend, size_t file_size_inByte,
                  size_t io_size, size_t thread_id) {
  size_t io_num = CEIL(file_size_inByte, io_size) - 1;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> rnd(0, io_num);

  char* out_buf = (char*) aligned_alloc(4096, io_size);
  char* tmp_buf = (char*) aligned_alloc(4096, io_size);

  ::memset(out_buf, 1, io_size);

  size_t curr_io_fileoffset, ret, page_id;

  while (true) {
    curr_io_fileoffset = rnd(gen) * io_size;
    page_id = curr_io_fileoffset / 4096;
    memcpy(out_buf, &page_id, sizeof(size_t));
    memcpy(tmp_buf, out_buf, io_size);
    // ret = ::pwrite(fd_os, out_buf, io_size, curr_io_fileoffset);

    io_backend->Write(curr_io_fileoffset, {out_buf, io_size}, 0);
    // assert(ret == io_size);

    gbp::PerformanceLogServer::GetPerformanceLogger()
        .GetClientWriteThroughputByte()
        .fetch_add(io_size);
  }
  // std::cout << thread_id << std::endl;
}

void warmup_mmap_inner(char* data_file_mmaped, size_t file_size_inByte,
                       size_t io_size, size_t start_offset,
                       std::atomic<size_t>& memory_used) {
  size_t curr_io_fileoffset, ret;
  volatile size_t sum = 0;
  size_t io_num = CEIL(file_size_inByte, io_size);

  for (size_t io_id = 0; io_id < io_num; io_id++) {
    curr_io_fileoffset = io_id * io_size;
    curr_io_fileoffset = curr_io_fileoffset + io_size < file_size_inByte
                             ? start_offset + curr_io_fileoffset
                             : start_offset + file_size_inByte - io_size;

    for (size_t i = 0; i < io_size; i += 4096) {
      sum += data_file_mmaped[curr_io_fileoffset + i];
    }
    memory_used.fetch_add(io_size);
    if (memory_used.load() / (1024LU * 1024LU * 1024LU) > 200) {
      break;
    }
  }
}

void warmup_bufferpool_inner(char* data_file_mmaped, size_t file_size_inByte,
                             size_t io_size, size_t start_offset,
                             std::atomic<size_t>& memory_used) {
  size_t curr_io_fileoffset;
  volatile size_t sum = 0;
  size_t io_num = CEIL(file_size_inByte, io_size);
  auto& bpm = gbp::BufferPoolManager::GetGlobalInstance();

  for (size_t io_id = 0; io_id < io_num; io_id++) {
    curr_io_fileoffset = io_id * io_size;
    curr_io_fileoffset = curr_io_fileoffset + io_size < file_size_inByte
                             ? start_offset + curr_io_fileoffset
                             : start_offset + file_size_inByte - io_size;
    auto ret = bpm.GetBlockSync(curr_io_fileoffset, io_size);
    // for (size_t i = 0; i < io_size; i += 4096)
    // {
    //   sum += ret.Data()[i];
    // }
    memory_used.fetch_add(io_size);
    if (memory_used.load() / (1024LU * 1024LU * 1024LU) > 200) {
      break;
    }
  }
}
void warmup(char* data_file_mmaped, size_t file_size_inByte, size_t io_size) {
  std::cout << "warm up start" << std::endl;

  size_t worker_num = 150;
  std::atomic<size_t> memory_used{0};
  std::vector<std::thread> thread_pool;

  size_t file_size_perFile = file_size_inByte / worker_num;
  for (size_t i = 0; i < worker_num; i++) {
    // thread_pool.emplace_back(warmup_mmap_inner, data_file_mmaped,
    //                          file_size_perFile, io_size, file_size_perFile
    //                          * i, std::ref(memory_used));
    thread_pool.emplace_back(warmup_bufferpool_inner, data_file_mmaped,
                             file_size_perFile, io_size, file_size_perFile * i,
                             std::ref(memory_used));
  }

  for (auto& thread : thread_pool) {
    thread.join();
  }
  std::cout << "warm up finish" << std::endl;
}

void extra_fun(bool& stop) {
  std::cout << "extra_fun: start" << std::endl;
  size_t buf_size = 1024LU * 1024LU * 1024LU * 120;
  char* buf = (char*) malloc(buf_size);
  volatile size_t sum = 0;
  for (size_t i = 0; i < buf_size; i += 1) {
    buf[i] = i;
  }
  std::cout << "extra_fun: init finished" << std::endl;

  while (true) {
    sleep(1);
    if (stop) {
      std::cout << "extra_fun: stop" << std::endl;
      return;
    }
  }
}
std::vector<std::vector<size_t>> read_trace(const std::string& trace_dir,
                                            size_t work_num) {
  std::vector<std::vector<size_t>> vecs(30, std::vector<size_t>());

  std::vector<std::thread> thread_pool;
  for (int thread_id = 1; thread_id < work_num + 1; thread_id++) {
    thread_pool.emplace_back([&, thread_id]() {
      std::string trace_file_path =
          trace_dir + "/thread_log_" + std::to_string(thread_id) + ".log";
      std::cout << __FILE__ << ":" << __LINE__ << ": " << trace_file_path
                << std::endl;
      std::ifstream trace_file(trace_file_path);
      assert(!!trace_file);

      for (std::string line = ""; std::getline(trace_file, line);) {
        std::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of(" "),
                     boost::token_compress_on);
        vecs[thread_id - 1].emplace_back(std::stoull(strs[0]));
      }
      std::cout << __FILE__ << ":" << __LINE__ << ": "
                << vecs[thread_id - 1].size() << std::endl;
    });
  }
  for (auto& thread : thread_pool) {
    thread.join();
  }
  return std::move(vecs);
}

int test_concurrency(int argc, char** argv) {
  size_t file_size_MB = std::stoull(argv[1]);
  size_t worker_num = std::stoull(argv[2]);
  size_t pool_num = std::stoull(argv[3]);
  size_t pool_size_MB = std::stoull(argv[4]);
  size_t io_server_num = std::stoull(argv[5]);
  size_t io_size = std::stoull(argv[6]);

  // set_cpu_affinity(2);

  // std::cout << log_directory << std::endl;
  // std::string file_path = "tests/db/test1.db";
  // std::string file_path = "/home/spdk/p4510/zhengyang/test_write.db";
  // std::string file_path = "/home/spdk/p4510/zhengyang/test_read.db";

  std::string file_path = "/mnt/nvme/test_read.db";
  // std::string trace_dir =
  //     "/data/zhengyang/data/experiment_space/LDBC_SNB/logs/"
  //     "2024-06-06-20:05:02/"
  //     "server/graphscope_logs";
  // get_trace_global() = read_trace(trace_dir, worker_num);

  // std::string file_path = "/nvme0n1/test_read.db";

  size_t file_size_inByte = 1024LU * 1024LU * file_size_MB;
  int data_file = -1;
  data_file = ::open(file_path.c_str(), O_RDWR | O_CREAT | O_DIRECT, 0777);
  assert(data_file != -1);
  // ::ftruncate(data_file, file_size_inByte);

  char* data_file_mmaped = nullptr;

  data_file_mmaped = (char*) ::mmap(
      NULL, file_size_inByte, PROT_READ | PROT_WRITE, MAP_SHARED, data_file, 0);
  assert(data_file_mmaped != nullptr);
  ::madvise(data_file_mmaped, file_size_inByte,
            MADV_RANDOM);  // Turn off readahead

  size_t pool_size_page =
      pool_size_MB * 1024LU * 1024LU / gbp::PAGE_SIZE_MEMORY / pool_num + 1;

  // gbp::DiskManager disk_manager(file_path);
  // gbp::IOBackend* io_backend = new gbp::RWSysCall(&disk_manager);

  auto& bpm = gbp::BufferPoolManager::GetGlobalInstance();
  bpm.init(pool_num, pool_size_page, io_server_num, file_path);
  // bpm.Resize(0, file_size_inByte);
  gbp::log_enable().store(0);
  // std::cout << "warm up starting" << std::endl;
  // bpm.WarmUp();
  // std::cout << "warm up finishing" << std::endl;
  gbp::log_enable().store(1);

  // worker_num = file_size_inByte / (1024LU * 1024LU * 1024LU * 1);
  // file_size_inByte = file_size_inByte / worker_num;

  printf(
      "file_size_MB = %lu\tworker_num = %lu\tpool_num = %lu\tpool_size_MB = "
      "%lu\tio_server_num = %lu\tio_size = %lu\n",
      file_size_MB, worker_num, pool_num, pool_size_MB, io_server_num, io_size);
  // warmup(data_file_mmaped, file_size_inByte, io_size);

  std::filesystem::create_directory(std::string{argv[7]} + "/latency");
  gbp::get_log_dir() = std::string{argv[7]} + "/latency";
  gbp::get_db_dir() = file_path;

  // ZipfianGenerator::GetGen().Init(1.6, 256 * 1024);
  // fileoffsetgenerator::GetGen().Init(file_size_inByte, 5, 512 * 128 * 1024,
  //                                    0.9);
  // std::thread extra_thread(extra_fun, std::ref(extra_thread_stop));
  sleep(10);
  gbp::PerformanceLogServer::GetPerformanceLogger().Start(
      std::string{argv[7]} + "/performance.log", "nvme0n1");

  std::vector<std::thread> thread_pool;
  size_t ssd_io_byte = std::get<0>(gbp::SSD_io_bytes());
  for (size_t i = 0; i < worker_num; i++) {
    // thread_pool.emplace_back(write_mmap, data_file_mmaped,
    //                          (1024LU * 1024LU * 1024LU * 1), io_size,
    //                          (1024LU * 1024LU * 1024LU * 1) * i, i);
    // thread_pool.emplace_back(read_mmap, data_file_mmaped, file_size_inByte,
    //                          io_size, 0, i);
    // thread_pool.emplace_back(read_pread, io_backend, file_size_inByte,
    // io_size,
    //                          0, i);
    // thread_pool.emplace_back(write_pwrite, io_backend, file_size_inByte,
    //                          io_size, i);
    // thread_pool.emplace_back(fiber_pread_1_2, &disk_manager,
    // file_size_inByte,
    //                          io_size, i);
    // thread_pool.emplace_back(read_mmap,
    // data_file_mmaped, file_size_inByte, io_size, i);
    // thread_pool.emplace_back(read_bufferpool, 0, file_size_inByte, io_size,
    // i); thread_pool.emplace_back(write_bufferpool, file_size_inByte * i,
    // file_size_inByte, io_size, i);
    // if (false)
    //   thread_pool.emplace_back(write_bufferpool, 0, file_size_inByte,
    //   io_size, i);
    // else

    thread_pool.emplace_back(read_bufferpool, 0, file_size_inByte, io_size, i);

    // thread_pool.emplace_back(randwrite_bufferpool, 0, file_size_inByte,
    // io_size,
    //                          i);
    // thread_pool.emplace_back(write_bufferpool, 0, file_size_inByte,
    // io_size, i);
  }
  sleep(1);
  mark_stop = false;
  // std::vector<std::pair<size_t, size_t>> times(worker_num, {100, 100});
  // while (true) {
  //   for (auto worker_id = 0; worker_id < worker_num; worker_id++) {
  //     size_t v_cur = gbp::get_counter_global(worker_id);
  //     if (v_cur % 2 == 1) {
  //       if (v_cur == times[worker_id].first &&
  //           gbp::GetSystemTime() - times[worker_id].second > 2.7e9) {
  //         assert(false);
  //       }
  //       if (v_cur != times[worker_id].first) {
  //         times[worker_id].first = v_cur;
  //         times[worker_id].second = gbp::GetSystemTime();
  //       }
  //     }
  //   }
  // }
  for (auto& thread : thread_pool) {
    thread.join();
  }
  ssd_io_byte = std::get<0>(gbp::SSD_io_bytes()) - ssd_io_byte;
  std::cout << "SSD IO = " << ssd_io_byte << "B" << std::endl;

  return 0;
}
}  // namespace test