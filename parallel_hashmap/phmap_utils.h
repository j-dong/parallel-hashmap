#if !defined(phmap_utils_h_guard_)
#define phmap_utils_h_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2019, Gregory Popovitch - greg7mdp@gmail.com
//
//       minimal header providing phmap::HashState
//
//       use as:  phmap::HashState().combine(0, _first_name, _last_name, _age);
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4514) // unreferenced inline function has been removed
    #pragma warning(disable : 4710) // function not inlined
    #pragma warning(disable : 4711) // selected for automatic inline expansion
#endif

#include <cstdint>
#include <functional>
#include <tuple>
#include "phmap_bits.h"

// ---------------------------------------------------------------
// Absl forward declaration requires global scope.
// ---------------------------------------------------------------
#if !defined(phmap_fwd_decl_h_guard_) && !defined(ABSL_HASH_HASH_H_)
    namespace absl { template <class T> struct Hash; };
#endif

namespace phmap
{

// --------------------------------------------
template<int n>
struct fold_if_needed
{
    inline size_t operator()(uint64_t) const;
};

template<>
struct fold_if_needed<4>
{
    inline size_t operator()(uint64_t a) const
    {
        return static_cast<size_t>(a ^ (a >> 32));
    }
};

template<>
struct fold_if_needed<8>
{
    inline size_t operator()(uint64_t a) const
    {
        return static_cast<size_t>(a);
    }
};

// ---------------------------------------------------------------
// see if class T has a hash_value() friend method
// ---------------------------------------------------------------
template<typename T>
struct has_hash_value
{
private:
    typedef std::true_type yes;
    typedef std::false_type no;

    template<typename U> static auto test(int) -> decltype(hash_value(std::declval<const U&>()) == 1, yes());

    template<typename> static no test(...);

public:
    static constexpr bool value = std::is_same<decltype(test<T>(0)), yes>::value;
};

#if !defined(phmap_fwd_decl_h_guard_)
    template <class T> using Hash = ::absl::Hash<T>;
#endif

#if defined(_MSC_VER)
#   define PHMAP_HASH_ROTL32(x, r) _rotl(x,r)
#else
#   define PHMAP_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif


template <class H, int sz> struct Combiner
{
    H operator()(H seed, size_t value);
};

template <class H> struct Combiner<H, 4>
{
    H operator()(H h1, size_t k1)
    {
        // Copyright 2005-2014 Daniel James.
        // Distributed under the Boost Software License, Version 1.0. (See accompanying
        // file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
        
        const uint32_t c1 = 0xcc9e2d51;
        const uint32_t c2 = 0x1b873593;

        k1 *= c1;
        k1 = PHMAP_HASH_ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = PHMAP_HASH_ROTL32(h1,13);
        h1 = h1*5+0xe6546b64;

        return h1;
    }
};

template <class H> struct Combiner<H, 8>
{
    H operator()(H h, size_t k)
    {
        // Copyright 2005-2014 Daniel James.
        // Distributed under the Boost Software License, Version 1.0. (See accompanying
        // file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
        const uint64_t m = (uint64_t(0xc6a4a793) << 32) + 0x5bd1e995;
        const int r = 47;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;

        // Completely arbitrary number, to prevent 0's
        // from hashing to 0.
        h += 0xe6546b64;

        return h;
    }
};

// define HashState to combine member hashes... see example below
// -----------------------------------------------------------------------------
template <typename H>
class HashStateBase {
public:
    template <typename T, typename... Ts>
    static H combine(H state, const T& value, const Ts&... values);

    static H combine(H state) { return state; }
};

template <typename H>
template <typename T, typename... Ts>
H HashStateBase<H>::combine(H seed, const T& v, const Ts&... vs)
{
    return HashStateBase<H>::combine(Combiner<H, sizeof(H)>()(
                                         seed, phmap::Hash<T>()(v)), 
                                     vs...);
}

using HashState = HashStateBase<size_t>;

// -----------------------------------------------------------------------------

}  // namespace phmap

#ifdef _MSC_VER
     #pragma warning(pop)
#endif

#endif // phmap_utils_h_guard_
