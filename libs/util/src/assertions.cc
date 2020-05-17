#include <scroom/assertions.hh>

#include <errno.h>
#ifdef _WIN32
  #include <boost/stacktrace.hpp>
#else
	#include <execinfo.h>
#endif
#include <signal.h>
#include <string.h>
#include <iostream>
#include <cstdlib>


namespace Scroom
{
  namespace Utils
  {
    namespace Detail
    {
      void stackTrace(std::ostream &os, size_t cutoff) {
        #ifdef _WIN32
          // Stack traces generated by this are not very informative, they just show the DLLs.
          // Showing more than then DDLs would probably require an additional dependency
          os << boost::stacktrace::stacktrace();
          // Silence the compiler warning for unused variable
          (void)cutoff;
        #else
          os << "stack trace (innermost first):" << std::endl;
          static const size_t maxTraces = 100;
          void *array[maxTraces];
          size_t nTraces = backtrace(array, maxTraces);
          char **strings = backtrace_symbols(array, static_cast<int>(nTraces));
          for (size_t i = cutoff + 1; i < nTraces; i++) {
            os << '#' << i << "  " << strings[i] << std::endl;
          }
          free(strings);
        #endif
      }

      void abort() __attribute__ ((noreturn));

      void assertionFailed(const char *type, const char *expr,
          const char *function, const char *filename, unsigned int line) {
        std::cerr << "PROGRAM DEFECTIVE: " << filename << ":" << line << ": "
                << type << " " << expr << " violated in " << function;

            stackTrace(std::cerr, 1);
            std::cerr << std::endl << std::flush;
            abort();
      }

      class ErrorSignalHandler {
            friend void abort();

            static volatile sig_atomic_t isHandlerActive;
            static void handler(int);

      public:
            ErrorSignalHandler();
        };

      volatile sig_atomic_t ErrorSignalHandler::isHandlerActive = 0;

      void ErrorSignalHandler::handler(int sig) {
            if (!isHandlerActive) {
              isHandlerActive = 1;
              std::cerr << __FILE__ << ":" << __LINE__ << ": Entering signal handler" << std::endl;
              #ifdef _WIN32
                std::string sigName;
                switch (sig) {
                  case SIGFPE:
                    sigName = "SIGFPE";
                    break;
                  case SIGILL:
                    sigName = "SIGILL";
                    break;
                  case SIGABRT:
                    sigName = "SIGABRT";
                    break;
                  case SIGSEGV:
                    sigName = "SIGSEGV";
                    break;
                }
                std::cerr << "PROGRAM DEFECTIVE (TERMINATED BY SIGNAL): " << sigName << std::endl;
              #else
                std::cerr << "PROGRAM DEFECTIVE (TERMINATED BY SIGNAL): " << strsignal(sig) << std::endl;
              #endif
              stackTrace(std::cerr, 1);
              std::cerr << std::endl;
              std::cerr << __FILE__ << ":" << __LINE__ << ": Leaving signal handler" << std::endl;
              std::cerr << std::flush;
            }

            signal(sig, SIG_DFL);
            raise(sig);
      }

      ErrorSignalHandler::ErrorSignalHandler() {
            signal(SIGFPE, handler);
            signal(SIGILL, handler);
            signal(SIGABRT, handler);
            signal(SIGSEGV, handler);
        #ifndef _WIN32
              signal(SIGBUS, handler);
              signal(SIGSYS, handler);
        #endif
        }

        static ErrorSignalHandler errorSignalHandler;

        void abort() {
            signal(SIGABRT, SIG_DFL);
            ::abort();
            signal(SIGABRT, errorSignalHandler.handler);
        }

     }
  }
}
