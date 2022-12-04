// graphics.cpp

#include "graphics.hpp"
#include "system.hpp"

#include <cassert>
#include <glad/glad.h>
#pragma warning(push)
#pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
#include <glm/gtc/type_ptr.hpp>
#pragma warning(pop)
#include <stb_image.h>

static void 
opengl_check_errors_(const char *file, const int line)
{
   GLenum error = glGetError();
   if (error != GL_NO_ERROR) {
      debug::error("0x%X (%d) in %s (%d)", error, error, file, line);
   }
   assert(error == GL_NO_ERROR);
}

#if _DEBUG
#define opengl_check_errors() opengl_check_errors_(__FILE__,__LINE__)
#else
#define opengl_check_errors() 
#endif

static uint32_t 
fnv1a32(const void *data, const size_t size) {
   const uint8_t *at = (uint8_t *)data;

   uint32_t result = 2166136261;
   for (size_t index = 0; index < size; index++) {
      result ^= uint32_t(at[index]);
      result *= 16777619;
   }

   return result;
}

bool shader_program_t::valid() const
{
   return m_id != 0;
}

static const char *
gl_uniform_type_string(const GLenum type)
{
   switch (type) {
   case GL_FLOAT_VEC2: return "vec2";
   case GL_FLOAT_VEC3: return "vec3";
   case GL_FLOAT_VEC4: return "vec4";
   case GL_FLOAT_MAT4: return "mat4";
   }
   return "unknown";
}

bool shader_program_t::create(const std::string_view &vertex_source,
                              const std::string_view &fragment_source)
{
   const char *glsl_vertex_source = vertex_source.data();
   const GLint glsl_vertex_length = GLint(vertex_source.length());
   GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertex_shader_id, 1, &glsl_vertex_source, &glsl_vertex_length);
   glCompileShader(vertex_shader_id);

   const char *glsl_fragment_source = fragment_source.data();
   const GLint glsl_fragment_length = GLint(fragment_source.length());
   GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(fragment_shader_id, 1, &glsl_fragment_source, &glsl_fragment_length);
   glCompileShader(fragment_shader_id);

   GLuint shader_program_id = glCreateProgram();
   glAttachShader(shader_program_id, vertex_shader_id);
   glAttachShader(shader_program_id, fragment_shader_id);
   glLinkProgram(shader_program_id);

   // note: we don't need the vertex and fragment shaders anymore, 
   //       they are (hopefully) linked into a shader program
   glDetachShader(shader_program_id, vertex_shader_id);
   glDetachShader(shader_program_id, fragment_shader_id);
   glDeleteShader(vertex_shader_id);
   glDeleteShader(fragment_shader_id);

   // note: verify shader program linkage status
   GLint link_status = GL_TRUE;
   glGetProgramiv(shader_program_id, GL_LINK_STATUS, &link_status);
   if (link_status == GL_FALSE) {
      GLchar error_message[1024];
      glGetProgramInfoLog(shader_program_id, sizeof(error_message), nullptr, error_message);
      glDeleteProgram(shader_program_id);
      debug::error("could not create shader program:\n%s",
                   error_message);
      return false;
   }

   m_id = shader_program_id;

   // note: introspection
   glUseProgram(m_id);
   GLint sampler_count = 0;
   GLint active_uniform_count = 0;
   glGetProgramiv(m_id, GL_ACTIVE_UNIFORMS, &active_uniform_count);
   debug::info("shader_program_t: %d - uniforms: %d", m_id, active_uniform_count);

   for (int index = 0; index < active_uniform_count; index++) {
      GLint uniform_ = 0;
      GLenum uniform_type = GL_NONE;
      GLchar uniform_name[128] = {};
      GLsizei uniform_name_length = 0;
      glGetActiveUniform(m_id,
                         index,
                         sizeof(uniform_name),
                         &uniform_name_length,
                         &uniform_,
                         &uniform_type,
                         uniform_name);

      GLint location = glGetUniformLocation(m_id, uniform_name);
      if (uniform_type == GL_SAMPLER_2D) {
         debug::info(" + %s - location: %d type: sampler2d", 
                     uniform_name,
                     sampler_count);
         glUniform1i(location, sampler_count);
         sampler_count++;
         continue;
      }

      debug::info(" + %s - location: %d type: %s",
                  uniform_name,
                  location,
                  gl_uniform_type_string(uniform_type));

      uint32_t uniform_name_hash = fnv1a32(uniform_name, uniform_name_length);
      m_uniforms.emplace_back(location, uniform_name_hash, uniform_type, 0);
   }

   glUseProgram(0);
   if (glGetError() != GL_NO_ERROR) {
      glDeleteProgram(m_id);

      m_id = 0;
      m_uniforms.clear();

      debug::error("could not create shader program!");
   }

   return valid();
}

