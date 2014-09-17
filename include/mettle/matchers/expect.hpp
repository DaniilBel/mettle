#ifndef INC_METTLE_MATCHERS_EXPECT_HPP
#define INC_METTLE_MATCHERS_EXPECT_HPP

#include <sstream>
#include <string>

#include "result.hpp"

namespace mettle {

class expectation_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

template<typename T, typename Matcher>
void expect(const T &value, const Matcher &matcher) {
  auto m = matcher(value);
  if(!m) {
    std::ostringstream ss;
    ss << "expected: " << matcher.desc() << std::endl
       << "actual:   " << matcher_message(m, to_printable(value));
    throw expectation_error(ss.str());
  }
}

template<typename T, typename Matcher>
void expect(const std::string &desc, const T &value, const Matcher &matcher) {
  auto m = matcher(value);
  if(!m) {
    std::ostringstream ss;
    ss << desc << std::endl
       << "expected: " << matcher.desc() << std::endl
       << "actual:   " << matcher_message(m, to_printable(value));
    throw expectation_error(ss.str());
  }
}

} // namespace mettle

#endif