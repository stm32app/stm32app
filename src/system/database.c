#include "database.h"
#include "core/buffer.h"
#include "core/file.h"

static int registerFunctions(sqlite3 *db, const char **pzErrMsg, const struct sqlite3_api_routines *pThunk) {
    return SQLITE_OK;
}

 int sqlite3_os_init(void) {
    return 1;
 };
 int sqlite3_os_end(void) {
     return 0;
 };
/*
** The maximum pathname length supported by this VFS.
*/
#define MAXPATHNAME 100

/*
** When using this VFS, the sqlite3_file* handles that SQLite uses are
** actually pointers to instances of type vfs_File.
*/
typedef struct vfs_File vfs_File;
struct vfs_File {
    sqlite3_file base; /* Base class. Must be first. */
    app_file_t file;
    app_buffer_t *cache;
    sqlite3_int64 cache_offset; /* Offset in file of datafer[0] */
};

/*
** Write directly to the file passed as the first argument. Even if the
** file has a write-buffer (vfs_File.cache->data), ignore it.
*/
static int vfs_DirectWrite(vfs_File *p,          /* File handle */
                           const void *data,     /* Buffer containing data to write */
                           int size,             /* Size of data to write in bytes */
                           sqlite_int64 position /* File offset to write to */
) {
    app_file_seek_to(&p->file, (uint32_t)position);
    app_file_write(&p->file, data, size);
    trace_printf("fn:DirectWrite:Success");
    return SQLITE_OK;
}

/*
** Flush the contents of the vfs_File.cache->data buffer to disk. This is a
** no-op if this particular file does not have a buffer (i.e. it is not
** a journal file) or if the buffer is currently empty.
*/
static int vfs_FlushBuffer(vfs_File *p) {
    int rc = SQLITE_OK;
    trace_printf("fn: FlushBuffer");
    if (p->cache) {
        rc = vfs_DirectWrite(p, p->cache->data, p->cache->size, p->cache_offset);
        p->cache->size = 0;
    }
    trace_printf("fn:FlushBuffer:Successs");
    return rc;
}

/*
** Close a file.
*/
static int vfs_Close(sqlite3_file *pFile) {
    int rc;
    vfs_File *p = (vfs_File *)pFile;
    trace_printf("fn: Close");
    rc = vfs_FlushBuffer(p);
    app_file_close(&p->file);
    trace_printf("fn:Close:Success");
    return rc;
}

/*
** Read data from a file.
*/
static int vfs_Read(sqlite3_file *pFile, void *data, int size, sqlite_int64 offset) {
    trace_printf("fn: Read");
    vfs_File *p = (vfs_File *)pFile;
    app_file_seek_to(&p->file, (uint32_t)offset);
    app_file_read(&p->file, data, size);

    // Todo: Short read
    // if (nRead == size) {
    trace_printf("fn:Read:Success");
    return SQLITE_OK;
    //} else if (nRead >= 0) {
    //    return SQLITE_IOERR_SHORT_READ;
    //}

    // return SQLITE_IOERR_READ;
}

/*
** Write data to a crash-file.
*/
static int vfs_Write(sqlite3_file *pFile, const void *data, int size, sqlite_int64 offset) {
    trace_printf("fn: Write");
    vfs_File *p = (vfs_File *)pFile;

    if (p->cache->data) {
        char *z = (char *)data;   /* Pointer to remaining data to write */
        int n = size;             /* Number of bytes at z */
        sqlite3_int64 i = offset; /* File offset to write to */

        while (n > 0) {
            int nCopy; /* Number of bytes to copy into buffer */

            /* If the buffer is full, or if this data is not being written directly
            ** following the data already buffered, flush the buffer. Flushing
            ** the buffer is a no-op if it is empty.
            */
            if (p->cache->size == p->cache->allocated_size || p->cache_offset + p->cache->size != i) {
                int rc = vfs_FlushBuffer(p);
                if (rc != SQLITE_OK) {
                    return rc;
                }
            }
            configASSERT(p->cache->size == 0 || p->cache_offset + p->cache->size == i);
            p->cache_offset = i - p->cache->size;

            /* Copy as much data as possible into the buffer. */
            nCopy = p->cache->allocated_size - p->cache->size;
            if (nCopy > n) {
                nCopy = n;
            }
            memcpy(&p->cache->data[p->cache->size], z, nCopy);
            p->cache->size += nCopy;

            n -= nCopy;
            i += nCopy;
            z += nCopy;
        }
        return SQLITE_OK;
    } else {
        return vfs_DirectWrite(p, data, size, offset);
    }
}

