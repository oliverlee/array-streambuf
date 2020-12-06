#include "dummy.h"
#include "log.h"

#include <cstdlib>
#include <new>

auto operator new(std::size_t size) -> void* {
    if (auto* ptr = std::malloc(size)) {
        std::cout << "[operator::new(" << size << ") -> " << ptr << "]\n";
        return ptr;
    }

    throw std::bad_alloc{};
}

auto operator delete(void* ptr) noexcept -> void {
    std::cout << "[operator::delete(" << ptr << ")]\n";
    std::free(ptr);
}

auto main() -> int {
    using namespace array_streambuf;

    { auto d = Dummy(); }

    log::info << "creating stream" << std::endl;

    log::info << "writing to stream" << std::endl;
    log::debug << "Here is a very long string that would normally result in allocation!"
               << std::endl;

    log::info << "done!" << std::endl;
    return 0;
}
