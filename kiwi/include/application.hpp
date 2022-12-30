// application.hpp

#pragma once

#include "system.hpp"
#include "graphics.hpp"

class application_t {
public:
   application_t();

   // note: enter/exit
   bool on_initialize();
   bool makeObjects();
   bool setTextures();
   void putTexturesInVector();
   void on_shutdown();

   // note: mainloop
   bool on_update(const timespan_t &deltatime,
                  const timespan_t &apptime);
   void on_render(const viewport_t &viewport);

   void renderObject(glm::mat4& projection, unsigned int iterator);

   // note: events
   void on_event(const mouse_moved_t &event);
   void on_event(const key_pressed_t &event);
   void on_event(const key_released_t &event);
   void on_event(const button_pressed_t &event);
   void on_event(const button_released_t &event);

private:
   bool make_cube(vertex_buffer_t &buffer, vertex_layout_t &layout, int &primitive_count, float size);

private:
   bool       m_running = true;
   renderer_t m_renderer;

   // note: for testing
   shader_program_t m_program;
   texture_t        m_textureSun;
   texture_t        m_textureMercury;
   texture_t        m_textureVenus;
   texture_t        m_textureEarth;
   texture_t        m_textureMoon;
   texture_t        m_textureMars;
   texture_t        m_textureJupiter;
   texture_t        m_textureSaturn;
   texture_t        m_textureUranus;
   texture_t        m_textureNeptune;
   std::vector<texture_t> m_textures;
   sampler_state_t  m_sampler;
   std::vector<vertex_buffer_t> m_objects;
   vertex_buffer_t  m_cube;
   vertex_layout_t  m_layout;
   
   blend_state_t    m_blend_state;
   depth_stencil_state_t m_depth_stencil_state;
   rasterizer_state_t m_rasterizer_state;

   int              m_cube_primitive_count = 0;
   unsigned int     iterator = 0;
   glm::vec3        m_position;
   glm::vec3        m_rotation;
   glm::mat4        m_world;
};
