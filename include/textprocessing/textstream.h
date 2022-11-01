#ifndef TEXTSTREAM_H_INCLUDED
#define TEXTSTREAM_H_INCLUDED

#ifndef TEXTSTREAM_PUSHBACK_BUFFER_SIZE
#define TEXTSTREA,_PUSHBACK_BUFFER_SIZE 16
#endif

struct textstream_reader_data
{
    off_t text_start_offset;
    int wchar_buffer_used;
    wchar_t wchar_buffer;
};

union textstream_data_pointer
{
    FILE *file;
    const void *constant;
    void *normal;
    void **growable;
} textstream_data_pointer_t;

union textstream_data_init
{

    FILE *file;
    union
    {
        struct
        {
            const void *constant;

            void *normal;
            // The reason I'm doing this is to copy fmemstream and **open_memstream** which allows something like this
            void **growable;
        } ptr;
        union {
            size_t normal;
            size_t *growable;
        } capacity;
        // It will append after this index
        size_t used;
        // But it will read from this
        size_t initial_position;
    } mem;
} textstream_data_init_t;

typedef struct textstream
{
    union
    {
        FILE *ptr;
        struct
        {
            union
            {
                const void *constant;

                void *normal;
                // The reason I'm doing this is to copy fmemstream and **open_memstream** which allows something like this
                void **growable;
            } ptr;
            union {
                size_t normal;
                size_t *growable;
            } capacity;
            size_t index;
            size_t used;
        } mem;
    } file;

    struct {
        size_t size;
        char buffer[TEXTREADER_PUSHBACK_BUFFER_SIZE];
    } ungetc_stack;
    size_t total_ungetc_count;

    union
    {
        struct textstream_reader_data reader;
    } mode_data;

    textstream_flags_t flags;
    textprocessing_encoding_t encoding;

    textstream_reader_data reader;
} textstream_t;

typedef enum textstream_flags
{
    TSFLG_ISMEM = 0x1,
    TSFLG_EOF = 0x2,
    TSFLG_ERROR = 0x4,
    TSFLG_READ_USING_FGETWC = 0x8,
    TSFLG_IS_INIT_FILEPTR = 0x10,

    // If the memory is allocated by the textstream instead of the user. If this is true it will deallocate on exit
    TSFLG_IS_MEM_ALLOC = 0x20,

    // If the memory pointer is allowed to the grown
    TSFLG_MEM_ALLOW_GROWTH = 0x40,

    TSFLG_WRITE = 0x80,
    TSFLG_APPEND = 0x80 | 0x100,
    TSFLG_READ = 0x200
} textstream_flags_t;

typedef enum textstream_mode
{
    // write and append not compatible with each other
    TSMODE_APPEND = 0x1,
    TSMODE_WRITE = 0x2,

    TSMODE_READ = 0x4,

    TSMODE_MEM = 0x8,
    TSMODE_FILE = 0x10
    TSMODE_FILEPTR = 0x10 | 0x20
} textstream_mode_t;

int textstream_init()

// yea big param count
int textstream_reopen(textstream_t stream, textstream_mode_t mode, textstream_data_init_t new_data, int close_old_data);

int textstream_close(textstream_t stream, int close_data);

#endif // TEXTSTREAM_H_INCLUDED
