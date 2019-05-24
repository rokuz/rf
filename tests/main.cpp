#include "rf.hpp"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  rf::Logger::ToLogWithFormat(rf::Logger::Info, "Test suite started at %s.",
                              rf::Utils::CurrentTimeDate().c_str());

  rf::Window window;
  window.Initialize(100, 100);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
