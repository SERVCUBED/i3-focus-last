namespace i3_focus_last {
  #ifndef DEBUG
    inline char pipefname[100];
    #define likely(x)       __builtin_expect(!!(x), 1)
    #define unlikely(x)     __builtin_expect(!!(x), 0)

    #define DEBUG CMAKE_BUILD_TYPE==Debug
    #if DEBUG
      inline uint8_t isDebug = 0u;
      /* Binary debug levels:
       * 00000   0  Off
       * 00001   1  Basic
       * 00010   2  Workspace
       * 00100   4  Command
       * 01000   8  Window
       * 10000  16  Private
       * 11111  31  Max
       */
      #define DBG_MAX_LEVEL 0b11111
      #define DBG_IS_LEVEL(lvl) if ((isDebug & lvl) == lvl)
      #define DBG_LEVEL(lvl, str) DBG_IS_LEVEL(lvl) std::cout << str << std::endl
      #define DBG_PRIVATE(str) DBG_LEVEL(16u, "\033[1;36m" << str << "\033[0m")
      #define DBG_WINDOW(str) DBG_LEVEL(8u, "\033[1;31m" << str << "\033[0m")
      #define DBG_CMD(str) DBG_LEVEL(4u, "\033[1;36m" << str << "\033[0m")
      #define DBG_WORKSPACE(str) DBG_LEVEL(2u, "\033[1;33m" << str << "\033[0m")
      #define DBG_BASIC(str) DBG_LEVEL(1u, str)
      #define DEBUG_MSG(str) if (isDebug) std::cout << str << std::endl
    #else
      #define DEBUG_MSG(str) (void)0
    #endif
    #define ERROR_MSG(str) std::cerr << str << std::endl
  #endif
}