#include "../glad/glad.h"

/*
 * to make my opengl life not as painful
 */

struct opengl_texture_parameters {
    GLenum filter_min;
    GLenum filter_mag;
    GLenum wrap_s;
    GLenum wrap_t;
};

GLuint opengl_create_texture(size_t width, size_t height, GLint internal_format,
                             GLenum format, GLenum datatype, uint8_t* pixels,
                             struct opengl_texture_parameters params) {
    if (params.filter_min == 0) {
        params.filter_min = GL_NEAREST;
    }
    if (params.filter_mag == 0) {
        params.filter_mag = GL_NEAREST;
    }
    if (params.wrap_s == 0) {
        params.wrap_s = GL_REPEAT;
    }
    if (params.wrap_t == 0) {
        params.wrap_t = GL_REPEAT;
    }

    GLuint object;
    glGenTextures(1, &object);

    glBindTexture(GL_TEXTURE_2D, object); {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.filter_min);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.filter_mag);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrap_s);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrap_t);

        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, datatype, pixels);

        glGenerateMipmap(GL_TEXTURE_2D);
    } glBindTexture(GL_TEXTURE_2D, 0);

    return object;
}

/* no error checking */
GLuint opengl_load_shader_program(char* vertex_source, char* fragment_source) {
    GLuint vertex_shader   = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint program = glCreateProgram();

    glShaderSource(vertex_shader, 1, &vertex_source, 0);
    glShaderSource(vertex_shader, 1, &fragment_source, 0);

    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    /* some sanity check */
    {
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        assert(status == GL_TRUE && "Whoops shader failed to link! Something is wrong!");
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

struct opengl_vertex_attribute {
    char*  name;                /*debug reasons*/
    size_t element_count;
    size_t offset;
    bool   normalized;
    GLenum type;
};
struct opengl_vertex_format {
    struct opengl_vertex_attribute attributes[16];
};
struct opengl_vertex_buffer {
    struct opengl_vertex_format format;
    size_t count;
    size_t vertex_size;
    GLuint vertex_array;
    GLuint vertex_buffer;
    /* no index buffer. Too much additional work, I'll just duplicate vertices */
};

void opengl_deallocate_vertex_buffer(struct opengl_vertex_buffer* buffer) {
    glDeleteVertexArrays(1, &buffer->vertex_array);
    glDeleteBuffers(1, &buffer->vertex_buffer);
}

struct opengl_vertex_buffer opengl_allocate_vertex_buffer(struct opengl_vertex_format format, void* data, size_t vertex_size, size_t count, GLenum usage) {
    GLuint vertex_buffer_object;
    GLuint vertex_array_object;

    glGenBuffers(1, &vertex_buffer_object);
    glGenVertexArrays(1, &vertex_array_object);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object); {
        glBufferData(GL_ARRAY_BUFFER, vertex_size * count, data, usage);
    }

    glBindVertexArray(vertex_array_object);
    {
        for (size_t index = 0; index < array_count(format.attributes); ++index) {
            struct opengl_vertex_attribute* attribute = format.attributes + index;

            bool empty_attribute = true;
            {
                if (attribute->element_count > 0) empty_attribute = false;
                if (attribute->name) empty_attribute = false;
            }

            if (empty_attribute) continue;

            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, attribute->element_count, attribute->type,
                                  attribute->normalized, vertex_size, (void*)(attribute->offset));
        }
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return (struct opengl_vertex_buffer) {
        .vertex_size = vertex_size,
        .count = count,
        .format = format,
        .vertex_array = vertex_array_object,
        .vertex_buffer = vertex_buffer_object,
    };
}

void opengl_draw_vertex_buffer(struct opengl_vertex_buffer* buffer, GLuint texture_id, GLuint shader_program, size_t offset_start, size_t count) {
    glUseProgram(shader_program);

    /* only single textures for now */
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glBindVertexArray(buffer->vertex_array);
    glDrawArrays(GL_TRIANGLES, offset_start, count);
}
