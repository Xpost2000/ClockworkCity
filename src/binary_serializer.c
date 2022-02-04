enum binary_serializer_type {
    BINARY_SERIALIZER_FILE,
    BINARY_SERIALIZER_MEMORY_BUFFER,
};
enum binary_serializer_mode {
    BINARY_SERIALIZER_READ,
    BINARY_SERIALIZER_WRITE,
};

struct binary_serializer {
    enum binary_serializer_type type;
    enum binary_serializer_mode mode;

    union {
        /*maybe use this arena, don't know quite yet*/
        struct memory_arena arena;
        /*or use the file, not too picky on sources*/
        FILE* file_handle;
    };
};

/* TODO(jerry): tomorrow or... The day after!! */
struct binary_serializer open_write_file_serializer(char* filename) {
    struct binary_serializer result;
    result.mode = BINARY_SERIALIZER_READ;
    result.type = BINARY_SERIALIZER_FILE;

    result.file_handle = fopen(filename, "wb+");
    return result;
}

struct binary_serializer open_read_file_serializer(char* filename) {
    struct binary_serializer result;
    result.mode = BINARY_SERIALIZER_READ;
    result.type = BINARY_SERIALIZER_FILE;

    result.file_handle = fopen(filename, "rb+");
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
            invalid_cases();
    }
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

/* 
   Using some macro sauce, but not all the way since I'm not sure if different serializing methods
   will look different. 
   
   So these are all duplicated.
*/

void serialize_u64(struct binary_serializer* serializer, uint64_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint64_t);
        invalid_cases();
    }
}

void serialize_i64(struct binary_serializer* serializer, int64_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int64_t);
        invalid_cases();
    }
}

void serialize_u32(struct binary_serializer* serializer, uint32_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint32_t);
        invalid_cases();
    }
}

void serialize_i32(struct binary_serializer* serializer, int32_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int32_t);
        invalid_cases();
    }
}

void serialize_u16(struct binary_serializer* serializer, uint16_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint16_t);
        invalid_cases();
    }
}

void serialize_i16(struct binary_serializer* serializer, int16_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int16_t);
        invalid_cases();
    }
}

void serialize_u8(struct binary_serializer* serializer, uint8_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(uint8_t);
        invalid_cases();
    }
}

void serialize_i8(struct binary_serializer* serializer, int8_t* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(int8_t);
        invalid_cases();
    }
}

void serialize_f64(struct binary_serializer* serializer, double* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(double);
        invalid_cases();
    }
}

void serialize_f32(struct binary_serializer* serializer, float* obj) {
    switch (serializer->type) {
        Serialize_Object_Into_File(float);
        invalid_cases();
    }
}

#undef Serialize_Object_Into_File
