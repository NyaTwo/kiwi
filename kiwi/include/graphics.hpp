// graphics.hpp

#pragma once

#include <cstdint>
#include <vector>
#include <string_view>
#include <glm/glm.hpp>

struct color_t {
   float r = 0.0f;
   float g = 0.0f;
   float b = 0.0f;
   float a = 1.0f;
};

struct viewport_t {
   int x = 0;
   int y = 0;
   int width = 0;
   int height = 0;
};

struct shader_program_t {
   struct uniform_t {
      int32_t  m_location;
      uint32_t m_name_hash;
      uint32_t m_value_type;
      uint32_t m_value_hash;
   };

   shader_program_t() = default;

   bool valid() const;
   bool create(const std::string_view &vertex_source,
               const std::string_view &fragment_source);
   bool create_from_file(const std::string_view &vertex_path,
                         const std::string_view &fragment_path);
   void destroy();

   uint32_t               m_id = 0;
   std::vector<uniform_t> m_uniforms;
};

struct texture_t {
   enum class pixel_format_t {
      r8,
      rg8,
      rgb8,
      rgba8,
      count,
      unknown,
   };

   texture_t() = default;

   bool valid() const;
   bool create(const int width,
               const int height,
               const void *data,
               const pixel_format_t format,
               const bool mipmap = false);
   bool create_from_file(const std::string_view &filename, 
                         const bool mipmap = false);
   void destroy();

   uint32_t m_id = 0;
   int32_t  m_width = 0;
   int32_t  m_height = 0;
};

struct sampler_state_t {
   enum class filter_mode_t {
      nearest,
      linear,
      nearest_mip_nearest,
      nearest_mip_linear,
      linear_mip_nearest,
      linear_mip_linear,
   };

   enum class address_mode_t {
      clamp_to_edge,
      clamp_to_border,
      wrap,
      mirror,
   };

   sampler_state_t() = default;

   bool valid() const;
   bool create(const filter_mode_t filter = filter_mode_t::nearest,
               const address_mode_t address_u = address_mode_t::clamp_to_edge,
               const address_mode_t address_v = address_mode_t::clamp_to_edge);
   void destroy();

   uint32_t m_id = 0;
};

struct vertex_buffer_t {
   enum class usage_hint_t {
      immutable,
      dynamic,
   };

   vertex_buffer_t() = default;

   bool valid() const;
   bool create(const size_t size, const void *data,
               const usage_hint_t usage = usage_hint_t::immutable);
   void destroy();

   uint32_t m_id = 0;
};

enum class attribute_type_t {
   float_,
   ubyte,
};

struct vertex_layout_t {
   static constexpr int max_vertex_attributes = 4;

   struct attribute_t {
      uint32_t index;
      uint32_t type;
      uint32_t size;
      uint32_t count;
      uint32_t normalized;
   };

   vertex_layout_t() = default;

   vertex_layout_t &clear();
   vertex_layout_t &add(const attribute_type_t type,
                        const uint32_t count,
                        const bool normalized);

   uint32_t    m_stride = 0;
   uint32_t    m_count = 0;
   attribute_t m_attributes[max_vertex_attributes] = {};
};

struct blend_state_t {
   enum class blend_equation_t {
      add,
      subtract,
      reverse_subtract,
      min,
      max,
   };

   enum class blend_factor_t {
      zero,
      one,
      src_color,
      one_minus_src_color,
      dst_color,
      one_minus_dst_color,
      src_alpha,
      one_minus_src_alpha,
      dst_alpha,
      one_minus_dst_alpha,
      constant_color,
      one_minus_constant_color,
      constant_alpha,
      one_minus_constant_alpha,
      src_alpha_saturate,
   };

   blend_state_t() = default;

   bool             m_enabled = true;
   blend_equation_t m_color_eq = blend_equation_t::add;
   blend_factor_t   m_color_src = blend_factor_t::src_alpha;
   blend_factor_t   m_color_dest = blend_factor_t::one_minus_src_alpha;
   blend_equation_t m_alpha_eq = blend_equation_t::add;
   blend_factor_t   m_alpha_src = blend_factor_t::one;
   blend_factor_t   m_alpha_dest = blend_factor_t::one;
};

struct depth_stencil_state_t {
   enum class compare_func_t {
      never,
      less,
      equal,
      less_equal,
      greater,
      not_equal,
      greater_equal,
      always,
   };

   depth_stencil_state_t() = default;

   bool           m_read = true;
   bool           m_write = true;
   float          m_znear = 0.0f;
   float          m_zfar = 1.0f;
   compare_func_t m_func = compare_func_t::less;
};

struct rasterizer_state_t {
   enum class cull_mode_t {
      none,
      back,
      front,
      both,
   };

   enum front_face_t {
      ccw,
      cw,
   };

   enum polygon_mode_t {
      fill,
      wireframe,
   };

   rasterizer_state_t() = default;

   cull_mode_t    m_cull_mode = cull_mode_t::back;
   front_face_t   m_front_face = front_face_t::cw;
   polygon_mode_t m_polygon_mode = polygon_mode_t::fill;
};

enum class topology_t {
   point_list,
   line_list,
   triangle_list,
};

struct renderer_t {
   renderer_t();
   ~renderer_t();

   void clear(const color_t &color, const float depth = 1.0f);
   void set_viewport(const viewport_t &viewport);
   void set_shader_program(shader_program_t &program);
   void set_uniform(const std::string_view &name, const glm::vec3 &value);
   void set_uniform(const std::string_view &name, const glm::vec4 &value);
   void set_uniform(const std::string_view &name, const glm::mat4 &value);
   void set_texture(texture_t &texture, const int unit = 0);
   void set_sampler_state(sampler_state_t &sampler, const int unit = 0);
   void set_blend_state(blend_state_t &state);
   void set_depth_stencil_state(depth_stencil_state_t &state);
   void set_rasterizer_state(rasterizer_state_t &state);
   void set_vertex_buffer_and_layout(vertex_buffer_t &buffer, vertex_layout_t &layout);
   void draw(const topology_t topology, const int start, const int count);

private:
   shader_program_t *m_program = nullptr;
};
