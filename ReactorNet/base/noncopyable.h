#ifndef REACTOR_BASE_NONCOPYABLE_H
#define REACTOR_BASE_NONCOPYABLE_H

namespace reactor
{
	
	// 不可拷贝，但可以移动
	class noncopyable
	{
		 public:
		  noncopyable(const noncopyable&) = delete;
		  const noncopyable& operator=(const noncopyable&) = delete;

		  // C++ 规则：如果类定义了拷贝构造或拷贝赋值，编译器不会自动生成移动操作。
		  noncopyable(noncopyable&&) = default;
		  noncopyable& operator=(noncopyable&&) = default;

		 protected:
		  noncopyable() = default;
		  ~noncopyable() = default;
	};

	// 不可拷贝，同时不可移动
	class nonmoveable
	{
	public:
		nonmoveable(const nonmoveable&) = delete;
		const nonmoveable& operator=(const nonmoveable&) = delete;

		nonmoveable(nonmoveable&&) = delete;
		nonmoveable& operator=(nonmoveable&&) = delete;

	protected:
		nonmoveable() = default;
		~nonmoveable() = default;
	};

}  // namespace reactor

#endif  // REACTOR_BASE_NONCOPYABLE_H