bool shader_program_t::create_from_file(const std::string_view &vertex_path,
                                        const std::string_view &fragment_path) 
{
   std::string vertex_source;
   if (!file_system_t::load_content(vertex_path, vertex_source)) {
      return false;
   }

   std::string fragment_source;
   if (!file_system_t::load_content(fragment_path, fragment_source)) {
      return false;
   }

   return create(vertex_source.c_str(), fragment_source.c_str());
}

void shader_program_t::destroy()
{
   if (valid()) {
      glDeleteProgram(m_id);
   }

   m_id = 0;
}

bool texture_t::valid() const
{
   return m_id != 0;
}

struct pixel_format_desc {
   GLenum internal_format;
   GLenum provided_format;
   GLenum pixel_element_type;
};

static pixel_format_desc gl_pixel_formats[] =
{
   { GL_R8     , GL_RED    , GL_UNSIGNED_BYTE },
   { GL_RG8    , GL_RG     , GL_UNSIGNED_BYTE },
   { GL_RGB8   , GL_RGB    , GL_UNSIGNED_BYTE },
   { GL_RGBA8  , GL_RGBA   , GL_UNSIGNED_BYTE },
};
static_assert(int(texture_t::pixel_format_t::count) == (sizeof(gl_pixel_formats) / sizeof(gl_pixel_formats[0])), "texture pixel_format mismatch!");

static texture_t::pixel_format_t
determine_pixel_format(int components) {
   switch (components) {
   case 1: return texture_t::pixel_format_t::r8;
   case 2: return texture_t::pixel_format_t::rg8;
   case 3: return texture_t::pixel_format_t::rgb8;
   case 4: return texture_t::pixel_format_t::rgba8;
   }
   return texture_t::pixel_format_t::unknown;
}

bool texture_t::create(const int width,
                       const int height,
                       const void *data,
                       const pixel_format_t format,
                       const bool mipmap)
{
   const pixel_format_desc &desc = gl_pixel_formats[int(format)];

   GLuint texture_id = 0;
   glGenTextures(1, &texture_id);
   glBindTexture(GL_TEXTURE_2D, texture_id);
   glTexImage2D(GL_TEXTURE_2D,
                0,
                desc.internal_format,
                width,
                height,
                0,
                desc.provided_format,
                desc.pixel_element_type,
                data);

   int levels = 1;
   if (mipmap) {
      glGenerateMipmap(GL_TEXTURE_2D);
      opengl_check_errors();

      int value = width > height ? width : height;
      while (value > 1) {
         value >>= 1;
         levels++;
      }
   }

   glBindTexture(GL_TEXTURE_2D, 0);
   if (glGetError() != GL_NO_ERROR) {
      glDeleteTextures(1, &texture_id);
      debug::error("could not create texture!");
      return false;
   }

   m_id = texture_id;
   m_width = width;
   m_height = height;

   debug::info("texture_t: %d - size: %dx%d levels: %d", m_id, width, height, levels);

   return valid();
}

