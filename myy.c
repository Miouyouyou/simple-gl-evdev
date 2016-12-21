#include <GLES3/gl3.h>
#include <helpers/base_gl.h>
#include <helpers/struct.h>
#include <helpers/log.h>

#include <myy.h>

void myy_init() {}

void myy_display_initialised(unsigned int width, unsigned int height) {}

void myy_generate_new_state() {}

void myy_init_drawing() {
  GLuint program =
    glhSetupAndUse("shaders/standard.vert", "shaders/standard.frag",
                   0 /* attributes */, "\0");
}

void myy_draw() {
  glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
  glClearColor(0.3f, 0.2f, 0.5f, 1.0f);
}

void myy_save_state(struct myy_game_state * const state) {}

void myy_resume_state(struct myy_game_state * const state) {}

void myy_cleanup_drawing() {}

void myy_stop() {}

void myy_click(int x, int y, unsigned int button) {}
void myy_doubleclick(int x, int y, unsigned int button) {}
void myy_move(int x, int y) {}
void myy_hover(int x, int y) {}
void myy_key(unsigned int keycode) {}

