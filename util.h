namespace i3_focus_last {
  static constexpr const char *pipefname = "/tmp/i3-focus-last.pipe";

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define DEBUG CMAKE_BUILD_TYPE==Debug
#if DEBUG
    inline bool isDebug = false;
#define DEBUG_MSG(str) if (isDebug) std::cout << str << std::endl
#else
#define DEBUG_MSG(str) (void)0
#endif
#define ERROR_MSG(str) std::cerr << str << std::endl
}