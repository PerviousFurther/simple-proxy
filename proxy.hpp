#if !defined(PR_X_HEADER_)
#define PR_X_HEADER_

#if !defined(PR_X_EXPORT_MODULE)
#	include <type_traits>
#	include <concepts>
#	include <cstdint>
#	include <limits>
#	include <array>
#	define PR_X_EXPORT
#else
#	define PR_X_EXPORT export
#endif

#pragma region proxy
namespace prx
{
	template<typename...>
	struct types;

	struct bad_proxy_call{};

	namespace meta
	{
		template<typename...>
		struct find;
	}

	namespace meta
	{
		template<::std::size_t value>
		using csize = ::std::integral_constant<::std::size_t, value>;

		struct by_index;
		template<::std::integral T, T value, typename F, typename...Ts, template<typename...>typename Tl>
			requires(value != 0)
		struct find<by_index, ::std::integral_constant<T, value>, Tl<F, Ts...>>
		{
			using type = typename find<by_index, ::std::integral_constant<T, value - 1>, Tl<Ts...>>::type;
		};
		template<::std::integral T, typename F, typename...Ts, template<typename...>typename Tl>
		struct find<by_index, ::std::integral_constant<T, 0>, Tl<F, Ts...>>
		{
			using type = F;
		};
		template<::std::integral T, T value, template<typename...>typename Tl>
		struct find<by_index, ::std::integral_constant<T, value>, Tl<>>
		{
			using type = void;
		};
	}

	template<::std::size_t index, typename...Ts>
	using element_t = typename meta::find<meta::by_index, meta::csize<index>, types<Ts...>>::type;
	template<::std::size_t index, typename T>
	using element_at = typename meta::find<meta::by_index, meta::csize<index>, T>::type;

	namespace meta
	{
		struct first_of;

		constexpr auto size_max{(::std::numeric_limits<::std::size_t>::max)()};

		template<typename T, ::std::size_t increment>
		struct add
		{
			// if value is not an member of T. that means out of ranges.
			static constexpr auto value{T::value == size_max ? T::value : T::value + increment}; 
		};
		template<typename Tf, template<typename, typename>typename Pred, 
			template<typename...>typename Tl, typename F, typename...Rs>
		struct find<first_of, Pred<Tf, void>, Tl<F, Rs...>>
		{
			static constexpr auto result {Pred<Tf, F>::value};
			using next 
				= find<first_of, Pred<Tf, void>, Tl<Rs...>>;
			using conditional = 
				typename::std::conditional_t<result, ::std::type_identity<F>, next>;

			using type = typename conditional::type;
			static constexpr auto is_contain{!::std::is_void_v<typename conditional::type>};
			static constexpr auto value     {::std::conditional_t<result, ::std::integral_constant<::std::size_t, 0u>, add<next, 1u>>::value};
		};

		template<typename Tf, template<typename, typename>typename Pred, 
			template<typename...>typename Tl>
		struct find<first_of, Pred<Tf, void>, Tl<>>
		{
			using type = void;
			static constexpr auto value{size_max};
		};

		struct most_of;
		template<template<typename, typename>typename Pred, 
			typename Mst, 
			template<typename...>typename Tl, typename F, typename...Rs>
		struct find<most_of, Pred<Mst, void>, Tl<F, Rs...>>
		{
			static constexpr auto result{Pred<Mst, F>::value};
			using next = find<most_of, ::std::conditional_t<result, Pred<F, void>, Pred<Mst, void>>, Tl<Rs...>>;
			using type = typename next::type;
		};
		template<template<typename, typename>typename Pred, 
			typename Mst, 
			template<typename...>typename Tl>
		struct find<most_of, Pred<Mst, void>, Tl<>>
		{
			using type = Mst;
		};
	}

	template<typename TF, typename...Ts>
	concept presented_in = meta::find<meta::first_of, ::std::is_same<TF, void>, types<Ts...>>::is_contain;
	template<typename TF, typename T>
	concept presented_at = meta::find<meta::first_of, ::std::is_same<TF, void>, T>::is_contain;
	template<typename TF, typename...Ts>
	inline constexpr auto index_of = meta::find<meta::first_of, ::std::is_same<TF, void>, types<Ts...>>::value;
	template<typename TF, template<typename, typename>typename Pred, typename...Ts>
	using first_required_t = typename meta::find<meta::first_of, Pred<TF, void>, types<Ts...>>::type;
}

namespace prx
{
	template<typename T>
	struct is_primitive_signature : ::std::false_type {};
	template<typename R, typename...Args>
	struct is_primitive_signature<R(Args...)> : ::std::true_type {};
	template<typename T>
	concept primitive_signature = is_primitive_signature<T>::value;

	template<typename C, typename...>
	struct primitive_traits;

	template<typename B, typename R, typename...Ps>
	struct primitive_traits<B, R(Ps...)>
	{
		// return biggest_struct.
		using result = R;
		// describe inovke biggest_struct.
		using type = R(Ps...);
		// for table receive invoke.
		using invoke_t = R(*)(void *, Ps...);

