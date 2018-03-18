#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>

#pragma warning(push)
#pragma warning(disable: 4584)

namespace reed
{
	using std::string_view;

	template <char... Chars>
	struct Ch
	{
		int operator()(string_view in) const
		{
			if (in.empty())
				return -1;
			return (... || (Chars == in.front())) ? 1 : -1;
		}
	};

	template <char... Chars>
	static constexpr Ch<Chars...> ch = {};

	template <char First, char Last>
	struct Chr
	{
		int operator()(string_view in) const
		{
			return (!in.empty() && ((First <= in.front()) & (in.front() <= Last))) ? 1 : -1;
		}
	};

	template <char First, char Last>
	static constexpr Chr<First, Last> chr = {};

	template <char... Chars>
	struct Chs
	{
		int operator()(string_view in) const
		{
			if (in.size() < sizeof...(Chars))
				return false;
			return (... && (in.front() == Chars && (in.remove_prefix(1), true))) ? sizeof...(Chars) : -1;
		}
	private:
		static bool _check_one(string_view& in, char ch)
		{
			if (in.front() != ch)
				return false;
			in.remove_prefix(1);
			return true;
		}
	};

	template <char... Chars>
	static constexpr Chs<Chars...> chs = {};

	struct Str
	{
		string_view string;

		int operator()(string_view in) const
		{
			return in.size() >= string.size() && 
				in.substr(0, string.size()) == string ? 
				string.size() : -1;
		}
	};

	constexpr Str str(string_view string) { return { string }; }


	template <class T> 
	struct AtLeast : private T
	{
		int min;

		constexpr AtLeast(T sub, int min) : T(std::move(sub)), min(min) { }

		int operator()(string_view in) const
		{
			int result = 0;
			for (int c = 0; ; ++c)
			{
				const int subres = T::operator()(in);
				if (subres <= 0)
					return c < min ? -1 : result;
				result += subres;
				in.remove_prefix(subres);
			}
		}
	};

	template <class T>
	constexpr AtLeast<T> operator+(int min, T expr) { return { expr, min }; }

	template <class T>
	struct AnyNumber : private T
	{
		constexpr AnyNumber(T sub) : T(std::move(sub)) { }

		int operator()(string_view in) const
		{
			int result = 0;
			for (;;)
			{
				const auto subres = T::operator()(in);
				if (subres <= 0)
					return result;
				result += subres;
				in.remove_prefix(subres);
			}
		}
	};

	template <class T>
	constexpr AnyNumber<T> operator*(T expr) { return { std::move(expr) }; }

	template <class T>
	struct AtLeastOne : private T
	{
		constexpr AtLeastOne(T sub) : T(std::move(sub)) { }

		int operator()(string_view in) const
		{
			int result = T::operator()(in);
			if (result <= 0)
				return result;
			in.remove_prefix(result);
			for (;;)
			{
				const auto subres = T::operator()(in);
				if (subres <= 0)
					return result;
				result += subres;
				in.remove_prefix(subres);
			}
		}
	};

	template <class T>
	constexpr AtLeastOne<T> operator+(T expr) { return { std::move(expr) }; }

	template <class First, class Then>
	struct Seq : private First, private Then
	{
		constexpr Seq(First first, Then then) : First(std::move(first)), Then(std::move(then)) { }

		int operator()(string_view in) const
		{
			const auto firstres = First::operator()(in);
			if (firstres < 0)
				return -1;
			const auto thenres = Then::operator()(in.substr(firstres));
			if (thenres < 0)
				return -1;
			return firstres + thenres;
		}
	};

	template <class A, class B>
	constexpr Seq<A, B> operator&(A a, B b) { return { a, b }; }

	template <class A, class B>
	struct Branch : private A, private B
	{
		constexpr Branch(A a, B b) : A(std::move(a)), B(std::move(b)) { }

		int operator()(string_view in) const
		{
			const auto ares = A::operator()(in);
			const auto bres = B::operator()(in);
			return ares > bres ? ares : bres;
		}
	};

	template <class A, class B>
	constexpr Branch<A, B> operator|(A a, B b) { return { a, b }; }

	template <class T>
	struct Maybe : private T
	{
		constexpr Maybe(T sub) : T(std::move(sub)) { }

		int operator()(string_view in) const
		{
			const auto res = T::operator()(in);
			return res < 0 ? 0 : res;
		}
	};
	template <class T>
	constexpr Maybe<T> maybe(T sub) { return { sub }; }

	template <class Item, class Sep>
	struct Split : private Item, private Sep
	{
		constexpr Split(Item item, Sep sep) : Item(std::move(item)), Sep(std::move(sep)) { }

		int operator()(string_view in) const
		{
			int result = Item::operator()(in);
			if (result < 0)
				return -1;
			in.remove_prefix(result);
			for (;;)
			{
				const auto sepres = Sep::operator()(in);
				if (sepres < 0)
					return result;
				in.remove_prefix(sepres);
				const auto subres = Item::operator()(in);
				if (subres < 0)
					return result;
				in.remove_prefix(subres);
				if (sepres + subres == 0)
					return result;
				result += sepres + subres;
			}
		}
	};

	template <class Item, class Sep>
	constexpr Split<Item, Sep> operator%(Item item, Sep sep) { return { std::move(item), std::move(sep) }; }

	class Rule
	{
		struct Impl
		{
			virtual ~Impl() = default;

			virtual int apply(string_view in) const = 0;
		};

		using PImpl = std::unique_ptr<Impl>;

		std::shared_ptr<PImpl> _impl;
	public:

		static constexpr struct { } none = {};

		constexpr Rule(decltype(none)) { };

		Rule() : _impl(std::make_shared<PImpl>()) { }

		template <class T>
		Rule(T e) : Rule() { *this = std::move(e); }

		template <class T>
		Rule& operator=(T e)
		{
			struct TImpl : Impl
			{
				T expr;

				constexpr TImpl(T&& expr) : expr(std::move(expr)) { }

				int apply(string_view in) const final { return expr(in); }
			};

			*_impl = std::make_unique<TImpl>(std::move(e));

			return *this;
		}

		int operator()(string_view in) const { return (_impl && *_impl) ? (*_impl)->apply(in) : -1; }

		friend bool operator==(const Rule& a, decltype(none)) { return a._impl == nullptr; }
		friend bool operator==(decltype(none), const Rule& a) { return a._impl == nullptr; }

		friend bool operator!=(const Rule& a, decltype(none)) { return a._impl != nullptr; }
		friend bool operator!=(decltype(none), const Rule& a) { return a._impl != nullptr; }
	};
}
#pragma warning(pop)
