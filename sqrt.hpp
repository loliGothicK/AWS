#pragma once

#include <limits>
#include <type_traits>
#include <cstdint>

namespace constexpr_sqrt_u {
  // Change: Non template to function template 
  /*
  [ Note:
    Reason: To solve a issue size_t overload resolution is ambiguous.
  -- end note ]
  */
  template < typename T >
  constexpr size_t sqrt_u(T number)
  {
    /*
    [ Note:
      Constexpr if statement is C++17 feature.
    -- end note ]
    */
    if constexpr (std::is_floating_point_v<T>)
    { // this block evaluate if and only if T is floating point type.
      if (!(number == number) || number < T{}) {
        return std::numeric_limits<T>::quiet_NaN();
      }

      if (number == std::numeric_limits<T>::infinity()) {
        return std::numeric_limits<T>::infinity();
      }
    }

    if (number<static_cast<T>(1)) return 0;
    // simple binary search
    double upper = static_cast<double>(number);
    double lower = 1;
    double root = (lower + upper) / 2;
    auto absolute = [](double d) { return d < 0.0 ? -d : d; };
    while (absolute(root*root - number) >= 1) {
      (root*root > number ? upper : lower) = root;
      root = (lower + upper) / 2;
    }
    
    if constexpr (std::is_integral_v<T>) {
      auto ret = static_cast<T>(root);
      return (ret + 1)*(ret + 1) == number ? ret + 1 : (ret - 1)*(ret - 1) == number ? ret - 1 : ret;
    }
    else {
      return static_cast<T>(root);
    }
  }
}