		// describe num of types.
		static constexpr auto size{1u};
		// pure biggest_struct
		using name_t = B;

		template<typename I>
		static constexpr auto represented() { return::std::invocable<B, I, Ps...>; };
	};

	template<typename B, primitive_signature...Fs>
		requires(sizeof...(Fs) > 1u)
	struct primitive_traits<B, Fs...> : primitive_traits<B, Fs>...
	{
		template<primitive_signature F>
		using single = primitive_traits<B, F>; // make index name easiler.
		// describe num of types.
		static constexpr auto size{sizeof...(Fs)};
		// types.
		using types = ::std::tuple<typename single<Fs>::type...>;

		// describe implementation should be const or volatile.
		template<::std::size_t index>
			requires(index < size) // out of range.
		using index_t = element_t<index, typename single<Fs>::type...>;

		template<::std::size_t index>
			requires(index < size) // out of range.
		using invoke_t = element_t<index, typename single<Fs>::invoke_t...>;

		using name_t = B;

		template<typename H>
		static consteval auto represented() { return ((single<Fs>::template represented<H>()) && ...); };
	};

	template<typename T>
	concept primitive = ::std::is_trivial_v<T> && requires{ T::size; };

	template<typename P>
	concept simple_primitive = primitive<P> && (P::size == 1u);
	template<typename P>
	concept overloaded_primitive = primitive<P> && (P::size > 1u);

	template<typename T>
	struct inplace {};

	inline constexpr struct no_return_t final {} no_return{};

	template<primitive...Rs>
	struct primitive_map
	{
		using this_type = primitive_map<Rs...>;

		static constexpr auto size{sizeof...(Rs)};

	private:
		// make static function table.
		template<typename T>
			requires(sizeof...(Rs) >= 1)
		struct table_storage
		{
		#if defined(__GNUC__) 
			// gcc do not allow function pointer cast to void* on compile time.
		#define PR_X_CONSTEVAL_
		#define PR_X_CONSTEXPR_
		#else
		#define PR_X_CONSTEVAL_ consteval
		#define PR_X_CONSTEXPR_ constexpr
		#endif

			template<typename F, typename R, typename...Ps>
			static auto do_invoke(void *raw, Ps...args) 
			{
				auto data{reinterpret_cast<T *>(raw)};
				if constexpr (::std::is_void_v<R>)
				{
					typename F::name_t{}(*data, ::std::forward<Ps>(args)...);
					return no_return;
				} else 
					return static_cast<R>(typename F::name_t{}(*data, ::std::forward<Ps>(args)...));
			}

			template<typename, typename>
			struct single_impl; // we assume every F have them function biggest_struct.
			template<typename F, typename R, typename...Ps>
			struct single_impl<F, R(Ps...)>
			{ inline static PR_X_CONSTEXPR_::std::conditional_t<::std::is_void_v<R>, no_return_t, R>(*value)(void *, Ps...){&do_invoke<F, R, Ps...>}; };

			template<::std::size_t size>
			using array_t = ::std::array<void *, size>;

			template<typename F, ::std::size_t index = 0>
			static PR_X_CONSTEVAL_ auto single()
			{ 
				if constexpr(overloaded_primitive<F>)
					return (void *const)single_impl<F, typename F::template index_t<index>>::value;
				else if constexpr (simple_primitive<F>)
					return (void *const)single_impl<F, typename F::type>::value;
				else 
					static_assert(::std::is_void_v<F> ? false : false, "Proxy error: not required.");
			}

			template<typename Tf, ::std::size_t size, ::std::size_t offset, ::std::size_t...oindices, ::std::size_t...indices>
			static PR_X_CONSTEVAL_ auto overloaded(const array_t<offset> prev, 
												   ::std::index_sequence<oindices...>, ::std::index_sequence<indices...>) 
			{ return array_t<size>{ prev.at(oindices)..., single<Tf, indices>()...}; }

			template<::std::size_t index = 0, ::std::size_t size = 0, ::std::size_t...indices>
			static PR_X_CONSTEVAL_ auto make(const array_t<size> prev = {},
											 ::std::index_sequence<indices...> = ::std::make_index_sequence<0>{})
			{
				if constexpr (index >= (sizeof...(Rs)))
					return prev;
				else {
					using this_f = element_t<index, Rs...>;
					constexpr auto new_size{size + this_f::size};
					// emplace new transfer function.
					auto current{overloaded<this_f, new_size, size>(prev, 
																	::std::make_index_sequence<size>{}, 
																	::std::make_index_sequence<this_f::size>{})};
					return make<index + 1>(current, ::std::make_index_sequence<new_size>{});
				}
			}

			inline static PR_X_CONSTEXPR_ array_t<((Rs::size) + ...)> table{make()};

		#undef PR_X_CONSTEXPR_
		#undef PR_X_CONSTEVAL_
		};

	public:
		constexpr primitive_map() = default;

		template<typename T>
		constexpr primitive_map(inplace<T> place)
			: table_{table_storage<T>::table.data()}
		{}

