#pragma once

#include <numa.h>
#include <pthread.h>
#include <xmmintrin.h>
#include <boost/circular_buffer.hpp>
#include <boost/fiber/all.hpp>
#include <cstddef>
#include <cstdint>
// #include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include "config.h"
#include "io_backend.h"
#include "utils.h"

namespace gbp
{

  struct context_type
  {
    enum class Type
    {
      Pin,
      UnPin
    } type;
    enum class Phase
    {
      Begin,
      Initing,
      Evicting,
      Loading,
      End
    } phase;
    enum class State
    {
      Commit,
      Poll,
      End
    } state;

    bool finish = false;

    FORCE_INLINE static context_type GetRawObject()
    {
      return { Type::Pin, Phase::Begin, State::Commit,
              false };
    }
  };

  struct async_request_fiber_type
  {
    async_request_fiber_type() = default;
    async_request_fiber_type(std::vector<::iovec>& _io_vec, fpage_id_type _fpage_id_start,
      fpage_id_type _page_num,
      GBPfile_handle_type _fd,
      context_type& _async_context, bool _read = true) : fpage_id_start(_fpage_id_start), page_num(_page_num), fd(_fd), async_context(_async_context), success(false), read(_read)
    {
      io_vec.swap(_io_vec);
    }
    async_request_fiber_type(char* buf, size_t buf_size, fpage_id_type _fpage_id_start,
      fpage_id_type _page_num,
      GBPfile_handle_type _fd,
      context_type& _async_context, bool _read = true) : io_vec_size(1), fpage_id_start(_fpage_id_start), page_num(_page_num), fd(_fd), async_context(_async_context), success(false), read(_read)
    {
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
    bool read; // read = true || write = false
    std::atomic<bool> success;
  };

  class IOServer_old
  {
  public:
    IOServer_old(DiskManager* disk_manager) : request_channel_(), num_async_fiber_processing_(0), stop_(false)
    {
      if constexpr (IO_BACKEND_TYPE == 1)
        io_backend_ = new RWSysCall(disk_manager);
      else if constexpr (IO_BACKEND_TYPE == 2)
        io_backend_ = new IOURing(disk_manager);
      else
        assert(false);

      server_ = std::thread([this]()
        { Run(); });
    }
    ~IOServer_old()
    {
      stop_ = true;
      if (server_.joinable())
        server_.join();
    }
    IOBackend* io_backend_;

    bool SendRequest(async_request_fiber_type* req, bool blocked = true)
    {
      if (unlikely(req == nullptr))
        return false;

      if (likely(blocked))
        while (!request_channel_.push(req))
          ;
      else
      {
        return request_channel_.push(req);
      }

      return true;
    }

    // std::tuple<bool, PointerWrapper<async_request_fiber_type>> SendRequest(GBPfile_handle_type fd, fpage_id_type fpage_id_start, fpage_id_type page_num, char* buf, bool blocked = true)
    // {
    //   assert(buf != nullptr);
    //   context_type context = context_type::GetRawObject();
    //   auto* req = new async_request_fiber_type(buf, PAGE_SIZE_FILE, fpage_id_start, 1, fd, context);
    //   SendRequest(req, blocked);
    //   return { SendRequest(req, blocked), req };
    // }

    std::tuple<bool, std::shared_ptr<async_request_fiber_type>> SendRequest(GBPfile_handle_type fd, fpage_id_type fpage_id_start, fpage_id_type page_num, char* buf, bool blocked = true)
    {
      assert(buf != nullptr);
      context_type context = context_type::GetRawObject();
      std::shared_ptr<async_request_fiber_type> req(new async_request_fiber_type(buf, PAGE_SIZE_FILE, fpage_id_start, 1, fd, context));
      return { SendRequest(req.get(), blocked), req };
    }

  private:
    bool ProcessFunc(async_request_fiber_type& req)
    {
      switch (req.async_context.state)
      {
      case context_type::State::Commit:
      { // 将read request提交至io_uring
        if (req.read) {
          auto ret = io_backend_->Read(
            req.fpage_id_start, req.io_vec.data(), req.fd, &req.async_context.finish);
          while (!ret)
          {
            ret = io_backend_->Read(
              req.fpage_id_start, req.io_vec.data(), req.fd, &req.async_context.finish); // 不断尝试提交请求直至提交成功
          }
        }
        else {
          auto ret = io_backend_->Write(
            req.fpage_id_start, req.io_vec.data(), req.fd, &req.async_context.finish);
          while (!ret)
          {
            ret = io_backend_->Write(
              req.fpage_id_start, req.io_vec.data(), req.fd, &req.async_context.finish); // 不断尝试提交请求直至提交成功
          }
        }

        if (!req.async_context.finish)
        {
          io_backend_->Progress();
          req.async_context.state = context_type::State::Poll;
          return false;
        }
        else
        {
          req.async_context.state = context_type::State::End;
          return true;
        }
      }
      case context_type::State::Poll:
      {
        io_backend_->Progress();
        if (req.async_context.finish)
        {
          req.async_context.state = context_type::State::End;
          return true;
        }
        break;
      }
      case context_type::State::End:
      {
        return true;
      }
      }
      return false;
    }

