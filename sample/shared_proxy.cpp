//////
////
///
// To compile this, set -std=c++20 or /std:c++20.
//
///
////

#include "proxy.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

struct work : prx::primitive_traits<work, void()>
{
  inline auto operator()(auto& worker) { worker.work(); }
};

struct diligent_worker
{
  ~diligent_worker() { ::std::cout << "eager worker get off." << ::std::endl; }

  void work() const
  {
    using namespace ::std::chrono_literals;
    ::std::cout << "Start to do work." << ::std::endl;
    ::std::this_thread::sleep_for(0.1s);
    ::std::cout << "do work 1 hours..." << ::std::endl;
  }
};

struct lazy_worker
{
  ~lazy_worker() { ::std::cout << "lazy worker get off." << ::std::endl; }

  void work() const
  {
    using namespace ::std::chrono_literals;
    ::std::cout << "Start to do work." << ::std::endl;
    ::std::this_thread::sleep_for(0.013s);
    ::std::cout << "do work 10 minute..." << ::std::endl;
    ::std::this_thread::sleep_for(0.087s);
    ::std::cout << "loaf 50 minute..." << ::std::endl;
    ::std::cout << "end do work." << ::std::endl;
  }
};

template<typename T>
struct weak_ptr : ::std::weak_ptr<T>
{
  using base = ::std::weak_ptr<T>;
  using base::base;

  inline auto get() const { return base::lock().get(); }
};

template<typename T>
struct prx::make_t<::std::shared_ptr<T>>
{
  template<typename... Args>
  inline constexpr auto operator()(Args&&... args) const
  {
    return ::std::make_shared<T>(::std::forward<Args>(args)...);
  }
};

int
main()
{
  prx::proxy<::std::shared_ptr, prx::entity<work>> workers[4];
  {
    auto dw{ ::std::make_shared<diligent_worker>() };
    auto lw{ ::std::make_shared<lazy_worker>() };
    dw->work();
    lw->work();
    workers[0] = dw;
    workers[1] = lw;
  }
  {
    prx::proxy<::weak_ptr, prx::entity<work>> weak_worker{ workers[0] };
    weak_worker.invoke<work>();
    workers[1].invoke<work>();
  }
  return 0;
}