#include <numa.h>
#include <pthread.h>
#include <xmmintrin.h>
#include <boost/circular_buffer.hpp>
#include <boost/fiber/all.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

#include "tests.h"

namespace test {

  struct context_type {

    enum class Type { Pin, UnPin } type;
    enum class Phase { Begin, Initing, Evicting, Loading, End } phase;
    enum class State { Commit, Poll, End } state;

    gbp::IOURing* io_backend;
    bool finish = false;

    FORCE_INLINE static context_type GetRawObject(gbp::IOURing* io_backend) {
      return  { Type::Pin, Phase::Begin, State::Commit, io_backend,
                   false };

    }
  };

  struct async_request_fiber_type {
    async_request_fiber_type() = default;
    async_request_fiber_type(std::vector<::iovec>& _io_vec, gbp::fpage_id_type _fpage_id_start,
      gbp::fpage_id_type _page_num,
      gbp::GBPfile_handle_type _fd,
      context_type& _async_context) : fpage_id_start(_fpage_id_start), page_num(_page_num), fd(_fd), async_context(_async_context) {
      io_vec.swap(_io_vec);
    }
    async_request_fiber_type(char* buf, size_t buf_size, gbp::fpage_id_type _fpage_id_start,
      gbp::fpage_id_type _page_num,
      gbp::GBPfile_handle_type _fd,
      context_type& _async_context) :io_vec_size(1), fpage_id_start(_fpage_id_start), page_num(_page_num), fd(_fd), async_context(_async_context) {
      // io_vec.emplace_back(buf, buf_size);
      io_vec.resize(1);
      io_vec[0].iov_base = buf;
      io_vec[0].iov_len = buf_size;

    }

    ~async_request_fiber_type() = default;

    std::vector<::iovec> io_vec;
    size_t io_vec_size;
    gbp::fpage_id_type fpage_id_start;
    gbp::fpage_id_type page_num;
    gbp::GBPfile_handle_type fd;
    context_type async_context;
  };

  // bool process_func(context_type& context, async_request_fiber_type& req) {
  //   auto ret = ::pread(req.fd, req.in_buf, req.io_size, req.file_offset);
  //   assert(ret == req.io_size);
  //   IO_throughput().fetch_add(req.io_size);
  //   // std::cout << *reinterpret_cast<size_t*>(req.in_buf) << std::endl;
  //   assert(*reinterpret_cast<size_t*>(req.in_buf) ==
  //          req.file_offset / req.io_size);
  //   return true;
  // }

  bool process_func(async_request_fiber_type& req) {
    switch (req.async_context.state) {
    case  context_type::State::Commit: { // 将read request提交至io_uring
      auto ret = req.async_context.io_backend->Read(
        req.fpage_id_start, req.io_vec.data(), req.fd, &req.async_context.finish);
      while (!ret) {
        ret = req.async_context.io_backend->Read(
          req.fpage_id_start, req.io_vec.data(), req.fd, &req.async_context.finish);// 不断尝试提交请求直至提交成功
      }

      if (!req.async_context.finish)
      {
        req.async_context.io_backend->Progress();
        req.async_context.state = context_type::State::Poll;
        return false;
      }
      else {
        IO_throughput().fetch_add(req.page_num * gbp::PAGE_SIZE_FILE);
        req.async_context.state = context_type::State::End;
        return true;
      }
    }
    case context_type::State::Poll: {
      req.async_context.io_backend->Progress();
      if (req.async_context.finish) {
        IO_throughput().fetch_add(req.page_num * gbp::PAGE_SIZE_FILE);
        req.async_context.state = context_type::State::End;
        return true;
      }
      break;
    }
    case context_type::State::End: {
      return true;
    }
    }
    return false;
  }

  void fiber_pread(gbp::DiskManager* disk_manager, size_t file_size_inByte, size_t io_size, size_t thread_id) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> rnd(
      0, gbp::ceil(file_size_inByte, io_size) - 1);
    size_t io_num = file_size_inByte / io_size;

    gbp::IOURing io_backend(disk_manager);

    boost::circular_buffer<std::optional<async_request_fiber_type>>
      async_requests(gbp::FIBER_CHANNEL_DEPTH);

    gbp::fpage_id_type fpage_id;
    for (int idx = 0;idx < gbp::FIBER_BATCH_SIZE;idx++) {
      fpage_id = rnd(gen);
      char* in_buf = (char*)aligned_alloc(gbp::PAGE_SIZE_FILE, io_size);
      // ::memset(in_buf, 1, io_size);
      context_type context = context_type::GetRawObject(&io_backend);
      async_request_fiber_type req(in_buf, io_size, (gbp::fpage_id_type)fpage_id, 1, 0,
        context);
      async_requests.push_back(req);
    }

    while (true) {
      for (auto& req : async_requests) {
        if (!req.has_value())
          continue;
        if (process_func(req.value())) {
          assert(*reinterpret_cast<gbp::fpage_id_type*>(req.value().io_vec[0].iov_base) == req.value().fpage_id_start);
          fpage_id = rnd(gen);

          context_type context = context_type::GetRawObject(&io_backend);
          async_request_fiber_type req_new((char*)req.value().io_vec[0].iov_base, io_size, (gbp::fpage_id_type)fpage_id, 1, 0,
            context);
          req.emplace(req_new);
        }
      }
    }

  }
}  // namespace test