bool texture_t::create_from_file(const std::string_view &filename, const bool mipmap)
{
   std::vector<uint8_t> content;
   if (!file_system_t::load_content(filename, content)) {
      return false;
   }

   int width = 0, height = 0, components = 0;
   stbi_uc *bitmap = stbi_load_from_memory(content.data(),
                                           int(content.size()),
                                           &width,
                                           &height,
                                           &components,
                                           STBI_default);
   if (bitmap == nullptr) {
      debug::warn("could not load image data: '%s'!", filename);
      return false;
   }

   create(width, height, bitmap, determine_pixel_format(components), mipmap);
   stbi_image_free(bitmap);

   return valid();
}

void texture_t::destroy()
{
   if (valid()) {
      glDeleteTextures(1, &m_id);
   }

   m_id = 0;
   m_width = 0;
   m_height = 0;
}

bool sampler_state_t::valid() const
{
   return m_id != 0;
}

static const GLenum gl_filter_modes[] =
{
   GL_NEAREST,
   GL_LINEAR,
   GL_NEAREST_MIPMAP_NEAREST,
   GL_NEAREST_MIPMAP_LINEAR,
   GL_LINEAR_MIPMAP_NEAREST,
   GL_LINEAR_MIPMAP_LINEAR,
};

static const GLenum gl_address_modes[] =
{
   GL_CLAMP_TO_EDGE,
   GL_CLAMP_TO_BORDER,
   GL_REPEAT,
   GL_MIRRORED_REPEAT
};

bool sampler_state_t::create(const filter_mode_t filter,
                             const address_mode_t address_u,
                             const address_mode_t address_v)
{
   GLuint sampler_state_id = 0;
   glGenSamplers(1, &sampler_state_id);
   glSamplerParameteri(sampler_state_id, GL_TEXTURE_MIN_FILTER, gl_filter_modes[int(filter)]);
   glSamplerParameteri(sampler_state_id, GL_TEXTURE_MAG_FILTER, filter == filter_mode_t::nearest ? GL_NEAREST : GL_LINEAR);
   glSamplerParameteri(sampler_state_id, GL_TEXTURE_WRAP_S, gl_address_modes[int(address_u)]);
   glSamplerParameteri(sampler_state_id, GL_TEXTURE_WRAP_T, gl_address_modes[int(address_v)]);
   if (glGetError() != GL_NO_ERROR) {
      glDeleteSamplers(1, &sampler_state_id);
      debug::error("could not create sampler state!");
      return false;
   }

   m_id = sampler_state_id;

   return valid();
}

void sampler_state_t::destroy()
{
   if (valid()) {
      glDeleteSamplers(1, &m_id);
   }

   m_id = 0;
}

bool vertex_buffer_t::valid() const
{
   return m_id != 0;
}

bool vertex_buffer_t::create(const size_t size,
                             const void *data,
                             const usage_hint_t usage)
{
   GLuint vertex_buffer_id = 0;
   glGenBuffers(1, &vertex_buffer_id);
   glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
   glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   if (glGetError() != GL_NO_ERROR) {
      glDeleteBuffers(1, &vertex_buffer_id);
      debug::error("could not create vertex buffer!");
      return false;
   }

   m_id = vertex_buffer_id;

   return valid();
}

void vertex_buffer_t::destroy()
{
   if (valid()) {
      glDeleteBuffers(1, &m_id);
   }

   m_id = 0;
}

struct vertex_attrib_desc {
   GLenum type;
   GLint  size;
};

static const vertex_attrib_desc gl_attrib_desc[] =
{
   { GL_FLOAT        , sizeof(float)   },
   { GL_UNSIGNED_BYTE, sizeof(uint8_t) },
};

vertex_layout_t &vertex_layout_t::clear()
{
   m_count = 0;
   m_stride = 0;

   return *this;
}

vertex_layout_t &vertex_layout_t::add(const attribute_type_t type,
                                      const uint32_t count,
                                      const bool normalized)
{
   assert(m_count < max_vertex_attributes);
   assert(count <= 4);

   m_attributes[m_count].index = m_count;
   m_attributes[m_count].type  = gl_attrib_desc[int(type)].type;
   m_attributes[m_count].size  = gl_attrib_desc[int(type)].size;
   m_attributes[m_count].count = count;
   m_attributes[m_count].normalized = normalized ? GL_TRUE : GL_FALSE;
   m_count++;

   m_stride += gl_attrib_desc[int(type)].size * count;

   return *this;
}

