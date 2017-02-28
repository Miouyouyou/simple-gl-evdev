module MyyColor
  class DecompInfo
    # right-shift operand (>>), and-mask operand (&), multiplication operand
    def initialize(rs, am, mult)
      @rs, @am, @mult = rs, am, mult
    end
    def decompose_component(whole_element)
      ((whole_element >> @rs) & @am) * @mult
    end
    def recompose_component(component)
      component / @mult << @rs
    end
  end
  @@rgba = [:r, :g, :b, :a].freeze
  @@orders = {
    # encoding - {r: DecompInfo(rs, am, mult), g: idem, b: idem, a: idem)}
    rgba4444:
      [[12, 0xf, 16], [8, 0xf, 16], [4, 0xf, 16], [0, 0xf, 16]],
    rgba5551:
      [[11, 0b11111, 8], [6,0b11111,8], [1, 0b11111, 8], [0,  1, 255]],
    argb5551:
      [[10, 0b11111, 8], [5,0b11111,8], [0, 0b11111, 8], [15, 1, 255]],
    rgba8888:
      [[24, 0xff, 1], [16, 0xff, 1], [8, 0xff, 1], [0, 0xff, 1]],
    bgra8888:
      [[8, 0xff, 1], [16, 0xff, 1], [24, 0xff, 1], [0, 0xff, 1]],
    argb8888:
      [[16, 0xff, 1], [8, 0xff, 1], [0, 0xff, 1], [24, 0xff, 1]]
  }
  @@orders.each do |encoding,decomp_infos|
    # if encoding is rgba5551 :
    # decomp_infos = [[11, 0b11111, 8],...]
    # encoding = :rgba5551

    @@orders[encoding] = {} # Example : @@orders[:rgba5551] = {}

    # Continuing with the same example :
    # shift remove the first value of an array and return the removed value
    #                                            rs,  am,  mult
    # @@orders[:rgba5551][:r] = DecompInfo.new(*[11,0b11111, 8])
    # @@orders[:rgba5551][:g] = DecompInfo.new(*[5,0b11111, 8])
    # ...

    @@rgba.each do |component|
      component_decomp_infos = decomp_infos.shift
      @@orders[encoding][component] =
        DecompInfo.new(*component_decomp_infos)
    end
  end
  def self.handle_format?(format)
    @@orders.has_key?(format)
  end
  @@hf_meth = method(:handle_format?)
  def self.handle_formats?(*formats)
    formats.flatten.all?(&@@hf_meth)
  end
  def self.handled_formats
    @@orders.keys
  end
  def self.check_infos(infos)
    if infos.nil?
      raise ArgumentError, "No valid informations provided about the encoding #{encoding}"
    end
  end
  def self.decode(encoded_color, encoding: nil, infos: @@orders[encoding])

    check_infos(infos)

    @@rgba.map do |component|

      infos[component].decompose_component(encoded_color)
    end
  end

  ## Use : MyyColors.encode(r, g, b, a, encoding: :rgba5551) -> Fixnum
  ## Note : r, g, b, a must be normalised values in the [0,255] range.
  def self.encode(*rgba_colors, encoding: nil, infos: @@orders[encoding])
    check_infos(infos)

    color_components = rgba_colors.flatten

    color = 0
    @@rgba.each do |component|
      color |= infos[component].recompose_component((color_components.shift))
    end
    color
  end
  def self.convert_pixels(input_pixels, from:, to:)
    input_pixels.map do |pixel|
      encode(decode(pixel, encoding: from), encoding: to)
    end
  end

  module BMP

    @@header_offsets = { content_start_address_info: 0xa }.freeze
    @@unpack_formats = {
      rgba5551: "S<*",
      rgba4444: "S<*",
      rgba8888: "I<*",
      argb8888: "I<*"
    }
    def self.convert_content(filename:, from:, to:)
      if MyyColor.handle_formats?(from, to)
        if !File.exists?(filename)
          raise ArgumentError,
            "Are you sure about that file ? #{filename}"
        end

        infile = File.open(filename, "r")

        infile.seek(@@header_offsets[:content_start_address_info])
        start_address, bitmap_header_size, width, height =
          infile.read(16).unpack("I<4")
        $stderr.puts("[Read] width : %d (%x) -- height : %d (%02x)" %
          [width, width, height, height])
        $stderr.puts "[Read] Starting from : 0x%x" % start_address

        infile.seek(start_address)

        input_pixels = infile.read.unpack(@@unpack_formats[from])

        infile.close

        output = {
          content:
            MyyColor.convert_pixels(input_pixels, from: from, to: to),
          metadata: {width: width, height: height}
        }.freeze

        $stderr.puts("Encoded pixels : %d - By width : %d" %
                     [output[:content].length,
                      output[:content].length/width])

        output
      else
        raise ArgumentError, <<-error.strip.gsub(/\s+/, ' ')
          One of these formats is not supported by MyyColor :
          #{from} - #{to}
          Supported formats :
          #{MyyColor.handled_formats.join('\n')}
        error
      end
    end
  end

  module OpenGL

    module GL
      UNSIGNED_BYTE = 0x1401
      RGBA = 0x1908
      UNSIGNED_SHORT_5_5_5_1 = 0x8034
      UNSIGNED_SHORT_4_4_4_4 = 0x8033
      TEXTURE_2D = 0x0DE1
    end

    @@formats = {
      rgba5551: {headers: [GL::TEXTURE_2D, GL::RGBA,
                           GL::UNSIGNED_SHORT_5_5_5_1, 2],
                 unpack: "S<*", pack: "S<*"},
      rgba4444: {headers: [GL::TEXTURE_2D, GL::RGBA,
                           GL::UNSIGNED_SHORT_4_4_4_4, 2],
                 unpack: "S<*", pack: "S<*"},
      rgba8888: {headers: [GL::RGBA, GL::UNSIGNED_BYTE],
                 unpack: "I<*", pack: "I>*"},
      argb8888: {headers: [GL::RGBA, GL::UNSIGNED_BYTE],
                 unpack: "I<*", pack: "I>*"}
    }

    def self.convert_bmp(bmp_filename:, raw_filename:, from:, to:)
      raw = MyyColor::BMP.convert_content(filename: bmp_filename,
                                          from: from.to_sym,
                                          to: to.to_sym)
      output_format = @@formats[to.to_sym]
      metadata = [
        raw[:metadata][:width], 
        raw[:metadata][:height],
        *output_format[:headers]]
        

      File.open(raw_filename, "w") do |out|
        out.write(metadata.pack("I<*"))
        out.write(raw[:content].flatten.pack(output_format[:pack]))
      end
    end
  end
end