		~primitive_map() { reset(); }

	private:
		template<::std::size_t end, ::std::size_t index = 0u, ::std::size_t offset = 0u>
		static consteval auto located() noexcept 
		{
			if constexpr (index != end)
				return located<end, index + 1u, offset + element_t<index, Rs...>::size>(); 
			else 
				return offset;
		}

	public:
		template<typename F, ::std::size_t index = 0u, typename...Args>
		inline auto invoke(void *data, Args&&...args) const
		{
			constexpr auto location{located<index_of<F, Rs...>>()};
			if constexpr (simple_primitive<F>)
			{
				using type     = typename F::type;
				using invoke_t = typename F::invoke_t;
				if constexpr (::std::invocable<type, Args...>)
					return reinterpret_cast<invoke_t const>(table_[location])
					(data, ::std::forward<Args>(args)...);
				static_assert(::std::invocable<type, Args...>, "[PRX ERROR]: functionality not presented.");
			}
			else // overloaded.
			{
				if constexpr (index < F::size)
				{
					using invoke_t = typename F::template invoke_t<index>;
					using type     = typename F::template index_t<index>;
					if constexpr (::std::invocable<type, Args...>)
						return reinterpret_cast<invoke_t const>(table_[location + index])
						(data, ::std::forward<Args>(args)...);
					else 
						return this->invoke<F, index + 1u>(data, ::std::forward<Args>(args)...);
				} 
				static_assert(index != F::size, "[PRX ERROR]: functionality not presented.");
			} 
		}

		template<typename T>
		void rebind(){ table_ = table_storage<T>::table.data(); }
		void reset() { table_ = nullptr; }

	private:
		void *const *table_;
	};

	template<typename T, typename P>
	struct is_entity : ::std::false_type {};

	template<typename T, primitive...Fs>
	struct is_entity<T, primitive_map<Fs...>> 
	{ static constexpr auto value{ ((Fs::template represented<T>()) && ...) }; };

	template<typename T, typename...Fs>
	concept entity = ((primitive<Fs>) && ...) // all of requirment should be functionality.
		&& is_entity<T, primitive_map<Fs...>>::value; // must implment all of the function.
	template<typename T, typename Tb>
	concept entity_of = is_entity<T, Tb>::value;

	template<template<typename>typename PtrM, typename Pm>
	struct proxy
	{ static_assert(::std::is_void_v<Pm> ? false : false, "[PRX ERROR]: Not an valid primitive table."); };

	template<template<typename>typename Ptr>
	struct fancy_ptr
	{
		template<typename T>
		using apply = Ptr<T>;
	};

	template<typename T>
	struct make_t;

	template<template<typename>typename PtrM, template<typename...>typename PmT, primitive...Ps>
		requires::std::is_object_v<PtrM<void>>
	struct proxy<PtrM, PmT<Ps...>>
	{
		using element_type  = PtrM<void>;
		using primitive_map = PmT<Ps...>;
		using this_t        = proxy<PtrM, PmT<Ps...>>;

		template<typename...Args>
			requires::std::constructible_from<element_type, Args&&...>
		constexpr proxy(Args &&...args) 
			: value_{::std::forward<Args>(args)...} 
		{};

		template<entity_of<primitive_map> Ent, typename...Args>
			requires::std::constructible_from<Ent, Args&&...>
		explicit constexpr proxy(inplace<Ent> use_prx_inplace, Args&&...args)
			: value_{make_t<PtrM<Ent>>{}(::std::forward<Args>(args)...)}
		{ map_.template rebind<Ent>(); }

	private:
		template<typename T, typename Traits>
		struct find
		{ static constexpr auto value{::std::same_as<T, typename Traits::name_t>}; };

	public:
		template<typename T, typename...Args>
		inline auto invoke(Args &&...args) const
		{ 
			if (!value_.get()) throw bad_proxy_call{};
			if constexpr (presented_at<T, primitive_map>)
				return map_.template invoke<T>(value_.get(), ::std::forward<Args>(args)...); 
			else if constexpr(presented_in<T, typename Ps::name_t...>)
				return map_.template invoke<first_required_t<T, find, Ps...>>(value_.get(), ::std::forward<Args>(args)...); 
			else 
				static_assert(presented_in<T, typename Ps::name_t...>, "[PRX ERROR]: Primitive not prsented.");
		}

		template<entity_of<primitive_map> Ent, typename...Args>
			requires::std::constructible_from<Ent, Args&&...>
		inline auto construct(Args &&...args)
		{ 
			value_ = make_t<PtrM<Ent>>{}(::std::forward<Args>(args)...); 
			map_.template rebind<Ent>();
		}

		template<typename T, typename...Args>
		friend inline constexpr this_t make(Args&&...args)
			requires(::std::same_as<T, this_t> && ::std::constructible_from<this_t, Args &&...>)
		{ return{::std::forward<Args>(args)...}; }

	private:
		primitive_map map_;
		element_type value_; 
	};
}
#pragma endregion

#endif