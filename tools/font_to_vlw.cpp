#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Glyph {
  std::uint32_t code_point = 0;
  std::uint32_t height = 0;
  std::uint32_t width = 0;
  std::uint32_t x_advance = 0;
  std::int32_t d_y = 0;
  std::int32_t d_x = 0;
  std::vector<std::uint8_t> bitmap;
};

class FreeTypeLibrary {
 public:
  FreeTypeLibrary() {
    if (FT_Init_FreeType(&library_) != 0) {
      throw std::runtime_error("failed to initialize FreeType");
    }
  }

  ~FreeTypeLibrary() { FT_Done_FreeType(library_); }

  FT_Library get() const { return library_; }

 private:
  FT_Library library_ = nullptr;
};

class FreeTypeFace {
 public:
  FreeTypeFace(FT_Library library, const std::string& font_path) {
    if (FT_New_Face(library, font_path.c_str(), 0, &face_) != 0) {
      throw std::runtime_error("failed to load font: " + font_path);
    }
  }

  ~FreeTypeFace() { FT_Done_Face(face_); }

  FT_Face get() const { return face_; }

 private:
  FT_Face face_ = nullptr;
};

std::string trim(const std::string& input) {
  const std::size_t start = input.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const std::size_t end = input.find_last_not_of(" \t\r\n");
  return input.substr(start, end - start + 1);
}

std::vector<std::uint32_t> readCodePoints(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open codepoint file: " + path);
  }

  std::vector<std::uint32_t> code_points;
  std::string line;
  std::uint32_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;

    const std::size_t comment = line.find('#');
    if (comment != std::string::npos) {
      line.erase(comment);
    }

    line = trim(line);
    if (line.empty()) {
      continue;
    }

    std::size_t parsed = 0;
    const unsigned long value = std::stoul(line, &parsed, 16);
    if (parsed != line.size()) {
      std::ostringstream message;
      message << "invalid hex code point on line " << line_number << ": "
              << line;
      throw std::runtime_error(message.str());
    }
    if (value <= 0xFFFFUL) {
      code_points.push_back(static_cast<std::uint32_t>(value));
    }
  }

  return code_points;
}

std::uint32_t rounded26Dot6(FT_Pos value) {
  if (value >= 0) {
    return static_cast<std::uint32_t>((value + 32) >> 6);
  }
  return static_cast<std::uint32_t>((-value + 32) >> 6);
}

void writeBigEndian32(std::ostream& output, std::uint32_t value) {
  const char bytes[] = {
      static_cast<char>((value >> 24) & 0xFF),
      static_cast<char>((value >> 16) & 0xFF),
      static_cast<char>((value >> 8) & 0xFF),
      static_cast<char>(value & 0xFF),
  };
  output.write(bytes, sizeof(bytes));
}

std::vector<std::uint8_t> copyBitmap(const FT_Bitmap& bitmap) {
  if (bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
    throw std::runtime_error("FreeType rendered a non-grayscale bitmap");
  }

  std::vector<std::uint8_t> bytes;
  bytes.reserve(static_cast<std::size_t>(bitmap.width) * bitmap.rows);
  const int pitch = bitmap.pitch;
  const int stride = std::abs(pitch);
  for (unsigned int y = 0; y < bitmap.rows; ++y) {
    const unsigned int source_y = pitch >= 0 ? y : bitmap.rows - 1 - y;
    const unsigned char* row = bitmap.buffer + source_y * stride;
    bytes.insert(bytes.end(), row, row + bitmap.width);
  }
  return bytes;
}

Glyph renderGlyph(FT_Face face, std::uint32_t code_point) {
  if (FT_Load_Char(face, code_point, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL) !=
      0) {
    std::ostringstream message;
    message << "failed to render U+" << std::uppercase << std::hex
            << code_point;
    throw std::runtime_error(message.str());
  }

  FT_GlyphSlot slot = face->glyph;
  Glyph glyph;
  glyph.code_point = code_point;
  glyph.height = slot->bitmap.rows;
  glyph.width = slot->bitmap.width;
  glyph.x_advance = rounded26Dot6(slot->advance.x);
  glyph.d_y = slot->bitmap_top;
  glyph.d_x = slot->bitmap_left;
  glyph.bitmap = copyBitmap(slot->bitmap);
  return glyph;
}

void printGlyphValidationError(const Glyph& glyph,
                               const char* field,
                               std::int64_t value) {
  std::cerr << "invalid VLW glyph U+" << std::uppercase << std::hex
            << std::setw(4) << std::setfill('0') << glyph.code_point
            << std::dec << std::setfill(' ') << ": " << field << "="
            << value << "\n";
}

