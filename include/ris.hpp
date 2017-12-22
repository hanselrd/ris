#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <thread>
#include <typeindex>
#include <type_traits>
#include <utility>

namespace ris {

  namespace internal {
    template <class Base>
    struct null_state {};
  }

  template <class Base>
  class transition_table {
  public:
    template <class From, class To>
    void add() {
      static_assert(
        std::is_base_of<Base, From>::value &&
        std::is_base_of<Base, To>::value,
        "From and To must be derived from Base");
      m_tt.insert(std::make_pair(typeid(From).name(), typeid(To).name()));
    }

    template <class To>
    bool is_allowed(const std::type_index& from_type) {
      static_assert(
        std::is_base_of<Base, To>::value,
        "From and To must be derived from Base");

      if (from_type == typeid(internal::null_state<Base>)) return true;

      auto range = m_tt.equal_range(from_type.name());
      for (auto i = range.first; i != range.second; ++i) {
        if (i->second == typeid(To).name()) {
          return true;
        }
      }
      return false;
    }

    bool empty() {
      return m_tt.empty();
    }
  private:
    std::multimap<std::string, std::string> m_tt;
  };

  template <class Base>
  class state_handler {
  public:
    state_handler(const transition_table<Base>& tt = transition_table<Base>())
      : m_tt(tt) {}

    template <class State, class... Args>
    void change(Args&&... args) {
      static_assert(std::is_base_of<Base, State>::value, "State must be derived from Base");
      if (m_tt.empty() == false) {
        if (m_tt.template is_allowed<State>(m_state_type) == false) { // throw an error
#ifndef RIS_DISABLE_LOGGING
          std::cout << "[" << m_state_type.name() << "] to [" << typeid(State).name() << "] not allowed" << std::endl;
#endif
          return;
        }
      }
      m_state_type = typeid(State);
      m_state = std::make_unique<State>(std::forward<Args>(args)...);
    }
  private:
    std::type_index m_state_type = typeid(internal::null_state<Base>);
    std::unique_ptr<Base> m_state;
    transition_table<Base> m_tt;
  };

}
