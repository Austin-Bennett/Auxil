#pragma once

#include <bits/stdc++.h>

namespace Auxil {
#define FORCE_INLINE __attribute__((always_inline)) inline

    template<typename T>
    using atomic_shared = std::atomic<std::shared_ptr<T>>;

}
