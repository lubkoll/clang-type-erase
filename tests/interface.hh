// This file was automatically generated using clang-type-erase.
// Please do not modify.

#pragma once

#include <gen/basic/SmartPointerStorage.h>

#include <memory>
#include <type_traits>

namespace Basic {
/// @brief class Fooable
class Fooable {
  struct Interface {
    virtual ~Interface() = default;
    virtual std::unique_ptr<Interface> clone() const = 0;
    virtual int foo() const = 0;
    virtual void set_value(int value) = 0;
  };

  template <class Impl> struct Wrapper : Interface {
    template <class T> Wrapper(T &&t) : impl(std::forward<T>(t)) {}

    std::unique_ptr<Interface> clone() const {
      return std::make_unique<Wrapper<Impl>>(impl);
    }

    int foo() const override { return impl.foo(); }

    void set_value(int value) override { impl.set_value(std::move(value)); }

    Impl impl;
  };

  template <class Impl>
  struct Wrapper<std::reference_wrapper<Impl>> : Wrapper<Impl &> {
    template <class T> Wrapper(T &&t) : Wrapper<Impl &>(std::forward<T>(t)) {}
  };

public:
  /// void type
  typedef void void_type;
  using type = int;
  static const int static_value = 1;

  Fooable() noexcept = default;

  template <class T> Fooable(T &&value) : impl_(std::forward<T>(value)) {}

  /// Does something.
  int foo() const { return impl_->foo(); }

  /// Retrieves something else.
  void set_value(int value) { impl_->set_value(std::move(value)); }

  template <class T> Fooable &operator=(T &&value) {
    return *this = Fooable(std::forward<T>(value));
  }

  explicit operator bool() const noexcept { return bool(impl_); }

  template <class T> T *target() noexcept {
    auto wrapper = dynamic_cast<Wrapper<T> *>(impl_.get());
    if (wrapper)
      return &wrapper->impl;
    return nullptr;
  }

  template <class T> const T *target() const noexcept {
    auto wrapper = dynamic_cast<const Wrapper<T> *>(impl_.get());
    if (wrapper)
      return &wrapper->impl;
    return nullptr;
  }

private:
  clang::type_erasure::polymorphic::Storage<Interface, Wrapper> impl_;
};
}
