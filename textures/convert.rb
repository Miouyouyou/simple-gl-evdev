require_relative 'myy-color'

MyyColor::OpenGL.convert_bmp(bmp_filename: "cursor.bmp",
                             raw_filename: "cursor.raw",
                             from: :argb8888,
                             to: :rgba4444)