/* Truncate a file. This is a no-op for this VFS As of version 3.6.24, SQLite may run without a working xTruncate() call, providing the user
   does not configure SQLite to use "journal_mode=truncate", or use both "journal_mode=persist" and ATTACHed databases. */
static int vfs_Truncate(sqlite3_file *pFile, sqlite_int64 size) {
    vfs_File *p = (vfs_File *)pFile;
    trace_printf("fn: Truncate");
    app_file_truncate(&p->file);
    trace_printf("fn:Truncate:Success");
    return SQLITE_OK;
}

/*
** Sync the contents of the file to the persistent media.
*/
static int vfs_Sync(sqlite3_file *pFile, int flags) {
    vfs_File *p = (vfs_File *)pFile;
    trace_printf("fn: Sync");
    int rc = vfs_FlushBuffer(p);
    if (rc != SQLITE_OK) {
        return rc;
    }
    app_file_sync(&p->file);
    return SQLITE_OK;
}

/*
** Write the size of the file in bytes to *pSize.
*/
static int vfs_FileSize(sqlite3_file *pFile, sqlite_int64 *pSize) {
    vfs_File *p = (vfs_File *)pFile;
    int rc; /* Return code from fstat() call */

    rc = vfs_FlushBuffer(p);
    if (rc != SQLITE_OK) {
        return rc;
    }
    app_file_stat(&p->file);
    *pSize = p->file.size;

    return SQLITE_OK;
}

/*
** Locking functions. The xLock() and xUnlock() methods are both no-ops.
** The xCheckReservedLock() always indicates that no other process holds
** a reserved lock on the database file. This ensures that if a hot-journal
** file is found in the file-system it is rolled back.
*/
static int vfs_Lock(sqlite3_file *pFile, int eLock) {
    return SQLITE_OK;
}
static int vfs_Unlock(sqlite3_file *pFile, int eLock) {
    return SQLITE_OK;
}
static int vfs_CheckReservedLock(sqlite3_file *pFile, int *pResOut) {
    *pResOut = 0;
    return SQLITE_OK;
}

/*
** No xFileControl() verbs are implemented by this VFS.
*/
static int vfs_FileControl(sqlite3_file *pFile, int op, void *pArg) {
    return SQLITE_OK;
}

/*
** The xSectorSize() and xDeviceCharacteristics() methods. These two
** may return special values allowing SQLite to optimize file-system
** access to some extent. But it is also safe to simply return 0.
*/
static int vfs_SectorSize(sqlite3_file *pFile) {
    return 0;
}
static int vfs_DeviceCharacteristics(sqlite3_file *pFile) {
    return 0;
}

/*
** Query the file-system to see if the named file exists, is readable or
** is both readable and writable.
*/
static int vfs_Access(sqlite3_vfs *pVfs, const char *zPath, int flags, int *pResOut) {
    trace_printf("fn: Access %s", zPath);
    app_file_t file = {};
    app_file_open(&file, zPath, APP_FILE_EXCLUSIVE);
    *pResOut = ((file.flags & APP_FILE_ERROR) ? 0 : 1);
    trace_printf("fn:Access:Success");
    return SQLITE_OK;
}

/*
** Open a file handle.
*/
static int vfs_Open(sqlite3_vfs *pVfs,   /* VFS */
                    const char *zName,   /* File to open, or 0 for a temp file */
                    sqlite3_file *pFile, /* Pointer to vfs_File struct to populate */
                    int flags,           /* Input SQLITE_OPEN_XXX flags */
                    int *pOutFlags       /* Output SQLITE_OPEN_XXX flags (or NULL) */
) {
    trace_printf("fn: Open");
    system_database_t *database = pVfs->pAppData;
    vfs_File *p = (vfs_File *)pFile; /* Populate this structure */
    memset(p, 0, sizeof(vfs_File));

    if (zName == 0) { // temp files are not supported by this vfs yet
        return SQLITE_IOERR;
    }

    p->file.owner = database->actor;
    p->file.storage = database->storage->actor;

    if (flags & SQLITE_OPEN_MAIN_JOURNAL) {
        p->cache = database->journal_buffer;
    }

    if (flags & SQLITE_OPEN_READONLY || flags & SQLITE_OPEN_READWRITE) {
        p->file.flags |= APP_FILE_READ;
    }
    if (flags & SQLITE_OPEN_CREATE || flags & SQLITE_OPEN_READWRITE) {
        p->file.flags |= APP_FILE_WRITE;
    }
    if (flags & SQLITE_OPEN_CREATE) {
        p->file.flags |= APP_FILE_CREATE;
    }

    app_file_open(&p->file, zName, p->file.flags);

    if (pOutFlags) {
        *pOutFlags = (int)flags;
    }
    p->base.pMethods = &database->vfs_io;

    trace_printf("fn:Open:Success");
    return SQLITE_OK; // SQLITE_IOERR_DELETE
}

