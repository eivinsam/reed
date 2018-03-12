#pragma once

#include <string>
#include <string_view>
#include <optional>



namespace reed
{
	using std::string_view;
	using std::optional;

	template <class T>
	struct E : public T
	{
		template <class... Args>
		constexpr E(Args&&... args) : T{ std::forward<Args>(args)... } { }
	};

	struct Ch
	{
		char first;
		char last;

		int operator()(string_view in) const
		{
			return (!in.empty() && ((first <= in.front()) & (in.front() <= last))) ? 1 : -1;
		}
	};

	constexpr E<Ch> ch(char c) { return { c, c }; }
	constexpr E<Ch> ch(char first, char last) { return { first, last }; }

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

	constexpr E<Str> str(string_view string) { return { string }; }


	template <class T> 
	struct AtLeast
	{
		T sub;
		int min;

		int operator()(string_view in) const
		{
			int result = 0;
			for (int c = 0; ; ++c)
			{
				const int subres = sub(in);
				if (subres <= 0)
					return c < min ? -1 : result;
				result += subres;
				in.remove_prefix(subres);
			}
		}
	};

	template <class T>
	constexpr E<AtLeast<T>> operator+(int min, E<T> expr) { return { expr, min }; }

	template <class First, class Then>
	struct Seq
	{
		First first;
		Then  then;

		int operator()(string_view in) const
		{
			const auto firstres = first(in);
			if (firstres < 0)
				return -1;
			const auto thenres = then(in.substr(firstres));
			if (thenres < 0)
				return -1;
			return firstres + thenres;
		}
	};

	template <class A, class B>
	constexpr E<Seq<A, B>> operator&(E<A> a, E<B> b) { return { a, b }; }

	template <class A, class B>
	struct Branch
	{
		A a;
		B b;

		int operator()(string_view in) const
		{
			const auto ares = a(in);
			const auto bres = b(in);
			return ares > bres ? ares : bres;
		}
	};

	template <class A, class B>
	constexpr E<Branch<A, B>> operator|(E<A> a, E<B> b) { return { a, b }; }

	template <class T>
	struct Maybe
	{
		T sub;

		int operator()(string_view in) const
		{
			const auto res = sub(in);
			return res < 0 ? 0 : res;
		}
	};
	template <class T>
	constexpr E<Maybe<T>> maybe(E<T> sub) { return { sub }; }

}
