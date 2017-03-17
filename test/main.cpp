#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

int main(int argc, char* argv[]) {
  const int result = Catch::Session().run(argc, argv);
  system("PAUSE");
  return result < 0xff ? result : 0xff;
}
