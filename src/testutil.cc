#include "include/testutil.h"
#include "include/random.h"

#include <string>

namespace slash {

extern std::string RandomString(const int len) {
  char buf[len];
  for (int i = 0; i < len; i++) {
    buf[i] = Random::Uniform('z' - 'a') + 'a';
  }
  return buf;
}

}  // namespace slash
