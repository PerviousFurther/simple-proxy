# Simple-Proxy
A simple rewheeling of [proxy](https://github.com/microsoft/proxy).
This implmentation is use for commuication only, and have more **extensible**
oppotunities but with a little limitation.
# Concept
A few concept in this library. 
- `primitive` an struct contain few function signature
which require the entity to implment. Simliar to *one function* 
in interface definition.
- `entity` series of `primitives` which is an abstract concept of implmentation.
- `proxy` an object managing the life time of implmentation and 
invocation of each `primitive` in `entity`.
# Usage
Example below is from [this file](sample/unique_proxy.cpp).
## Define a primitive
An simple draw can be describe as below:
```cpp
struct draw 
  : prx::primitive_traits<
    draw
  , void()
  , void(int)
  , int(char *)>
{
  void operator()(auto const &impl) 
   { impl.draw(); }
  void operator()(auto &impl, int time) 
   { impl.draw(time); }
  int operator()(auto &impl, char *time) 
   { return impl.draw(time); }
};
```

- `prx::primitive_traits<...>` make primitive,
first parameter `draw` decide the name of whole primitive *(help library to select which primitive should be invoke)*.
- `void(), void(int)...` at `primitive_traits` is the function signature that 
need entity to impl *(only constraint on fucntion's type)*.
- `operator()` transfer impl to invoke concrete named member function *(constraint on function's name.)*. 
So reserve an template parameter `impl` is necessary.

Inheritance `primitive_traits` from is not required in this implmentation. However, the `struct draw` and `operation()` is **required**.

OFF TOPIC: *(It will probably becoming unneccessary when using static reflection... 
however this should be the only way currently).*

**LIMITATION**: the value is always transfer by `T&`*(lvalue)*. thus you **cannot** declare like this:
```cpp
struct draw
{
  void operator()(auto &impl);
  void operator()(auto const &impl); // will never be invoked.
};
```

## Define a entity

```cpp
...
// as supplement of last step.
struct dance
{
  void operator(auto &impl) { impl.dance(); }
};

prx::entity<draw, prx::primitive_traits<dance, void()>>
...
```
- `prx::entity<...>` just push all primitive you need in it. All 
template parameter should meet `prx::primitive`'s requirement 
*(Or, must be `primitive_traits`).*

## Define a proxy

Before using concrete proxy, 
we need to make an simple smart ptr to managing the life time of proxied object.

We use `std::unique_ptr<...>` as example using following persudo-code:

```cpp
...
struct dynamic_deleter /*omitted*/;
template<typename T>
using unique_ptr = ::std::unique_ptr<T, dynamic_deleter>;

template<typename T>
struct prx::make_t<unique_ptr<T>>
{
  unique_ptr<T> operator()(auto&&...args) const
  { return { new T{ forward(args)...}, dynamic_deleter{...} }; }
};

```
- `::unique_ptr<T>` is declare for proxy. 
Since proxy can only support one template argument's class template.
and `dyanmic_deleter` is use for different implmentation since `prx::proxy` **CANNOT**
offer *dynamically meta information*.
- `prx::make_t` is define to help `proxy` make different `::unique_ptr<T>`.
It is a full-specialization class template.

Now we can use `proxy` as:
```cpp
...
prx::proxy<::unique_ptr, entity<...>> proxy{};
```
The `::unqiue_ptr` is fill with `void` inside proxy, 
which is also why we use our own `::unique_ptr` but not `std::unique_ptr`.

## Combine proxy with implmentation

Here comes a simple implmentation.

```cpp
struct artist
{
  void draw() const;
  void draw(int val);
  int draw(const char *string);
};
```
We can use:
```cpp
proxy<::unique_ptr, entity<draw>> prx_artist{};
proxy.construct<artist>();
```
or
```cpp
proxy<::unique_ptr, entity<draw>> prx_artist{prx::inplace<artist>{}};
```
to let `artist` to be proxied.

And we can now invoke function by:
```cpp
proxy.invoke<draw>();  // declval<artist&>().draw();
proxy.invoke<draw>(5); // declval<artist&>().draw(5);
auto v = proxy.invoke<draw>("..."); // v = declval<artist&>().draw("...");
```