static GLuint gl_vertex_array_object_id = 0;

renderer_t::renderer_t()
{
   assert(gl_vertex_array_object_id == 0);
   glGenVertexArrays(1, &gl_vertex_array_object_id);
   glBindVertexArray(gl_vertex_array_object_id);
   debug::info("created gl_vertex_array_object_id: %d", gl_vertex_array_object_id);
}

renderer_t::~renderer_t()
{
   glDeleteVertexArrays(1, &gl_vertex_array_object_id);
}

void renderer_t::clear(const color_t &color, const float depth)
{
   glClearDepth(depth);
   glClearColor(color.r, color.g, color.b, color.a);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   opengl_check_errors();
}

void renderer_t::set_viewport(const viewport_t &viewport)
{
   glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
   opengl_check_errors();
}

void renderer_t::set_shader_program(shader_program_t &program)
{
   m_program = &program;

   glUseProgram(program.m_id);
   opengl_check_errors();
}

void renderer_t::set_uniform(const std::string_view &name, const glm::vec3 &value)
{
   assert(m_program);

   const uint32_t name_hash = fnv1a32(name.data(), name.length());

   auto &uniforms = m_program->m_uniforms;
   for (auto &uniform : uniforms) {
      if (uniform.m_name_hash == name_hash) {
         assert(uniform.m_value_type == GL_FLOAT_VEC3);

         const uint32_t value_hash = fnv1a32(glm::value_ptr(value), sizeof(glm::vec3));
         if (uniform.m_value_hash != value_hash) {
            uniform.m_value_hash = value_hash;

            glUniform3fv(uniform.m_location, 1, glm::value_ptr(value));
         }

         break;
      }
   }
}

void renderer_t::set_uniform(const std::string_view &name, const glm::vec4 &value)
{
   assert(m_program);

   const uint32_t name_hash = fnv1a32(name.data(), name.length());

   auto &uniforms = m_program->m_uniforms;
   for (auto &uniform : uniforms) {
      if (uniform.m_name_hash == name_hash) {
         assert(uniform.m_value_type == GL_FLOAT_VEC4);

         const uint32_t value_hash = fnv1a32(glm::value_ptr(value), sizeof(glm::vec4));
         if (uniform.m_value_hash != value_hash) {
            uniform.m_value_hash = value_hash;

            glUniform4fv(uniform.m_location, 1, glm::value_ptr(value));
         }

         break;
      }
   }
}

void renderer_t::set_uniform(const std::string_view &name, const glm::mat4 &value)
{
   assert(m_program);

   const uint32_t name_hash = fnv1a32(name.data(), name.length());

   auto &uniforms = m_program->m_uniforms;
   for (auto &uniform : uniforms) {
      if (uniform.m_name_hash == name_hash) {
         assert(uniform.m_value_type == GL_FLOAT_MAT4);

         const uint32_t value_hash = fnv1a32(glm::value_ptr(value), sizeof(glm::mat4));
         if (uniform.m_value_hash != value_hash) {
            uniform.m_value_hash = value_hash;

            glUniformMatrix4fv(uniform.m_location, 1, GL_FALSE, glm::value_ptr(value));
         }

         break;
      }
   }
}

void renderer_t::set_texture(texture_t &texture, const int unit)
{
   glActiveTexture(GL_TEXTURE0 + unit);
   glBindTexture(GL_TEXTURE_2D, texture.m_id);
   opengl_check_errors();
}

void renderer_t::set_sampler_state(sampler_state_t &sampler, const int unit)
{
   glBindSampler(unit, sampler.m_id);
   opengl_check_errors();
}

static const GLenum gl_blend_equations[] =
{
   GL_FUNC_ADD,
   GL_FUNC_SUBTRACT,
   GL_FUNC_REVERSE_SUBTRACT,
   GL_MIN,
   GL_MAX,
};

