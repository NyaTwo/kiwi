// application.cpp

#include "application.hpp"
#include <GLFW/glfw3.h>

#pragma warning(push)
#pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <numbers> 

application_t::application_t()
   : m_position(0.0f)
   , m_rotation(0.0f)
   , m_world(1.0f)
{
   event_dispatcher_t::add_listener<mouse_moved_t>(*this);
   event_dispatcher_t::add_listener<key_pressed_t>(*this);
   event_dispatcher_t::add_listener<key_released_t>(*this);
   event_dispatcher_t::add_listener<button_pressed_t>(*this);
   event_dispatcher_t::add_listener<button_released_t>(*this);
}

bool application_t::on_initialize()
{
   if (!m_program.create_from_file("assets/shader.vs.glsl", "assets/shader.fs.glsl")) {
      return false;
   }

   if (!setTextures()) {
      return false;
   }

   if (!m_sampler.create(sampler_state_t::filter_mode_t::linear)) {
      return false;
   }

   if (!makeObjects()) {
      return false;
   }

   return true;
}

bool application_t::makeObjects()
{
    bool success = true;
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    success &= make_cube(m_cube, m_layout, m_cube_primitive_count, 1.0f);
    return success;
}

bool application_t::setTextures()
{
    bool success = true;
    success &= m_textureSun.create_from_file("assets/8k_sun.jpg");
    success &= m_textureMercury.create_from_file("assets/8k_mercury.jpg");
    success &= m_textureVenus.create_from_file("assets/8k_venus.jpg");
    success &= m_textureEarth.create_from_file("assets/8k_earth.jpg");
    success &= m_textureMoon.create_from_file("assets/8k_moon.jpg");
    success &= m_textureMars.create_from_file("assets/8k_mars.jpg");
    success &= m_textureJupiter.create_from_file("assets/8k_jupiter.jpg");
    success &= m_textureSaturn.create_from_file("assets/8k_saturn.jpg");
    success &= m_textureUranus.create_from_file("assets/2k_uranus.jpg");
    success &= m_textureNeptune.create_from_file("assets/2k_neptune.jpg");
    return success;
}


void application_t::on_shutdown()
{
}

bool application_t::on_update(const timespan_t &deltatime,
                              const timespan_t &apptime)
{
   constexpr float cube_origin_z = -12.0f;
   constexpr float cube_span_z = 10.0f;
   constexpr float cube_speed_factor = 2.0f;

   m_position.z = cube_origin_z + std::cosf(apptime.elapsed_seconds() * cube_speed_factor) * cube_span_z;
   m_rotation.x += deltatime.elapsed_seconds();
   m_rotation.y += deltatime.elapsed_seconds();
   m_rotation.z += deltatime.elapsed_seconds();

   m_world = glm::translate(glm::mat4(1.0f), m_position) *
      glm::rotate(glm::mat4(1.0f), m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f))
      ;

   return m_running;
}

void application_t::on_render(const viewport_t &viewport)
{
   // todo: remove this later
   glm::mat4 projection = glm::perspective(std::numbers::pi_v<float> * 0.25f,
                                           float(viewport.width) / viewport.height,
                                           1.0f,
                                           100.0f);

   // note: done once
   m_renderer.clear(color_t{ 0.1f, 0.2f, 0.3f, 1.0f });
   m_renderer.set_viewport(viewport);

   // note: done for each object we want to render
   m_renderer.set_shader_program(m_program);
   m_renderer.set_uniform("u_projection", projection);
   m_renderer.set_uniform("u_world", m_world);
   m_renderer.set_texture(m_textureSun);
   m_renderer.set_sampler_state(m_sampler);
   m_renderer.set_blend_state(m_blend_state);
   m_renderer.set_depth_stencil_state(m_depth_stencil_state);
   m_renderer.set_rasterizer_state(m_rasterizer_state);
   m_renderer.set_vertex_buffer_and_layout(m_cube, m_layout);
   m_renderer.draw(topology_t::triangle_list, 0, m_cube_primitive_count);
}

void application_t::on_event(const mouse_moved_t &event)
{
}

void application_t::on_event(const key_pressed_t &event)
{
   if (event.keycode == GLFW_KEY_SPACE) {
      if (m_rasterizer_state.m_polygon_mode == rasterizer_state_t::polygon_mode_t::fill) {
         m_rasterizer_state.m_polygon_mode = rasterizer_state_t::polygon_mode_t::wireframe;
      }
      else {
         m_rasterizer_state.m_polygon_mode = rasterizer_state_t::polygon_mode_t::fill;
      }
   }
}

