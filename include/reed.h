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

	template <class T>
	using ResultOf = decltype(std::declval<T>()(string_view{}));

	template <class T>
	static constexpr bool simple = std::is_same_v<int, reed::ResultOf<T>>;


	struct Mismatch
	{
		constexpr operator int() const { return -1; }
	};
	static constexpr Mismatch mismatch = {};

	struct Empty
	{
		friend constexpr bool operator==(int length, Empty) { return length <= 0; }
		friend constexpr bool operator==(Empty, int length) { return length <= 0; }
	};
	static constexpr Empty empty = {};

	inline int max(int a, int b) { return a < b ? b : a; }
	inline int length(int a) { return a; }

	template <char... Chars>
	struct Ch
	{
		int operator()(string_view in) const
		{
			if (in.empty())
				return mismatch;
			return (... || (Chars == in.front())) ? 1 : mismatch;
		}
	};

	template <char... Chars>
	static constexpr Ch<Chars...> ch = {};

	template <char First, char Last>
	struct Chr
	{
		int operator()(string_view in) const
		{
			return (!in.empty() && ((First <= in.front()) & (in.front() <= Last))) ? 1 : mismatch;
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
				return mismatch;
			return (... && (in.front() == Chars && (in.remove_prefix(1), true))) ? 
				sizeof...(Chars) : mismatch;
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
				string.size() : mismatch;
		}
	};

	constexpr Str str(string_view string) { return { string }; }


	template <class T> 
	struct AtLeast : private T
	{
		int min;

		constexpr AtLeast(T sub, int min) : T(std::move(sub)), min(min) { }

		auto operator()(string_view in) const
		{
			ResultOf<T> result = 0;
			for (int c = 0; ; ++c)
			{
				auto subres = T::operator()(in);
				if (subres == empty)
					return c < min ? mismatch : result;
				in.remove_prefix(subres);
				result += std::move(subres);
			}
		}
	};

	template <class T, class = ResultOf<T>>
	constexpr AtLeast<T> operator+(int min, T expr) { return { expr, min }; }

	template <class T>
	struct AnyNumber : private T
	{
		constexpr AnyNumber(T sub) : T(std::move(sub)) { }

		auto operator()(string_view in) const
		{
			reed::ResultOf<T> result = 0;
			for (;;)
			{
				const auto subres = T::operator()(in);
				if (subres == empty)
					return result;
				in.remove_prefix(length(subres));
				result += std::move(subres);
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
			ResultOf<T> result = 0;
			result += T::operator()(in);
			if (result == empty)
				return result;
			in.remove_prefix(result);
			for (;;)
			{
				auto subres = T::operator()(in);
				if (subres == empty)
					return result;
				in.remove_prefix(length(subres));
				result += std::move(subres);
			}
		}
	};

	template <class T, class = ResultOf<T>>
	constexpr AtLeastOne<T> operator+(T expr) { return { std::move(expr) }; }

	template <class First, class Then>
	struct Seq : private First, private Then
	{
		constexpr Seq(First first, Then then) : First(std::move(first)), Then(std::move(then)) { }

		decltype(std::declval<ResultOf<First>>() + std::declval<ResultOf<Then>>())
			operator()(string_view in) const
		{
			auto firstres = First::operator()(in);
			if (firstres == mismatch)
				return mismatch;
			in.remove_prefix(length(firstres));
			auto thenres = Then::operator()(in);
			if (thenres == mismatch)
				return mismatch;
			return std::move(firstres) + std::move(thenres);
		}
	};

	template <class A, class B>
	constexpr Seq<A, B> operator&(A a, B b) { return { a, b }; }

	template <class A, class B>
	struct Branch : private A, private B
	{
		constexpr Branch(A a, B b) : A(std::move(a)), B(std::move(b)) { }

		auto operator()(string_view in) const
		{
			return max(A::operator()(in), B::operator()(in));
		}
	};

	template <class A, class B>
	constexpr Branch<A, B> operator|(A a, B b) { return { a, b }; }

	template <class T>
	struct Maybe : private T
	{
		constexpr Maybe(T sub) : T(std::move(sub)) { }

		auto operator()(string_view in) const
		{
			auto res = T::operator()(in);
			if (res == mismatch)
				res = 0;
			return res;
		}
	};
	template <class T>
	constexpr Maybe<T> maybe(T sub) { return { sub }; }

	template <class Item, class Sep>
	struct Split : private Item, private Sep
	{
		constexpr Split(Item item, Sep sep) : Item(std::move(item)), Sep(std::move(sep)) { }

		auto operator()(string_view in) const
		{
			std::conditional_t<simple<Item>, reed::ResultOf<Sep>, reed::ResultOf<Item>> result = 0;
			result += Item::operator()(in);
			if (result == mismatch)
				return result;
			in.remove_prefix(length(result));
			for (;;)
			{
				auto sepres = Sep::operator()(in);
				if (sepres == mismatch)
					return result;
				auto subres = Item::operator()(in.substr(length(sepres)));
				if (subres == mismatch)
					return result;
				if (sepres == empty && subres == empty)
					return result;

				in.remove_prefix(length(sepres) + length(subres));

				result += std::move(sepres);
				result += std::move(subres);
			}
		}
	};

	template <class Item, class Sep>
	constexpr Split<Item, Sep> operator%(Item item, Sep sep) { return { std::move(item), std::move(sep) }; }

	class Rule
	{
	public:
		struct Impl;
		using PImpl = std::unique_ptr<Impl>;
		class Result
		{
			friend class Rule;
			std::shared_ptr<PImpl> _rule;
		public:
			int length = mismatch;
			std::string literal;
			std::vector<Result> parts;

			Result() = default;
			Result(int length) : length(length) { }
			Result(Mismatch) : length(mismatch) { }

			const Rule& rule() const;

			friend bool operator==(const Result& r, Mismatch) { return r.length == mismatch; }
			friend bool operator==(Mismatch, const Result& r) { return r.length == mismatch; }

			friend bool operator==(const Result& r, Empty) { return r.length == empty; }
			friend bool operator==(Empty, const Result& r) { return r.length == empty; }

			void operator+=(int len) { if (length == mismatch) length = 0; length += len; }
			void operator+=(Result&& part)
			{
				*this += part.length;
				parts.emplace_back(std::move(part));
			}

			friend Result operator+(Result&& a, int b)
			{
				a.length += b;
				return std::move(a);
			}
			friend Result operator+(int a, Result&& b)
			{
				b.length += a;
				return std::move(b);
			}
			friend Result operator+(Result&& a, Result&& b)
			{
				if (a == empty) return std::move(b);
				if (b == empty) return std::move(a);
				Result result;
				result += std::move(a);
				result += std::move(b);
				return result;
			}
			friend Result max(Result&& a, int b)
			{
				if (a.length >= b)
					return std::move(a);
				return { b };
			}
			friend Result max(int a, Result&& b)
			{
				if (a >= b.length)
					return { a };
				return std::move(b);
			}
			friend Result max(Result&& a, Result&& b)
			{
				return (a.length >= b.length) ? std::move(a) : std::move(b);
			}

			friend int length(const Result& a) { return a.length; }
		};
		struct Impl
		{
			virtual ~Impl() = default;

			virtual Result apply(string_view in) const = 0;
		};

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

				Result apply(string_view in) const final 
				{
					auto expres = expr(in);
					if constexpr (std::is_same_v<int, decltype(expres)>)
					{
						Result result(expres);
						if (result.length > 0)
							result.literal = std::string(in.substr(0, length(expres)));
						return result;
					}
					else 
						return expres; 
				}
			};

			*_impl = std::make_unique<TImpl>(std::move(e));

			return *this;
		}

		Result operator()(string_view in) const
		{
			auto result = (_impl && *_impl) ? (*_impl)->apply(in) : Result{};
			result._rule = _impl;
			return result;
		}

		friend bool operator==(const Rule& a, decltype(none)) { return a._impl == nullptr; }
		friend bool operator==(decltype(none), const Rule& a) { return a._impl == nullptr; }

		friend bool operator!=(const Rule& a, decltype(none)) { return a._impl != nullptr; }
		friend bool operator!=(decltype(none), const Rule& a) { return a._impl != nullptr; }
	};
}
#pragma warning(pop)
