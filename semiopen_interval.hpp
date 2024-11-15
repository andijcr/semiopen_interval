#include <concepts>
#include <map>

// unit test helper
struct semiopen_interval_tester;
/**
 * class to encode a map of semiopen intervals [K, K+N) -> V, with a default V0
 * value if no interval covers the input
 * @tparam K the type of the key, must be semiregular and less-comparable
 * @tparam V the type of the value, must be regular
 */
template <std::semiregular K, std::regular V>
  requires requires(K k1, K k2) {
    { k1 < k2 } -> std::convertible_to<bool>;
  }
class semiopen_interval {
  friend struct semiopen_interval_tester;

  // default value if the query is not covered by an interval
  V _default_value;
  std::map<K, V> _so_intervals;

public:
  // constructor, it accepts the default value for the range (-inf, inf)
  template <std::convertible_to<V> VV>
  semiopen_interval(VV &&default_value)
      : _default_value(std::forward<VV>(default_value)) {}

  // set [so_begin, so_end) -> val.
  // overwrites any previous value covered by this new interval.
  // if so_begin >= so_end, the operation is a no-op
  template <std::convertible_to<V> VV>
  void assign(K const &so_begin, K const &so_end, VV &&val) {
    if (!(so_begin < so_end)) {
      // no op: we are trying to insert an empty interval
      return;
    }
    // range is not empty

    if (_so_intervals.empty()) {
      // range is enclosed in the current range (-inf, inf)
      if (val != _default_value) {
        // insert the range directly, val is different than _default_value
        _so_intervals.emplace(so_begin, std::forward<VV>(val));
        _so_intervals.emplace(so_end, _default_value);
      }
      return;
    }
    // _so_intervals is not empty

    // invariants:
    // - _so_intervals.size() is at least 2 because of assignment in an empty
    // map
    // - _so_intervals.last() is m_valBegin

    // find the insertion point for so_end
    auto gt_end_it = _so_intervals.upper_bound(so_end);
    if (gt_end_it == _so_intervals.begin()) {
      // the range ends before the start of the map
      if (val != _default_value) {
        // insert the range directly, val is different than _default_value
        _so_intervals.emplace_hint(_so_intervals.begin(), so_end,
                                   _default_value);
        _so_intervals.emplace_hint(_so_intervals.begin(), so_begin,
                                   std::forward<VV>(val));
      }
      return;
    }
    // find the insertion point for so_begin
    auto gt_begin_it = _so_intervals.upper_bound(so_begin);

    // the range might overlap with the map, careful here.
    // insert so_end with the preexising value to signal the end of the range.
    // note on this: it's an extra move that could be removed at the cost of
    // complexity.
    auto so_end_val = std::move(std::prev(gt_end_it)->second);
    auto range_next_it = _so_intervals.insert_or_assign(gt_end_it, so_end,
                                                        std::move(so_end_val));
    // insert so_begin to se the start of the new range
    auto range_start_it = _so_intervals.insert_or_assign(gt_begin_it, so_begin,
                                                         std::forward<VV>(val));

    // note: we inserted nodes that might be removed in the next section,
    // trading runtime cost for less complex code. a better solution would
    // ensure that [so_begin, so_end) needs to be expanded before inserting the
    // nodes.

    // erase entries that are covered by [so_begin, so_end).
    auto to_erase_begin = _so_intervals.end();
    auto to_erase_end = _so_intervals.end();
    // find from where to delete, check if range_start_it needs to be absorbed
    // in the previous range
    if (auto const &val_in_prev_range = range_start_it == _so_intervals.begin()
                                            ? _default_value
                                            : std::prev(range_start_it)->second;
        val_in_prev_range == range_start_it->second) {
      // absorb so_begin in the previous range
      to_erase_begin = range_start_it;
    } else {
      to_erase_begin = std::next(range_start_it);
    }

    // find out to where to delete, check if range needs to be absorbed in the
    // next range
    if (range_start_it->second == range_next_it->second) {
      to_erase_end = std::next(range_next_it);
    } else {
      to_erase_end = range_next_it;
    }

    _so_intervals.erase(to_erase_begin, to_erase_end);
  }

  // query the V for the K key. returns the default value if key is not covered
  // by an interval
  V const &operator[](K const &key) const {
    auto it = _so_intervals.upper_bound(key);
    return it == _so_intervals.begin() ? _default_value : std::prev(it)->second;
  }
};