static const GLenum gl_blend_factors[] =
{
   GL_ZERO,
   GL_ONE,
   GL_SRC_COLOR,
   GL_ONE_MINUS_SRC_COLOR,
   GL_DST_COLOR,
   GL_ONE_MINUS_DST_COLOR,
   GL_SRC_ALPHA,
   GL_ONE_MINUS_SRC_ALPHA,
   GL_DST_ALPHA,
   GL_ONE_MINUS_DST_ALPHA,
   GL_CONSTANT_COLOR,
   GL_ONE_MINUS_CONSTANT_COLOR,
   GL_CONSTANT_ALPHA,
   GL_ONE_MINUS_CONSTANT_ALPHA,
   GL_SRC_ALPHA_SATURATE,
};

void renderer_t::set_blend_state(blend_state_t &state)
{
   if (state.m_enabled) {
      glEnable(GL_BLEND);
      glBlendFuncSeparate(gl_blend_factors[int(state.m_color_src)],
                          gl_blend_factors[int(state.m_color_dest)],
                          gl_blend_factors[int(state.m_alpha_src)],
                          gl_blend_factors[int(state.m_alpha_dest)]);
      glBlendEquationSeparate(gl_blend_equations[int(state.m_color_eq)],
                              gl_blend_equations[int(state.m_alpha_eq)]);
   }
   else {
      glDisable(GL_BLEND);
   }

   opengl_check_errors();
}

static const GLenum gl_compare_funcs[] =
{
   GL_NEVER,
   GL_LESS,
   GL_EQUAL,
   GL_LEQUAL,
   GL_GREATER,
   GL_NOTEQUAL,
   GL_GEQUAL,
   GL_ALWAYS,
};

void renderer_t::set_depth_stencil_state(depth_stencil_state_t &state)
{
   if (state.m_read) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(gl_compare_funcs[int(state.m_func)]);

      if (state.m_write) {
         glDepthMask(GL_TRUE);
      }
      else {
         glDepthMask(GL_FALSE);
      }
   }
   else {
      glDisable(GL_DEPTH_TEST);
   }

   glDepthRange(state.m_znear, state.m_zfar);
   opengl_check_errors();
}

static const GLenum gl_cull_modes[] =
{
   GL_NONE,
   GL_BACK,
   GL_FRONT,
   GL_FRONT_AND_BACK,
};

static const GLenum gl_front_faces[] =
{
   GL_CCW,
   GL_CW,
};

void renderer_t::set_rasterizer_state(rasterizer_state_t &state)
{
   if (state.m_cull_mode != rasterizer_state_t::cull_mode_t::none) {
      glEnable(GL_CULL_FACE);
      glCullFace(gl_cull_modes[int(state.m_cull_mode)]);
   }
   else {
      glDisable(GL_CULL_FACE);
   }

   glFrontFace(gl_front_faces[int(state.m_front_face)]);
   glPolygonMode(GL_FRONT_AND_BACK, state.m_polygon_mode == rasterizer_state_t::polygon_mode_t::fill ? GL_FILL : GL_LINE);
   opengl_check_errors();
}

void renderer_t::set_vertex_buffer_and_layout(vertex_buffer_t &buffer, vertex_layout_t &layout)
{
   glBindBuffer(GL_ARRAY_BUFFER, buffer.m_id);
   opengl_check_errors();

   uint8_t *offset = nullptr;
   for (uint32_t index = 0; index < layout.m_count; index++) {
      auto &attrib = layout.m_attributes[index];

      glEnableVertexAttribArray(attrib.index);
      glVertexAttribPointer(attrib.index,
                            attrib.count,
                            attrib.type,
                            (GLboolean)attrib.normalized,
                            layout.m_stride,
                            offset);

      offset += attrib.count * attrib.size;
      opengl_check_errors();
   }
}

static const GLenum gl_topology_types[] =
{
   GL_POINTS,
   GL_LINES,
   GL_TRIANGLES,
};

void renderer_t::draw(const topology_t topology, const int start, const int count)
{
   glDrawArrays(gl_topology_types[int(topology)], start, count);
   opengl_check_errors();
}