bool validateGlyphs(const std::vector<Glyph>& glyphs) {
  bool valid = true;
  if (glyphs.size() > std::numeric_limits<std::uint16_t>::max()) {
    std::cerr << "invalid VLW font: glyphCount=" << glyphs.size() << "\n";
    valid = false;
  }

  std::uint32_t previous_code_point = 0;
  for (std::size_t index = 0; index < glyphs.size(); ++index) {
    const Glyph& glyph = glyphs[index];
    if (glyph.code_point > 0xFFFF) {
      printGlyphValidationError(glyph, "codePoint", glyph.code_point);
      valid = false;
    }
    if (index > 0 && glyph.code_point <= previous_code_point) {
      printGlyphValidationError(glyph, "codePointOrder", glyph.code_point);
      valid = false;
    }
    previous_code_point = glyph.code_point;

    if (glyph.height > 255) {
      printGlyphValidationError(glyph, "height", glyph.height);
      valid = false;
    }
    if (glyph.width > 255) {
      printGlyphValidationError(glyph, "width", glyph.width);
      valid = false;
    }
    if (glyph.x_advance > 255) {
      printGlyphValidationError(glyph, "xAdvance", glyph.x_advance);
      valid = false;
    }
    if (glyph.d_x < -128 || glyph.d_x > 127) {
      printGlyphValidationError(glyph, "dX", glyph.d_x);
      valid = false;
    }
    if (glyph.d_y < -32768 || glyph.d_y > 32767) {
      printGlyphValidationError(glyph, "dY", glyph.d_y);
      valid = false;
    }

    const std::uint64_t expected_bitmap_size =
        static_cast<std::uint64_t>(glyph.width) * glyph.height;
    if (glyph.bitmap.size() != expected_bitmap_size) {
      printGlyphValidationError(
          glyph,
          "bitmapSize",
          static_cast<std::int64_t>(glyph.bitmap.size()));
      valid = false;
    }
  }

  return valid;
}

void writeVlw(const std::string& path,
              const std::vector<Glyph>& glyphs,
              std::uint32_t y_advance,
              std::uint32_t ascent,
              std::uint32_t descent) {
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("failed to open output VLW file: " + path);
  }

  writeBigEndian32(output, static_cast<std::uint32_t>(glyphs.size()));
  writeBigEndian32(output, 11);
  writeBigEndian32(output, y_advance);
  writeBigEndian32(output, 0);
  writeBigEndian32(output, ascent);
  writeBigEndian32(output, descent);

  for (const Glyph& glyph : glyphs) {
    writeBigEndian32(output, glyph.code_point);
    writeBigEndian32(output, glyph.height);
    writeBigEndian32(output, glyph.width);
    writeBigEndian32(output, glyph.x_advance);
    writeBigEndian32(output, static_cast<std::uint32_t>(glyph.d_y));
    writeBigEndian32(output, static_cast<std::uint32_t>(glyph.d_x));
    writeBigEndian32(output, 0);
  }

  for (const Glyph& glyph : glyphs) {
    output.write(reinterpret_cast<const char*>(glyph.bitmap.data()),
                 static_cast<std::streamsize>(glyph.bitmap.size()));
  }

  output.close();
  if (!output) {
    throw std::runtime_error("failed to write complete VLW file: " + path);
  }
}

int run(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0]
              << " <font.ttf> <codepoints.txt> <pixel-size> <output.vlw>\n";
    return 2;
  }

  const std::string font_path = argv[1];
  const std::string codepoint_path = argv[2];
  const int pixel_size = std::stoi(argv[3]);
  if (pixel_size <= 0) {
    throw std::runtime_error("pixel size must be positive");
  }
  const std::string output_path = argv[4];

  const std::vector<std::uint32_t> code_points =
      readCodePoints(codepoint_path);
  if (code_points.empty()) {
    throw std::runtime_error("no BMP code points were requested");
  }

  FreeTypeLibrary library;
  FreeTypeFace face(library.get(), font_path);
  if (FT_Select_Charmap(face.get(), FT_ENCODING_UNICODE) != 0) {
    throw std::runtime_error("font does not provide a Unicode charmap");
  }
  if (FT_Set_Pixel_Sizes(face.get(), 0, pixel_size) != 0) {
    throw std::runtime_error("failed to set FreeType pixel size");
  }

  std::vector<std::uint32_t> available_code_points;
  available_code_points.reserve(code_points.size());
  for (const std::uint32_t code_point : code_points) {
    if (FT_Get_Char_Index(face.get(), code_point) == 0) {
      std::cerr << "missing glyph: U+" << std::uppercase << std::hex
                << std::setw(4) << std::setfill('0') << code_point
                << std::dec << "\n";
    } else {
      available_code_points.push_back(code_point);
    }
  }

  std::vector<Glyph> glyphs;
  glyphs.reserve(available_code_points.size());
  for (const std::uint32_t code_point : available_code_points) {
    glyphs.push_back(renderGlyph(face.get(), code_point));
  }
  if (!validateGlyphs(glyphs)) {
    return 1;
  }

  const std::uint32_t y_advance =
      rounded26Dot6(face.get()->size->metrics.height);
  const std::uint32_t ascent =
      rounded26Dot6(face.get()->size->metrics.ascender);
  const std::uint32_t descent =
      rounded26Dot6(face.get()->size->metrics.descender);

  writeVlw(output_path, glyphs, y_advance, ascent, descent);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << "font_to_vlw: " << error.what() << "\n";
    return 1;
  }
}