void application_t::on_event(const key_released_t &event)
{
   if (event.keycode == GLFW_KEY_ESCAPE) {
      m_running = false;
   }
}

void application_t::on_event(const button_pressed_t &event)
{
}

void application_t::on_event(const button_released_t &event)
{
}

bool application_t::make_cube(vertex_buffer_t &buffer, vertex_layout_t &layout, int &primitive_count, float size)
{
   // todo: move this to a shared header!
   struct vertex3d_t {
      glm::vec3 position;
      glm::vec2 texcoord;
      glm::vec4 color;
   };

   // note: we are using origo (0,0,0) and we are extending 'size' units on each axis
   //       divide by two...
   size *= 0.5f;
   
   // note: each corner of the cube
   const glm::vec3 positions[8] =
   {
      glm::vec3{ -size, size, size },
      glm::vec3{  size, size, size },
      glm::vec3{  size,-size, size },
      glm::vec3{ -size,-size, size },
      glm::vec3{  size, size,-size },
      glm::vec3{ -size, size,-size },
      glm::vec3{ -size,-size,-size },
      glm::vec3{  size,-size,-size },
   };

   // note: we are applying the whole texture on each side of the cube
   const glm::vec2 texcoords[4] =
   {
      glm::vec2{ 0.0f, 0.0f },
      glm::vec2{ 1.0f, 0.0f },
      glm::vec2{ 1.0f, 1.0f },
      glm::vec2{ 0.0f, 1.0f },
   };

   // note: this only uses one color for all vertices
   const glm::vec4 colors[1] =
   {
      glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }
   };

   // note: cube data, each face
   const vertex3d_t vertices[] =
   {
      // front
      { positions[0], texcoords[0], colors[0] },
      { positions[1], texcoords[1], colors[0] },
      { positions[2], texcoords[2], colors[0] },
      { positions[2], texcoords[2], colors[0] },
      { positions[3], texcoords[3], colors[0] },
      { positions[0], texcoords[0], colors[0] },

      // right
      { positions[1], texcoords[0], colors[0] },
      { positions[4], texcoords[1], colors[0] },
      { positions[7], texcoords[2], colors[0] },
      { positions[7], texcoords[2], colors[0] },
      { positions[2], texcoords[3], colors[0] },
      { positions[1], texcoords[0], colors[0] },

      // back
      { positions[4], texcoords[0], colors[0] },
      { positions[5], texcoords[1], colors[0] },
      { positions[6], texcoords[2], colors[0] },
      { positions[6], texcoords[2], colors[0] },
      { positions[7], texcoords[3], colors[0] },
      { positions[4], texcoords[0], colors[0] },

      // left
      { positions[5], texcoords[0], colors[0] },
      { positions[0], texcoords[1], colors[0] },
      { positions[3], texcoords[2], colors[0] },
      { positions[3], texcoords[2], colors[0] },
      { positions[6], texcoords[3], colors[0] },
      { positions[5], texcoords[0], colors[0] },

      // top
      { positions[5], texcoords[0], colors[0] },
      { positions[4], texcoords[1], colors[0] },
      { positions[1], texcoords[2], colors[0] },
      { positions[1], texcoords[2], colors[0] },
      { positions[0], texcoords[3], colors[0] },
      { positions[5], texcoords[0], colors[0] },

      // bottom 
      { positions[3], texcoords[0], colors[0] },
      { positions[2], texcoords[1], colors[0] },
      { positions[7], texcoords[2], colors[0] },
      { positions[7], texcoords[2], colors[0] },
      { positions[6], texcoords[3], colors[0] },
      { positions[3], texcoords[0], colors[0] },
   };

   layout
      .clear() // reset the layout
      .add(attribute_type_t::float_, 3, false)
      .add(attribute_type_t::float_, 2, false)
      .add(attribute_type_t::float_, 4, false);

   primitive_count = sizeof(vertices) / sizeof(vertices[0]);
   if (!buffer.create(sizeof(vertices), vertices)) {
      return false;
   }

   return true;
}

bool application_t::make_sphere(vertex_buffer_t& buffer, vertex_layout_t& layout, int& primitive_count, float radius)
{
    return false;
}
