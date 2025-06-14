#include "../include/buffer_pool_manager.h"

#include "cgraph/vertex.h"

namespace test {

constexpr static size_t MMAP_ADVICE = MADV_RANDOM;
constexpr static size_t exp_num = 1024LU * 1024 * 1024LU;

static bool log_thread_run = true;
static std::mutex latch;

int test_concurrency(int argc, char** argv);

void fiber_pread_0(gbp::DiskManager* disk_manager, size_t file_size_inByte,
                   size_t io_size, size_t thread_id);

void fiber_pread_1(gbp::DiskManager* disk_manager, size_t file_size_inByte,
                   size_t io_size, size_t thread_id);
void fiber_pread_1_1(gbp::DiskManager* disk_manager, size_t file_size_inByte,
                     size_t io_size, size_t thread_id);
void fiber_pread_1_2(gbp::DiskManager* disk_manager, size_t file_size_inByte,
                     size_t io_size, size_t thread_id);

void fiber_pread_2(gbp::DiskManager* disk_manager, size_t file_size_inByte,
                   size_t io_size, size_t thread_id);

void fiber_pread_3(gbp::IOServer* io_server, size_t file_size_inByte,
                   size_t io_size, size_t thread_id);

void fiber_pread_4(gbp::IOServer* io_server, size_t file_size_inByte,
                   size_t io_size, size_t thread_id);

void fiber_pread(gbp::DiskManager* disk_manager, size_t file_size_inByte,
                 size_t io_size, size_t thread_id);
struct Page_mine {
  size_t data[512];
};

int test_graph(int argc, char** argv);

void test_csv(const std::string& file_path);
void test_vertex(const std::string& config_file_path,
                 const std::string& data_file_path,
                 const std::string& db_dir_path);
}  // namespace test
