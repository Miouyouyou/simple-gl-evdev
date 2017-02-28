precision highp float;

uniform vec4 px_to_norm;
uniform vec2 cursor_pos;

attribute vec4 xyst;

varying vec2 out_st;

void main() {
  /* The screen's bottom left is considered to be at :
     pixels : (0,0) - normalised (-1, -1) (left, bottom)
     The screen's top right is considered to be at :
     pixels : (1920,1080) - normalised (1,1) (right, bottom)
     x will either be 0 or 24.
     y will either be 0 or -24 since the cursor icon is located
     at the bottom right of the cursor position.
     px_to_norm.xy is the inverse of half the screen's size
     px_to_norm.zw is used for recentering
     
     cursor_pos will be the current desired cursor position in pixels.
     When sent to OpenGL, cursor_pos.y will go higher as the cursor go
     higher, and lower when the cursor go lower.
     Note that this is only done when setting the OpenGL value.
     So when cursor_pos.y == the screen (surface) height, the cursor
     will be at the top of the screen (surface).

     (* px_to_norm.xy - px_to_norm.zw) transforms pixels to normalised 
     coordinates by:
     - multiplying the pixels by [1/half_width,-1/half_height], which
       equates to [pixels/half_width, pixels/half_height] if you ignore
       rounding errors
     - recenter the position with (-1,-1)
       Example with a 1920x1080 screen (half -> (960,540)) :
       - cursor_pos = (0,0)
       =>   ( 0, 0) * (1/960, 1/540) + (-1, -1) 
          = ( 0, 0) + (-1, -1)
          = (-1, -1)
            (left, bottom)

       - cursor_pos = (960, 540)
       =>   (960, 540) * (1/960, 1/540) + (-1, -1)
          = ( 1, 1) + (-1, -1)
          = ( 0, 0) 
            (center, center)

       - cursor_pos = (1920,1080)
       =>   (1920, 1080) * (1/960, 1/540) + (-1, -1)
          = (2,2) + (-1, -1) 
          = (1,1)
            (right, top)
  */
  vec2 normalised_pos = (xyst.xy + cursor_pos) * px_to_norm.xy + px_to_norm.zw;
  /*                      xy          z    w   */
  gl_Position = vec4(normalised_pos, 0.5, 1.0);
  out_st = xyst.zw;
}
