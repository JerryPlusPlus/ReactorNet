#ifndef REACTORNET_BASE_ANY_H
#define REACTORNET_BASE_ANY_H
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>
namespace reactor {
// Value-type context storage for C++11; std::any requires C++17.
class Any {
  struct Value {
    virtual ~Value() = default;
    virtual std::unique_ptr<Value> clone() const = 0;
    virtual const std::type_info& type() const noexcept = 0;
  };
  template<typename T> struct Holder : Value {
    template<typename U> explicit Holder(U&& value) : data(std::forward<U>(value)) {}
    std::unique_ptr<Value> clone() const override { return std::unique_ptr<Value>(new Holder(data)); }
    const std::type_info& type() const noexcept override { return typeid(T); }
    T data;
  };
 public:
  Any() = default;
  Any(const Any& other) : m_value(other.m_value ? other.m_value->clone() : nullptr) {}
  Any(Any&&) noexcept = default;
  template<typename T, typename D = typename std::decay<T>::type,
           typename std::enable_if<!std::is_same<D, Any>::value, int>::type = 0>
  Any(T&& value) : m_value(new Holder<D>(std::forward<T>(value))) {
    static_assert(std::is_copy_constructible<D>::value, "context values must be copyable");
  }
  Any& operator=(Any other) noexcept { swap(other); return *this; }
  void swap(Any& other) noexcept { m_value.swap(other.m_value); }
  bool empty() const noexcept { return !m_value; }
  void clear() noexcept { m_value.reset(); }
  const std::type_info& type() const noexcept { return m_value ? m_value->type() : typeid(void); }
  template<typename T> T* get() noexcept {
    using U = typename std::remove_cv<T>::type;
    return type() == typeid(U) ? &static_cast<Holder<U>*>(m_value.get())->data : nullptr;
  }
  template<typename T> const T* get() const noexcept {
    using U = typename std::remove_cv<T>::type;
    return type() == typeid(U) ? &static_cast<const Holder<U>*>(m_value.get())->data : nullptr;
  }
 private:
  std::unique_ptr<Value> m_value;
};
template<typename T> T* any_cast(Any* value) noexcept { return value ? value->get<T>() : nullptr; }
template<typename T> const T* any_cast(const Any* value) noexcept { return value ? value->get<T>() : nullptr; }
template<typename T> T any_cast(Any& value) {
  using U = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  U* result = value.get<U>();
  if (!result) throw std::bad_cast();
  return static_cast<T>(*result);
}
template<typename T> T any_cast(const Any& value) {
  using U = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  const U* result = value.get<U>();
  if (!result) throw std::bad_cast();
  return static_cast<T>(*result);
}
} // namespace reactor
#endif