    void Run()
    {
      size_t loops = 100;
      {
        boost::circular_buffer<std::optional<async_request_fiber_type*>>
          async_requests(gbp::FIBER_BATCH_SIZE);
        while (!async_requests.full())
          async_requests.push_back(std::nullopt);

        async_request_fiber_type* async_request;
        while (true)
        {
          for (auto& req : async_requests)
          {
            if (!req.has_value())
            {
              if (request_channel_.pop(async_request))
              {
                req.emplace(async_request);
              }
              else
              {
                continue;
              }
            }
            if (ProcessFunc(*req.value()))
            {
              req.value()->success.store(true);
              if (request_channel_.pop(async_request))
              {
                req.emplace(async_request);
              }
              else
                req.reset();
            }
          }
          if (stop_)
            break;
          // hybrid_spin(loops);
        }
      }
    }

    std::thread server_;
    boost::lockfree::queue<async_request_fiber_type*, boost::lockfree::capacity<FIBER_CHANNEL_DEPTH>> request_channel_;
    size_t num_async_fiber_processing_;
    bool stop_;
  };

  class IOServer {
  public:
    IOServer(gbp::DiskManager* disk_manager) :stop_(false) {
      if constexpr (gbp::IO_BACKEND_TYPE == 1)
        io_backend_ = new gbp::RWSysCall(disk_manager);
      else if (gbp::IO_BACKEND_TYPE == 2)
        io_backend_ = new gbp::IOURing(disk_manager);
      else
        assert(false);

      server_ = std::thread([this]()
        { Run(); });
    }

    ~IOServer() {
      stop_ = true;
      if (server_.joinable())
        server_.join();
    }

    bool SendRequest(async_request_fiber_type* req, bool blocked = true) {
      if (unlikely(req == nullptr))
        return false;

      if (likely(blocked))
        while (!async_channel_.push(req))
          ;
      else
      {
        return async_channel_.push(req);
      }

      return true;
    }


  private:

    bool process_func(async_request_fiber_type* req)
    {
      switch (req->async_context.state)
      {
      case context_type::State::Commit:
      { // 将read request提交至io_uring
        auto ret = io_backend_->Read(
          req->fpage_id_start, req->io_vec.data(), req->fd, &req->async_context.finish);
        while (!ret)
        {
          ret = io_backend_->Read(
            req->fpage_id_start, req->io_vec.data(), req->fd, &req->async_context.finish); // 不断尝试提交请求直至提交成功
        }

        if (!req->async_context.finish)
        {
          io_backend_->Progress();
          req->async_context.state = context_type::State::Poll;
          return false;
        }
        else
        {
          req->async_context.state = context_type::State::End;
          return true;
        }
      }
      case context_type::State::Poll:
      {
        io_backend_->Progress();
        if (req->async_context.finish)
        {
          req->async_context.state = context_type::State::End;
          return true;
        }
        break;
      }
      case context_type::State::End:
      {
        return true;
      }
      }
      return false;
    }

    void Run() {
      boost::circular_buffer<std::optional<async_request_fiber_type*>>
        async_requests(gbp::FIBER_BATCH_SIZE);
      async_request_fiber_type* async_request;
      while (!async_requests.full())
      {
        if (async_channel_.pop(async_request))
        {
          async_requests.push_back(async_request);
        }
      }

      while (true)
      {
        for (auto& req : async_requests)
        {
          if (!req.has_value())
          {
            if (async_channel_.pop(async_request))
              req.emplace(async_request);
            else
              continue;
          }
          if (process_func(req.value()))
          {
            req.value()->success.store(true);
            req.reset();
            // async_channel.pop(async_request);
            // req.emplace(async_request);
            // req.emplace(req_new);
          }
        }
        if (stop_)
          break;
      }
    }

    boost::lockfree::queue<async_request_fiber_type*, boost::lockfree::capacity<gbp::FIBER_CHANNEL_DEPTH>> async_channel_;
    bool stop_ = false;
    std::thread server_;
    gbp::IOBackend* io_backend_;
  };

} // namespace gbp