/*
** Delete the file identified by argument zPath. If the dirSync parameter
** is non-zero, then ensure the file-system modification to delete the
** file has been synced to disk before returning.
*/
static int vfs_Delete(sqlite3_vfs *pVfs, const char *zPath, int dirSync) {
    trace_printf("fn: Delete");
    system_database_t *database = pVfs->pAppData;
    app_file_t file = {.owner = database->actor, .storage = database->storage->actor, .path = zPath};
    app_file_delete(&file);
    trace_printf("fn:Delete:Success");
    return SQLITE_OK; // SQLITE_IOERR_DELETE
}

/*
** Argument zPath points to a nul-terminated string containing a file path.
** If zPath is an absolute path, then it is copied as is into the output
** buffer. Otherwise, if it is a relative path, then the equivalent full
** path is written to the output buffer.
**
** This function assumes that paths are UNIX style. Specifically, that:
**
**   1. Path components are separated by a '/'. and
**   2. Full paths begin with a '/' character.
*/
static int vfs_FullPathname(sqlite3_vfs *pVfs, /* VFS */
                            const char *zPath, /* Input path (possibly a relative path) */
                            int nPathOut,      /* Size of output buffer in bytes */
                            char *zPathOut     /* Pointer to output buffer */
) {
    strncpy(zPathOut, zPath, nPathOut);
    zPathOut[nPathOut - 1] = '\0';
    trace_printf("fn:Fullpathname:Success");

    return SQLITE_OK;
}

/*
** The following four VFS methods:
**
**   xDlOpen
**   xDlError
**   xDlSym
**   xDlClose
**
** are supposed to implement the functionality needed by SQLite to load
** extensions compiled as shared objects. This simple VFS does not support
** this functionality, so the following functions are no-ops.
*/
static void *vfs_DlOpen(sqlite3_vfs *pVfs, const char *zPath) {
    return 0;
}
static void vfs_DlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg) {
    sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are not supported");
    zErrMsg[nByte - 1] = '\0';
}
static void (*vfs_DlSym(sqlite3_vfs *pVfs, void *pH, const char *z))(void) {
    return 0;
}
static void vfs_DlClose(sqlite3_vfs *pVfs, void *pHandle) {
    return;
}

/*
** Parameter zByte points to a buffer nByte bytes in size. Populate this
** buffer with pseudo-random data.
*/
static int vfs_Randomness(sqlite3_vfs *pVfs, int nByte, char *zByte) {
    return SQLITE_OK;
}

/*
** Sleep for at least nMicro microseconds. Return the (approximate) number
** of microseconds slept for.
*/
static int vfs_Sleep(sqlite3_vfs *pVfs, int nMicro) {
    // sleep(nMicro / 1000000);
    // usleep(nMicro % 1000000);
    return nMicro;
}

/*
** Set *pTime to the current UTC time expressed as a Julian day. Return
** SQLITE_OK if successful, or an error code otherwise.
**
**   http://en.wikipedia.org/wiki/Julian_day
**
** This implementation is not very good. The current time is rounded to
** an integer number of seconds. Also, assuming time_t is a signed 32-bit
** value, it will stop working some time in the year 2038 AD (the so-called
** "year 2038" problem that afflicts systems that store time this way).
*/
static int vfs_CurrentTime(sqlite3_vfs *pVfs, double *pTime) {
    // time_t t = time(0);
    //*pTime = t / 86400.0 + 2440587.5;
    *pTime = 2440587;
    return SQLITE_OK;
}

static int vfs_GetLastError(sqlite3_vfs *pVfs, int nBuf, char *zBuf) {
    configASSERT(zBuf[0] == '\0');
    return 0;
}

static ODR_t database_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    system_database_t *database = stream->object;
    (void)database;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t database_validate(system_database_properties_t *properties) {
    return 0;
}

