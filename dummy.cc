#include "dummy.h"

#include "log.h"

namespace array_streambuf {

Dummy::Dummy() {
    log::debug << "Dummy::Dummy()\n";
}

Dummy::~Dummy() {
    log::debug << "Dummy::~Dummy()\n";
}

} // namespace array_streambuf
