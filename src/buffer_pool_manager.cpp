#include "../include/buffer_pool_manager.h"

#include <sys/mman.h>
#include <utility>

namespace gbp {

/*
 * BufferPoolManager Constructor
 * When log_manager is nullptr, logging is disabled (for test purpose)
 * WARNING: Do Not Edit This Function
 */
void BufferPoolManager::init(size_t pool_size, DiskManager* disk_manager) {
  pool_size_ = pool_size;
  disk_manager_ = disk_manager;

  // a consecutive memory space for buffer pool
  buffer_pool_ = aligned_alloc(PAGE_SIZE_OS, PAGE_SIZE * pool_size);
  madvise(buffer_pool_, pool_size * PAGE_SIZE, MADV_RANDOM);
  pages_ = new Page[pool_size_];
  // page_table_ = new std::vector<ExtendibleHash<page_id_infile, Page *>>();
  replacer_ = new LRUReplacer<Page*>;
  free_list_.reset(new VectorSync(pool_size_));

  for (auto file_handler : disk_manager_->file_handlers_) {
    page_tables_.push_back(
        std::make_shared<ExtendibleHash<page_id_infile, Page*>>(BUCKET_SIZE));
  }

  // put all the pages into free list
  for (size_t i = 0; i < pool_size_; ++i) {
    pages_[i].data_ = (char*) buffer_pool_ + (i * PAGE_SIZE);
    pages_[i].ResetMemory();
    free_list_->GetData()[i] = &(pages_[i]);
  }
  free_list_->size_ = pool_size_;
}

void BufferPoolManager::init(size_t pool_size) {
  DiskManager* disk_manager = new gbp::DiskManager("test.db");
  init(pool_size, disk_manager);
}

/*
 * BufferPoolManager Deconstructor
 * WARNING: Do Not Edit This Function
 */
BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  // delete page_table_;
  delete replacer_;
  free(buffer_pool_);
}

int BufferPoolManager::RegisterFile(int file_handler) {
  int file_handler_new = disk_manager_->RegisterFile(file_handler);
  page_tables_.push_back(
      std::make_shared<ExtendibleHash<page_id_infile, Page*>>(BUCKET_SIZE));
  return file_handler_new;
}

/**
 * 1. search hash table.
 *  1.1 if exist, pin the page and return immediately
 *  1.2 if no exist, find a replacement entry from either free list or lru
 *      replacer. (NOTE: always find from free list first)
 * 2. If the entry chosen for replacement is dirty, write it back to disk.
 * 3. Delete the entry for the old page from the hash table and insert an
 * entry for the new page.
 * 4. Update page metadata, read page content from disk file and return page
 * pointer
 *
 * This function must mark the Page as pinned and remove its entry from
 * LRUReplacer before it is returned to the caller.
 */
PageDescriptor BufferPoolManager::FetchPage(page_id_infile page_id,
                                            int file_handler) {
  // std::lock_guard<std::mutex> lck(latch_);
  Page* tar = nullptr;
  if (page_tables_[file_handler]->Find(page_id, tar)) {  // 1.1
    tar->pin_count_++;
    replacer_->Erase(tar);
    return tar;
  }

  // 1.2
  tar = GetVictimPage();
  if (tar == nullptr)
    return tar;

  // 2
  if (tar->is_dirty_) {
    disk_manager_->WritePage(tar->GetPageId(), tar->GetData(),
                             tar->GetFileHandler());
  }

  // 3
  page_tables_[file_handler]->Remove(tar->GetPageId());
  page_tables_[file_handler]->Insert(page_id, tar);

  // 4
  disk_manager_->ReadPage(page_id, tar->GetData(), file_handler);
  tar->pin_count_ = 1;
  tar->is_dirty_ = false;
  tar->page_id_ = page_id;
  tar->file_handler_ = file_handler;
  tar->buffer_pool_manager_ = this;

  return PageDescriptor(tar);
}

/*
 * Implementation of unpin page
 * if pin_count>0, decrement it and if it becomes zero, put it back to
 * replacer if pin_count<=0 before this call, return false. is_dirty: set the
 * dirty flag of this page
 */
bool BufferPoolManager::UnpinPage(page_id_infile page_id, bool is_dirty,
                                  int file_handler) {
  Page* tar = nullptr;
  page_tables_[file_handler]->Find(page_id, tar);
  if (tar == nullptr) {
    return false;
  }
  tar->is_dirty_ = is_dirty;
  if (tar->GetPinCount() <= 0) {
    return false;
  };
  if (--tar->pin_count_ == 0) {
    replacer_->Insert(tar);
  }
  return true;
}

/*
 * Implementation of unpin page
 * if pin_count>0, decrement it and if it becomes zero, put it back to
 * replacer if pin_count<=0 before this call, return false. is_dirty: set the
 * dirty flag of this page
 */
