#pragma once

#include <boost/preprocessor.hpp>

// ブロック内にセクションを張る
#define FUJINAMI_LOGGING_SECTION(name)                                        \
  const ::fujinami::logging::ScopedSection _fl_scoped_section_##__COUNTER__ { \
    name                                                                      \
  }

// ログを吐く
#define FUJINAMI_LOG_(v, ...) ::fujinami::logging::Logger::v(__VA_ARGS__)

#ifdef FUJINAMI_LOGGING_TRACE_ON
#define FUJINAMI_LOG_trace(...) FUJINAMI_LOG_(trace, __VA_ARGS__)
#else  // FUJINAMI_LOGGING_TRACE_ON
#define FUJINAMI_LOG_trace(...) \
  do {                          \
  } while (false)
#endif  // FUJINAMI_LOGGING_TRACE_ON

#ifdef FUJINAMI_LOGGING_DEBUG_ON
#define FUJINAMI_LOG_debug(...) FUJINAMI_LOG_(debug, __VA_ARGS__)
#else  // FUJINAMI_LOGGING_DEBUG_ON
#define FUJINAMI_LOG_debug(...) \
  do {                          \
  } while (false)
#endif  // FUJINAMI_LOGGING_DEBUG_ON

#define FUJINAMI_LOG_info(...) FUJINAMI_LOG_(info, __VA_ARGS__)
#define FUJINAMI_LOG_warn(...) FUJINAMI_LOG_(warn, __VA_ARGS__)
#define FUJINAMI_LOG_error(...) FUJINAMI_LOG_(error, __VA_ARGS__)
#define FUJINAMI_LOG_critical(...) FUJINAMI_LOG_(critical, __VA_ARGS__)
#define FUJINAMI_LOG(v, ...) FUJINAMI_LOG_##v(__VA_ARGS__)

#ifndef FUJINAMI_LOGGING_SUPRESS
// printを定義する
#define FJL_PRINT_BLOCK(block) block
#define FUJINAMI_LOGGING_DEFINE_PRINT(specifier, T, t, block)        \
  specifier std::ostream& operator<<(std::ostream& os, const T& t) { \
    using ::fujinami::logging::operator<<;                           \
    FJL_PRINT_BLOCK block return os;                                 \
  }

#else  // FUJINAMI_LOGGING_SUPRESS
// printを定義する
#define FUJINAMI_LOGGING_DEFINE_PRINT(specifier, T, t, block)        \
  specifier std::ostream& operator<<(std::ostream& os, const T& t) { \
    os << "...";                                                     \
    return os;                                                       \
  }

#endif  // FUJINAMI_LOGGING_SUPRESS

// enumの文字列化を定義する
#define FJL_ENUM_MACRO(r, T, VALUE)  \
  case T::VALUE:                     \
    os << BOOST_PP_STRINGIZE(VALUE); \
    break;
#define FUJINAMI_LOGGING_ENUM(specifier, T, seq) \
  FUJINAMI_LOGGING_DEFINE_PRINT(                 \
      specifier, T, value,                       \
      (switch (value){BOOST_PP_SEQ_FOR_EACH(FJL_ENUM_MACRO, T, seq)}))

// Flagsetの文字列化を定義する
#define FJL_FLAGSET_MACRO(r, T, VALUE) \
  if (value & T::VALUE) os << sep << BOOST_PP_STRINGIZE(VALUE);
#define FUJINAMI_LOGGING_FLAGSET(specifier, T, FlagsetT, seq) \
  FUJINAMI_LOGGING_DEFINE_PRINT(                              \
      specifier, FlagsetT, value,                             \
      (::fujinami::logging::Separator sep('|');               \
       BOOST_PP_SEQ_FOR_EACH(FJL_FLAGSET_MACRO, T, seq)))

// struct/classの文字列化を定義する
#define FJL_STRUCT_MACRO(r, data, tpl)            \
  os << sep << BOOST_PP_TUPLE_ELEM(0, tpl) << ':' \
     << value.BOOST_PP_TUPLE_ELEM(1, tpl);
#define FUJINAMI_LOGGING_STRUCT(T, seq)               \
  FUJINAMI_LOGGING_DEFINE_PRINT(                      \
      friend, T, value,                               \
      (::fujinami::logging::Separator sep; os << '{'; \
       BOOST_PP_SEQ_FOR_EACH(FJL_STRUCT_MACRO, _, seq) os << '}';))

// unionの文字列化を定義する
#define FJL_UNION_MACRO(r, TypeT, tpl)                                      \
  case TypeT::BOOST_PP_TUPLE_ELEM(0, tpl):                                  \
    os << "type:" << BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, tpl)) << ',' \
       << BOOST_PP_TUPLE_ELEM(1, tpl) << ':'                                \
       << value.BOOST_PP_TUPLE_ELEM(2, tpl);                                \
    break;
#define FUJINAMI_LOGGING_UNION(T, TypeT, seq)                \
  FUJINAMI_LOGGING_DEFINE_PRINT(                             \
      friend, T, value, (os << '{'; switch (value.type_) {   \
        case TypeT::NONE:                                    \
          os << "type:NONE";                                 \
          break;                                             \
          BOOST_PP_SEQ_FOR_EACH(FJL_UNION_MACRO, TypeT, seq) \
      } os << '}';))
