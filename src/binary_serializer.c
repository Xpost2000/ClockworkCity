enum binary_serializer_type {
    BINARY_SERIALIZER_FILE,
    BINARY_SERIALIZER_MEMORY,
};
enum binary_serializer_mode {
    BINARY_SERIALIZER_READ,
    BINARY_SERIALIZER_WRITE,
};
struct binary_serializer_memory_node {
    struct binary_serializer_memory_node* next;
    size_t size;
    uint8_t buffer[]; /*Probably GNU C only right? fuck*/
};

struct binary_serializer {
    enum binary_serializer_type type;
    enum binary_serializer_mode mode;

    union {
        /*maybe use this arena, don't know quite yet*/
        struct memory_arena arena;
        /*or use the file, not too picky on sources*/
        FILE* file_handle;

        struct {
            struct binary_serializer_memory_node* head;
            struct binary_serializer_memory_node* tail;
        } memory_nodes;

        struct {
            size_t size;
            uint8_t* buffer;

            size_t already_read;
        } memory_buffer;
    };
};

/* TODO(jerry): tomorrow or... The day after!! */
struct binary_serializer open_write_file_serializer(char* filename) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_WRITE;
    result.type = BINARY_SERIALIZER_FILE;

    result.file_handle = fopen(filename, "wb+");
    return result;
}

struct binary_serializer open_read_file_serializer(char* filename) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_READ;
    result.type = BINARY_SERIALIZER_FILE;

    result.file_handle = fopen(filename, "rb+");
    return result;
}

struct binary_serializer open_write_memory_serializer(void) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_WRITE;
    result.type = BINARY_SERIALIZER_MEMORY;

    return result;
}

struct binary_serializer open_read_memory_serializer(void* buffer, size_t buffer_size) {
    struct binary_serializer result = {};
    result.mode = BINARY_SERIALIZER_READ;
    result.type = BINARY_SERIALIZER_MEMORY;

    result.memory_buffer.size   = buffer_size;
    result.memory_buffer.buffer = buffer;

    return result;
}

void serializer_finish(struct binary_serializer* serializer) {
    switch (serializer->type) {
        case BINARY_SERIALIZER_FILE: {
            if (serializer->file_handle) {
                fclose(serializer->file_handle);
                serializer->file_handle = NULL;
            }
        } break;
        case BINARY_SERIALIZER_MEMORY: {
            if (serializer->mode == BINARY_SERIALIZER_WRITE) {
                /*free all memory*/ {
                    struct binary_serializer_memory_node* current_node = serializer->memory_nodes.head;

                    while (current_node) {
                        struct binary_serializer_memory_node* next = current_node->next;
                        system_deallocate_memory(current_node);
                        current_node = next;
                    }
                }
            }
        } break;
            invalid_cases();
    }
}

/* does not use an arena, but maybe it should? */
void* serializer_flatten_memory(struct binary_serializer* serializer, size_t* size) {
    assert(serializer->type == BINARY_SERIALIZER_MEMORY &&
           serializer->mode == BINARY_SERIALIZER_WRITE &&
           "Incompatible serializer type");
    size_t buffer_size = 0;
    /*count buffer size*/ {
        struct binary_serializer_memory_node* current_node = serializer->memory_nodes.head;

        while (current_node) {
            struct binary_serializer_memory_node* next = current_node->next;
            buffer_size += current_node->size;
            current_node = next;
        }
    }

    uint8_t* buffer = system_allocate_zeroed_memory(buffer_size);
    /*copy all data*/{
        struct binary_serializer_memory_node* current_node = serializer->memory_nodes.head;

        size_t written = 0;
        while (current_node) {
            struct binary_serializer_memory_node* next = current_node->next;
            memcpy(buffer + written, current_node->buffer, current_node->size);
            written += current_node->size;
            current_node = next;
        }
    }

    safe_assignment(size) = buffer_size;
    return buffer;
}

/* simple singly linked list */
void serializer_push_memory_node(struct binary_serializer* serializer, void* bytes, size_t size) {
    struct binary_serializer_memory_node* new_node = system_allocate_zeroed_memory(size + sizeof(*new_node));
    new_node->size = size;
    memcpy(new_node->buffer, bytes, size);

    if (!serializer->memory_nodes.head) {
        serializer->memory_nodes.head = new_node;
    }

    if (serializer->memory_nodes.tail) {
        serializer->memory_nodes.tail->next = new_node;
    }

    serializer->memory_nodes.tail = new_node;
}

