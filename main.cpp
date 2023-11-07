#include <iostream>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "buffer_pool_manager.h"

int main()
{
    // int ret;
    // std::cout << "Start of main" << std::endl;
    // // int data_file_ = open("/data/lgraph_db/test.file", O_RDWR | O_DIRECT | O_CREAT);
    // int data_file_ = open("./test.file", O_RDWR | O_DIRECT | O_CREAT | O_CLOEXEC);
    // size_t io_size_ = 1024LU * 4;
    // char *in_buf = (char *)malloc(io_size_);
    // ret = pread(data_file_, in_buf, io_size_, 0);
    // // pwrite(data_file_, out_buf, io_size_, curr_io_fileoffset);
    graphbuffer::page_id_t temp_page_id;

    graphbuffer::DiskManager *disk_manager = new graphbuffer::DiskManager("test.db");
    graphbuffer::BufferPoolManager bpm(10, disk_manager);
    temp_page_id = 0;
    auto page_zero = bpm.NewPage(temp_page_id);

    strcpy(page_zero->GetData(), "Hello");
    bpm.FlushPage(temp_page_id);
    page_zero = bpm.FetchPage(temp_page_id);
    std::cout << page_zero->GetData() << std::endl;
    return 0;
}