bool BufferPoolManager::ReleasePage(Page* tar) {
  // std::lock_guard<std::mutex> lck(latch_);
  if (tar->GetPinCount() <= 0) {
    return false;
  };
  if (--tar->pin_count_ == 0) {
    replacer_->Insert(tar);
  }
  return true;
}

/*
 * Used to flush a particular page of the buffer pool to disk. Should call the
 * write_page method of the disk manager
 * if page is not found in page table, return false
 * NOTE: make sure page_id != INVALID_PAGE_ID
 */
bool BufferPoolManager::FlushPage(page_id_infile page_id, int file_handler) {
  // std::lock_guard<std::mutex> lck(latch_);
  Page* tar = nullptr;
  page_tables_[file_handler]->Find(page_id, tar);
  if (tar == nullptr || tar->page_id_ == INVALID_PAGE_ID) {
    return false;
  }
  if (tar->is_dirty_) {
    disk_manager_->WritePage(page_id, tar->GetData(), tar->GetFileHandler());
    tar->is_dirty_ = false;
  }

  return true;
}

/**
 * User should call this method for deleting a page. This routine will call
 * disk manager to deallocate the page. First, if page is found within page
 * table, buffer pool manager should be reponsible for removing this entry out
 * of page table, reseting page metadata and adding back to free list. Second,
 * call disk manager's DeallocatePage() method to delete from disk file. If
 * the page is found within page table, but pin_count != 0, return false
 */
bool BufferPoolManager::DeletePage(page_id_infile page_id, int file_handler) {
  // std::lock_guard<std::mutex> lck(latch_);
  Page* tar = nullptr;
  page_tables_[file_handler]->Find(page_id, tar);
  if (tar != nullptr) {
    if (tar->GetPinCount() > 0) {
      return false;
    }
    replacer_->Erase(tar);
    page_tables_[file_handler]->Remove(page_id);
    tar->is_dirty_ = false;
    tar->ResetMemory();
    free_list_->InsertItem(tar);
  }
  disk_manager_->DeallocatePage(page_id);
  return true;
}

/**
 * User should call this method if needs to create a new page. This routine
 * will call disk manager to allocate a page.
 * Buffer pool manager should be responsible to choose a victim page either
 * from free list or lru replacer(NOTE: always choose from free list first),
 * update new page's metadata, zero out memory and add corresponding entry
 * into page table. return nullptr if all the pages in pool are pinned
 */
Page* BufferPoolManager::NewPage(page_id_infile& page_id, int file_handler) {
  // std::lock_guard<std::mutex> lck(latch_);
  Page* tar = nullptr;
  tar = GetVictimPage();
  if (tar == nullptr)
    return tar;

  page_id = disk_manager_->AllocatePage();

  // 2
  if (tar->is_dirty_) {
    disk_manager_->WritePage(tar->GetPageId(), tar->GetData(),
                             tar->GetFileHandler());
  }

  // 3
  page_tables_[file_handler]->Remove(tar->GetPageId());
  page_tables_[file_handler]->Insert(page_id, tar);

  // 4
  tar->page_id_ = page_id;
  tar->ResetMemory();
  tar->is_dirty_ = false;
  tar->pin_count_ = 1;
  tar->file_handler_ = file_handler;

  return tar;
}

Page* BufferPoolManager::GetVictimPage() {
  Page* tar = nullptr;
  tar = free_list_->GetItem();
  if (tar == nullptr) {
    if (replacer_->Size() == 0) {
      return nullptr;
    }
    replacer_->Victim(tar);
  }
  assert(tar->GetPinCount() == 0);
  return tar;
}

int BufferPoolManager::GetObject(char* buf, size_t file_offset,
                                 size_t object_size, int file_handler) {
  std::lock_guard<std::mutex> lck(latch_);

  size_t page_id = file_offset / PAGE_SIZE_BUFFER_POOL;
  size_t page_offset = file_offset % PAGE_SIZE_BUFFER_POOL;
  size_t object_size_t = 0;
  while (object_size > 0) {
    auto pd = FetchPage(page_id, file_handler);
    object_size_t = pd.GetPage()->GetObject(buf, page_offset, object_size);

    object_size -= object_size_t;
    buf += object_size_t;
    page_id++;
    page_offset = 0;
  }
  return 0;
}

int BufferPoolManager::SetObject(const char* buf, size_t file_offset,
                                 size_t object_size, int file_handler) {
  std::lock_guard<std::mutex> lck(latch_);

  size_t page_id = file_offset / PAGE_SIZE_BUFFER_POOL;
  size_t page_offset = file_offset % PAGE_SIZE_BUFFER_POOL;
  size_t object_size_t = 0;
  while (object_size > 0) {
    auto pd = FetchPage(page_id, file_handler);
    object_size_t = pd.GetPage()->SetObject(buf, page_offset, object_size);

    object_size -= object_size_t;
    buf += object_size_t;
    page_id++;
    page_offset = 0;
  }
  return 0;
}
}  // namespace gbp
