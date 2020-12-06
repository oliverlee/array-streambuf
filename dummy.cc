#include "dummy.h"
#include "log.h"

Dummy::Dummy() {
    log::debug << "Dummy::Dummy()\n";
}

Dummy::~Dummy() {
    log::debug << "Dummy::~Dummy()\n";
}
