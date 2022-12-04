// main.cpp

#include "application.hpp"

#include <glad/glad.h>
#include <glfw/glfw3.h>

int main(int argc, char **argv)
{
   // note: initialize glfw
   if (!glfwInit()) {
      debug::error("could not initialize glfw!");
      return 0;
   }

   // note: specify window and render context settings (double buffering and opengl v3.3 core context)
   glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   GLFWwindow *window = glfwCreateWindow(1280, 720, "kiwi", nullptr, nullptr);
   if (window == nullptr) {
      debug::error("could not create window!");
      return 0;
   }

   // note: set current active render context and load opengl functions for opengl v3.3 core
   glfwMakeContextCurrent(window);
   if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
      debug::error("could not load opengl functions!");
      return 0;
   }

   // note: request vsync on
   glfwSwapInterval(1);
   
   // note: instanciate app
   application_t *app_ = new application_t;
   application_t &app = *app_;
   if (!app.on_initialize()) {
      return 0;
   }

   // note: set up event callbacks
   glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos) {
      event_dispatcher_t::dispatch(mouse_moved_t{ int(xpos), int(ypos) });
   });

   glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
      if (action == GLFW_PRESS) {
         event_dispatcher_t::dispatch(button_pressed_t{ button });
      }
      else if (action == GLFW_RELEASE) {
         event_dispatcher_t::dispatch(button_released_t{ button });
      } 
   });

   glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
      if (action == GLFW_PRESS) {
         event_dispatcher_t::dispatch(key_pressed_t{ key });
      }
      else if (action == GLFW_RELEASE) {
         event_dispatcher_t::dispatch(key_released_t{ key});
      }
   });

   // note: get monitor refresh rate
   GLFWmonitor *monitor = glfwGetPrimaryMonitor();
   const GLFWvidmode *videomode = glfwGetVideoMode(monitor);
   const float monitor_refresh_rate = float(videomode->refreshRate);

   // note: time-dependent updates, here we go! superstar dj!
   float frames_per_second = monitor_refresh_rate;
   timespan_t time_last_frame;
   timespan_t average_frame_duration = timespan_t::from_seconds(1.0f / frames_per_second);

   // note: mainloop as long as the window is open
   while (!glfwWindowShouldClose(window)) {
      // note: calculate frame duration and frames per second ...
      const timespan_t current_time = watch_t::time_since_start();
      const timespan_t current_frame_duration = current_time - time_last_frame;
      const float current_frames_per_second = 1.0f / current_frame_duration.elapsed_seconds();
      time_last_frame = current_time;

      // note: ... and averages using leaky integration
      constexpr float leaky_factor_from = 0.98f; 
      constexpr float leaky_factor_to   = 1.0f - leaky_factor_from;
      frames_per_second = frames_per_second * leaky_factor_from + current_frames_per_second * leaky_factor_to;
      average_frame_duration = average_frame_duration * leaky_factor_from + current_frame_duration * leaky_factor_to;

      // note: poll all queued events since last frame
      glfwPollEvents();

      // note: since we have a resizable window (by default in glfw)
      //       get the current dimensions
      int width = 0, height = 0;
      glfwGetWindowSize(window, &width, &height);

      // note: let the application update logic
      if (!app.on_update(current_frame_duration, current_time)) {
         glfwSetWindowShouldClose(window, GLFW_TRUE);
      }

      // note: ... and then render
      app.on_render(viewport_t{ 0, 0, width, height });

      // note: we are done with this frame, swap backbuffer
      glfwSwapBuffers(window);

      { // note: whats cooler than being cool?
         char title[256];
         sprintf_s(title, 
                   "kiwi - window: %dx%d fps: %2.3f frame: %2.3fms", 
                   width, 
                   height, 
                   frames_per_second, 
                   average_frame_duration.elapsed_milliseonds());
         glfwSetWindowTitle(window, title);
      }
   } 

   // note: clean up cr3w!
   app.on_shutdown();
   delete app_;

   // note: destroy the Windows (tm)
   glfwDestroyWindow(window);

   // note: ... and we are done!
   glfwTerminate();

   return 0;
}
