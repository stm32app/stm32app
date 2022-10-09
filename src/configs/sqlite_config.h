#ifndef CONFIG_SQLITE_H
#define CONFIG_SQLITE_H

#define SQLITE_OS_UNIX 0
#define SQLITE_OS_WIN 0
#define SQLITE_OMIT_EXPLAIN 1
#define SQLITE_OMIT_DEPRECATED 1
#define SQLITE_OMIT_COMPILEOPTION_DIAGS 1
#define SQLITE_OMIT_AUTOINIT 1
#define SQLITE_OMIT_TRACE 1
#define SQLITE_OMIT_DESERIALIZE 1
#define SQLITE_OMIT_WAL                      1
#define SQLITE_OMIT_WAL                      1
#define SQLITE_OMIT_SHARED_CACHE 1
#define SQLITE_OMIT_LOOKASIDE 1
#define SQLITE_OMIT_MEMORYDB 1
#define SQLITE_OMIT_DECLTYPE 1
#define SQLITE_OMIT_TCL_VARIABLE 1
#define SQLITE_OMIT_UTF16 1

#define BUILD_sqlite -DNDEBUG
#define SQLITE_OMIT_LOAD_EXTENSION           1
#define SQLITE_DQS                           0
#define SQLITE_OS_OTHER                      1
#define SQLITE_NO_SYNC                       1
#define SQLITE_TEMP_STORE                    1
#define SQLITE_DISABLE_LFS                   1
#define SQLITE_DISABLE_DIRSYNC               1
#define SQLITE_SECURE_DELETE                 0
#define SQLITE_DEFAULT_LOOKASIDE        512,64
#define YYSTACKDEPTH                        20
#define SQLITE_SMALL_STACK                   1
#define SQLITE_DEFAULT_PAGE_SIZE          4096
#define SQLITE_SORTER_PMASZ                  4
#define SQLITE_DEFAULT_CACHE_SIZE           -1
#define SQLITE_DEFAULT_MEMSTATUS             0
#define SQLITE_DEFAULT_MMAP_SIZE             0
#define SQLITE_CORE                          1
#define SQLITE_SYSTEM_MALLOC                 1
#define SQLITE_THREADSAFE                    0
#define SQLITE_MUTEX_APPDEF                  1
#define SQLITE_DISABLE_FTS3_UNICODE          1
#define SQLITE_DISABLE_FTS4_DEFERRED         1
#define SQLITE_LIKE_DOESNT_MATCH_BLOBS       1
#define SQLITE_DEFAULT_FOREIGN_KEYS          0
#define SQLITE_DEFAULT_LOCKING_MODE          1
#define SQLITE_DEFAULT_PAGE_SIZE          4096
#define SQLITE_DEFAULT_PCACHE_INITSZ         8
#define SQLITE_MAX_DEFAULT_PAGE_SIZE     32768
#define SQLITE_POWERSAFE_OVERWRITE           1
#define SQLITE_MAX_EXPR_DEPTH                0

#endif