#ifndef ASCII_VIDEO_EXCEPT_INCLUDE
#define ASCII_VIDEO_EXCEPT_INCLUDE

#include <stdexcept>

namespace ascii {
    class bad_alloc : public std::bad_alloc {
        private:
            const char* m_explanation;
        public:
            bad_alloc(const char* explanation);
            virtual const char* what() const noexcept;
    };
}

#endif