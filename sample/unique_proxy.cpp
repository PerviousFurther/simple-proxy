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

struct draw : prx::primitive_traits<draw, void(), void(int), int(const char*)>
{
  void operator()(auto const& impl) { impl.draw(); }
  void operator()(auto& impl, int time) { impl.draw(time); }
  int operator()(auto& impl, const char* str) { return impl.draw(str); }
};
struct dance
{
  void operator()(auto& value) { value.dance(); }
};

struct artist
{
  inline void draw() const
  {
    ::std::cout << "hello world in draw." << ::std::endl;
  }
  inline void draw(int time)
  {
    ::std::cout << "hello world in draw. in " << time << ::std::endl;
  }
  inline int draw(const char* string)
  {
    ::std::cout << "hello world. with " << ::std::string_view{ string }
                << ::std::endl;
    return 0;
  }
  inline void dance() { ::std::cout << "hello world in dance."; }
};

struct dynamic_deleter
{
  template<typename T>
  static void do_delete(void* ptr)
  {
    ::std::destroy_at<T>(static_cast<T*>(ptr));
    delete static_cast<T*>(ptr);
  }

  constexpr dynamic_deleter() = default;

  template<typename T>
  dynamic_deleter(prx::inplace<T>)
    : fn_{ &do_delete<T> }
  {
  }

  inline auto operator()(void* ptr) { fn_(ptr); }

private:
  void (*fn_)(void*){ nullptr };
};

template<typename T>
using unique_ptr = ::std::unique_ptr<T, dynamic_deleter>;

template<typename T>
struct prx::make_t<unique_ptr<T>>
{
  template<typename... Args>
  inline constexpr unique_ptr<T> operator()(Args&&... args) const
  {
    return { new T{ ::std::forward<Args>(args)... },
             dynamic_deleter{ prx::inplace<T>{} } };
  }
};

int
main()
{
  prx::proxy<unique_ptr,
             prx::entity<draw, prx::primitive_traits<dance, void()>>>
    proxy{};
  proxy.construct<artist>();
  proxy.invoke<draw>();
  proxy.invoke<draw>(6);
  ::std::cout << "some say: " << proxy.invoke<draw>("having value of:")
              << ::std::endl;
  proxy.invoke<dance>();
}