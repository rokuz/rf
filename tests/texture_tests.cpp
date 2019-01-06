#include "rf.hpp"

#include <gtest/gtest.h>

TEST(Texture, Smoke)
{
  rf::Texture tex;
  size_t constexpr width = 100;
  size_t constexpr height = 50;

  std::vector<uint8_t> buf(width * height * 4);
  for (size_t i = 0; i < height; ++i)
  {
    auto v1 = (static_cast<float>(i) / (height - 1)) * 255;
    for (size_t j = 0; j < width * 4; j += 4)
    {
      auto v2 = (static_cast<float>(j) / (width * 4 - 1)) * 255;
      auto v = sqrt(v1 * v1 + v2 * v2);
      buf[i * width * 4 + j] = static_cast<uint8_t>(v);
      buf[i * width * 4 + j + 1] = static_cast<uint8_t>(v);
      buf[i * width * 4 + j + 2] = static_cast<uint8_t>(v);
      buf[i * width * 4 + j + 3] = 255;
    }
  }

  EXPECT_EQ(true, tex.InitializeWithData(GL_RGBA8, buf.data(), width, height, true));

  char const * const kFilename = "test.png";
  rf::SaveTextureToPng(kFilename, tex);

  EXPECT_EQ(true, rf::Utils::IsPathExisted(kFilename));

  EXPECT_EQ(true, tex.Initialize(kFilename));

  rf::Utils::RemoveFile(kFilename);
  EXPECT_EQ(false, rf::Utils::IsPathExisted(kFilename));
}