static app_signal_t database_construct(system_database_t *database) {
    app_thread_allocate(&database->thread, database, (void (*)(void *ptr))app_thread_execute, "DB", 1024, 0, 4, NULL);
    database->journal_buffer = app_buffer_target(database->actor, NULL, database->properties->journal_buffer_size);
    if (database->journal_buffer == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    }

    database->vfs = (sqlite3_vfs){
        1,                // iVersion
        sizeof(vfs_File), // szOsFile
        MAXPATHNAME,      // mxPathname
        0,                // pNext
        "vfs_",           // zName
        database,         // pAppData
        vfs_Open,         // xOpen
        vfs_Delete,       // xDelete
        vfs_Access,       // xAccess
        vfs_FullPathname, // xFullPathname
        vfs_DlOpen,       // xDlOpen
        vfs_DlError,      // xDlError
        vfs_DlSym,        // xDlSym
        vfs_DlClose,      // xDlClose
        vfs_Randomness,   // xRandomness
        vfs_Sleep,        // xSleep
        vfs_CurrentTime,  // xCurrentTime
        vfs_GetLastError, // xGetLastError
        NULL,             // xCurrentTimeInt64
        NULL,             // xSetSystemCall
        NULL,             // xGetSystemCall
        NULL,             // xNextSystemCall
    };
    database->vfs_io = (sqlite3_io_methods){
        1,         /* iVersion */
        vfs_Close, /* xClose */
        vfs_Read,  /* xRead */
        vfs_Write, /* xWrite */

        vfs_Truncate,              /* xTruncate */
        vfs_Sync,                  /* xSync */
        vfs_FileSize,              /* xFileSize */
        vfs_Lock,                  /* xLock */
        vfs_Unlock,                /* xUnlock */
        vfs_CheckReservedLock,     /* xCheckReservedLock */
        vfs_FileControl,           /* xFileControl */
        vfs_SectorSize,            /* xSectorSize */
        vfs_DeviceCharacteristics, /* xDeviceCharacteristics */
        0,                         /* xShmMap */
        0,                         /* xShmLock */
        0,                         /* xShmBarrier */
        0,                         /* xShmUnmap */
        NULL,                      /* xFetch */
        NULL                       /* xUnfetch */
    };

    sqlite3_vfs_register(&database->vfs, 1);
    //sqlite3_auto_extension((void (*)())registerFunctions);
    return 0;
}

static app_job_signal_t database_job_query(app_job_t *job) {
    return 0;
}

static app_job_signal_t database_job_connect(app_job_t *job) {
    system_database_t *database = job->actor->object;
    sqlite3_open(database->properties->path, &database->connection);
    return APP_JOB_HALT;
}

static app_signal_t database_start(system_database_t *database) {
    sqlite3_initialize();
    return 0;
}

static app_signal_t database_stop(system_database_t *database) {
    sqlite3_shutdown();
    return 0;
}

static app_signal_t database_link(system_database_t *database) {
    return 0;
}

static app_signal_t database_on_phase(system_database_t *database, actor_phase_t phase) {
    return 0;
}

static app_signal_t database_on_signal(system_database_t *database, actor_t *actor, app_signal_t signal, void *source) {
    return 0;
}

static app_signal_t database_medium_priority(system_database_t *database, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    return app_job_execute_if_running_in_thread(&database->job, thread);
}
static app_signal_t database_on_input(system_database_t *database, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_START:
        debug_log_inhibited = false;
        return actor_event_handle_and_start_job(database->actor, event, &database->job, database->thread, database_job_connect);
    case APP_EVENT_QUERY:
        return actor_event_handle_and_start_job(database->actor, event, &database->job, database->thread, database_job_query);
    default:
        break;
    }
    return 0;
}

actor_class_t system_database_class = {
    .type = SYSTEM_DATABASE,
    .size = sizeof(system_database_t),
    .phase_subindex = SYSTEM_DATABASE_PHASE,
    .validate = (app_method_t)database_validate,
    .construct = (app_method_t)database_construct,
    .link = (app_method_t)database_link,
    .start = (app_method_t)database_start,
    .stop = (app_method_t)database_stop,
    .on_phase = (actor_on_phase_t)database_on_phase,
    .on_signal = (actor_on_signal_t)database_on_signal,
    .worker_input = (actor_on_worker_t)database_on_input,
    .worker_medium_priority = (actor_on_worker_t)database_medium_priority,
    .property_write = database_property_write,
};
