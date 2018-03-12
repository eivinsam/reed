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
		constexpr E(Args&&... args) : T(std::forward<Args>(args)...) { }
	};

	struct Ch
	{
		char first;
		char last;

		constexpr Ch(char first, char last) : first(first), last(last) { }

		int operator()(string_view in) const
		{
			if (!in.empty() && first <= in.front() && in.front() <= last)
				return 1;
			else
				return -1;
		}
	};

	constexpr E<Ch> ch(char first, char last) { return { first, last }; }


	template <class T> 
	struct AtLeast
	{
		T sub;
		int min;

		constexpr AtLeast(T sub, int min) : sub(sub), min(min) { }

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
}