void serialize_bytes(struct binary_serializer* serializer, void* bytes, size_t size) {
    switch (serializer->type) {
        case BINARY_SERIALIZER_FILE: {
            assert(serializer->file_handle && "File handle not opened on file serializer?");

            if (serializer->mode == BINARY_SERIALIZER_READ) {
                fread(bytes, size, 1, serializer->file_handle);
            } else {
                fwrite(bytes, size, 1, serializer->file_handle);
            }
        } break;
        case BINARY_SERIALIZER_MEMORY: {
            if (serializer->mode == BINARY_SERIALIZER_READ) {
                memcpy(bytes, serializer->memory_buffer.buffer + serializer->memory_buffer.already_read, size);
                serializer->memory_buffer.already_read += size;
            } else {
                serializer_push_memory_node(serializer, bytes, size);
            }
        } break;
            invalid_cases();
    }
}

#define Serialize_Object_Into_File(type)                                \
    case BINARY_SERIALIZER_FILE: {                                      \
        assert(serializer->file_handle && "File handle not opened on file serializer?"); \
        if (serializer->mode == BINARY_SERIALIZER_READ) {               \
            fread(obj, sizeof(type), 1, serializer->file_handle);       \
        } else {                                                        \
            fwrite(obj, sizeof(type), 1, serializer->file_handle);      \
        }                                                               \
    } break
#define Serialize_Object_Into_Memory_Buffer(type)                       \
    case BINARY_SERIALIZER_MEMORY: {                                    \
        if (serializer->mode == BINARY_SERIALIZER_READ) {               \
            memcpy(obj, serializer->memory_buffer.buffer + serializer->memory_buffer.already_read, sizeof(type)); \
            serializer->memory_buffer.already_read += sizeof(type);     \
        } else {                                                        \
            serializer_push_memory_node(serializer, obj, sizeof(type)); \
        }                                                               \
    } break
/* 
   Using some macro sauce, but not all the way since I'm not sure if different serializing methods
   will look different. 
   
   So these are all duplicated.
*/

void serialize_u64(struct binary_serializer* serializer, uint64_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint64_t);
        Serialize_Object_Into_Memory_Buffer(uint64_t);
        invalid_cases();
    }
}

void serialize_i64(struct binary_serializer* serializer, int64_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int64_t);
        Serialize_Object_Into_Memory_Buffer(int64_t);
        invalid_cases();
    }
}

void serialize_u32(struct binary_serializer* serializer, uint32_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint32_t);
        Serialize_Object_Into_Memory_Buffer(uint32_t);
        invalid_cases();
    }
}

void serialize_i32(struct binary_serializer* serializer, int32_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int32_t);
        Serialize_Object_Into_Memory_Buffer(int32_t);
        invalid_cases();
    }
}

void serialize_u16(struct binary_serializer* serializer, uint16_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint16_t);
        Serialize_Object_Into_Memory_Buffer(uint16_t);
        invalid_cases();
    }
}

void serialize_i16(struct binary_serializer* serializer, int16_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int16_t);
        Serialize_Object_Into_Memory_Buffer(int16_t);
        invalid_cases();
    }
}

void serialize_u8(struct binary_serializer* serializer, uint8_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint8_t);
        Serialize_Object_Into_Memory_Buffer(uint8_t);
        invalid_cases();
    }
}

void serialize_i8(struct binary_serializer* serializer, int8_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int8_t);
        Serialize_Object_Into_Memory_Buffer(int8_t);
        invalid_cases();
    }
}

void serialize_f64(struct binary_serializer* serializer, double* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(double);
        Serialize_Object_Into_Memory_Buffer(double);
        invalid_cases();
    }
}

void serialize_f32(struct binary_serializer* serializer, float* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(float);
        Serialize_Object_Into_Memory_Buffer(float);
        invalid_cases();
    }
}

/*
  helpful macros
*/

#define Serialize_Fixed_Array_And_Allocate_From_Arena_Top(serializer, arena, type, counter, array) \
    do {                                                                \
        serialize_##type(serializer, &counter);                         \
        if (serializer->mode == BINARY_SERIALIZER_READ)                 \
            array = memory_arena_push_top(arena, counter * sizeof(*array)); \
        serialize_bytes(serializer, array, counter * sizeof(*array));   \
    } while (0)
#define Serialize_Fixed_Array_And_Allocate_From_Arena(serializer, arena, type, counter, array) \
    do {                                                                \
        serialize_##type(serializer, &counter);                         \
        if (serializer->mode == BINARY_SERIALIZER_READ)                 \
            array = memory_arena_push(arena, counter * sizeof(*array)); \
        serialize_bytes(serializer, array, counter * sizeof(*array));   \
    } while (0)
#define Serialize_Fixed_Array(serializer, type, counter, array) \
    do {                                                                \
        serialize_##type(serializer, &counter);                         \
        serialize_bytes(serializer, array, counter * sizeof(*array));   \
    } while (0)
#define Serialize_Structure(serializer, structure) \
    do {                                                                \
        serialize_bytes(serializer, &structure, sizeof(structure));     \
    } while (0)


#undef Serialize_Object_Into_File
