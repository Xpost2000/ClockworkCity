enum binary_serializer_mode {
    BINARY_SERIALIZER_READ,
    BINARY_SERIALIZER_WRITE,
};

struct binary_serializer {
    /*maybe use this arena, don't know quite yet*/
    struct memory_arena arena;
    /*or use the file, not too picky on sources*/
    FILE* file_handle;

    enum binary_serializer_mode mode;
};

/* TODO(jerry): tomorrow or... The day after!! */
