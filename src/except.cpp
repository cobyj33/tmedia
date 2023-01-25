#include <except.h>

ascii::bad_alloc::bad_alloc(const char* explanation) {
    this->m_explanation = explanation;
}

const char* ascii::bad_alloc::what() const throw() {
    return this->m_explanation;
}