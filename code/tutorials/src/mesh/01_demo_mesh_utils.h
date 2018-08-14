
#ifndef NDEBUG
#define assert_between(__low, __v, __high)                                                                                                                    \
	if(__allscale_unused auto __allscale_temp_object_ = insieme::utils::detail::LazyAssertion((__low) <= (__v) && (__v) <= (__high)))                         \
	std::cerr << "\nAssertion " #__low " <= " #__v " <= " #__high " of " __FILE__ ":" __allscale_xstr_(__LINE__) " failed!\n\t" #__v " = " << (__v) << "\n"
#else
#define assert_between(__low, __v, __high) if(0) std::cerr
#endif

#define MIN_TEMP 0
#define MAX_TEMP 511

#ifdef USE_TEMPERATURE_ASSERTIONS
#define assert_temperature(__v)                                                                                                                               \
	assert_between(MIN_TEMP, __v, MAX_TEMP)
#else // USE_TEMPERATURE_ASSERTIONS
#define assert_temperature(__v) if(0) std::cerr
#endif // USE_TEMPERATURE_ASSERTIONS
