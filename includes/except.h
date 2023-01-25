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

    class not_found_error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    class forbidden_action_error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}

#endif