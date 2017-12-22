#include <condition_variable>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <vector>

namespace ris {

  namespace internal {
    template <class Base>
    struct null_state {};
  }

  /*
      Transition table to allow and disallow movement between states
  */
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
    bool is_allowed(const std::type_index& from_type) const {
      static_assert(
        std::is_base_of<Base, To>::value,
        "From and To must be derived from Base");

      if (from_type == typeid(internal::null_state<Base>)) return true;

      auto range = m_tt.equal_range(from_type.name());
      for (auto it = range.first; it != range.second; ++it) {
        if (it->second == typeid(To).name()) {
          return true;
        }
      }
      return false;
    }

    bool empty() const {
      return m_tt.empty();
    }
  private:
    std::multimap<std::string, std::string> m_tt;
  };

  /*
      Holds and handles state transitions depending on a transition table if passed in
  */
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

  /*
      Multi-threaded container to run `state_handler`s in parallel
  */
  template <class Base>
  class async_container {
  public:
    async_container() {
      for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
        m_workers.push_back(std::thread([this, i] {
          while (true) {
            std::lock_guard<std::mutex> guard1(m_handlers_mutex);
            auto range = m_handlers.equal_range(i);
            for (auto it = range.first; it != range.second; ++it) {
              std::lock_guard<std::mutex> guard2(m_cout_mutex);
              std::cout << "Thread " << i << ": " << it->second << std::endl;
            }
          }
        }));
      }
    }

    ~async_container() {
      for (auto& it : m_workers) {
        if (it.joinable()) {
          it.join();
        }
      }
    }

    void add_handler(unsigned thread_id, state_handler<Base>* handler) {
      if (thread_id < m_workers.size())
        m_handlers.insert(std::make_pair(thread_id, handler));
      else
        throw std::out_of_range("thread_id must be between 0 and " + std::to_string(m_workers.size()));
    }
  private:
    std::mutex m_cout_mutex, m_handlers_mutex;
    std::condition_variable m_cond_var;
    std::vector<std::thread> m_workers;
    std::multimap<unsigned, state_handler<Base>*> m_handlers;
  };

}
