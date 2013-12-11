#ifndef SPLIT_HPP
#define SPLIT_HPP

#include <algorithm>
#include <vector>

/* Find the first occurence of value in the sequence, that is not preceded by
 * an (unescaped) escape value. Returns last if nothing is found. */
template <typename InputIt, typename T>
InputIt find_escaped (InputIt first, InputIt last, const T& value, const T& escape) {
  const T seekset[] = { value, escape };

  do {
    first = std::find_first_of(first, last,
        seekset, &seekset[0] + sizeof(seekset) / sizeof(seekset[0]));
    if (last != first) {
      if (value == *first) {
        return first;
      }
      ++first;
      if (last != first) {
        ++first;
      }
    }
  } while (last != first);

  return last;
}

/* Break a container (type C) of objects of type T into sub sequences. The
 * breakpoints will be at elements equal to a delimiter value, skipping
 * comparison with the elements immediately following an escape value. */
template <typename C, typename T>
std::vector<C> split_escaped (const C& seq, const T& delimiter, const T& escape) {
  std::vector<C> ret;
  typename C::const_iterator first, last;

  first = seq.begin();
  while (seq.end() != last) {
    last = find_escaped(first, seq.end(), delimiter, escape);
    ret.push_back(C(first, last));
    first = last;
    if (seq.end() != first) {
      ++first;
    }
  }

  return ret;
}

#endif
