//
// Created by Bikram Kumar Panda on 07/04/26.
//
#include "Hashlib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <algorithm>


//-----------------------------------------------------------------------------
typedef std::unordered_map<std::string, const HashInfo *>  HashMap;
typedef std::vector<const HashInfo *>                      HashMapOrder;

static HashMap & hashMap() {
    static auto * map = new HashMap;
    return *map;
}

//-----------------------------------------------------------------------------
// Add a hash to the hashMap list of all hashes.
//
// FIXME Verify hinfo is all filled out.
unsigned register_hash(const HashInfo *hinfo) {
  static std::unordered_map<uint32_t, const HashInfo *> hashcodes;
  std::string name = hinfo->name;

  // Allow users to lookup hashes by any case
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);

  if (hashMap().find(name) != hashMap().end()) {
    printf("Hash names must be unique.\n");
    printf("\"%s\" (\"%s\") was added multiple times.\n", hinfo->name,
           name.c_str());
    printf("Note that hash names are using a case-insensitive comparison.\n");
    exit(1);
  }

  hashMap()[name] = hinfo;
  return hashMap().size();
}


//-----------------------------------------------------------------------------
// Routines for querying/finding hashes that have been registered.

// The sort_order field is intended to be used for people adding
// hashes which should appear inside their family in
// other-than-alphabetical order.
//
// This is overloaded for mock hashes to also override the sorting for
// _family name_, which is not something general users should do.
static HashMapOrder defaultSort(const HashMap & map ) {
    HashMapOrder hashes;

    hashes.reserve(map.size());
    for (const auto& kv: map) {
        hashes.push_back(kv.second);
    }
    std::sort(hashes.begin(), hashes.end(), []( const HashInfo * a, const HashInfo * b ) {
            int r;
            // Then sort by family (case-insensitive)
            if ((r = strcasecmp(a->family, b->family)) != 0) {
                return r < 0;
            }
            // Then by hash output size (smaller first)
            if (a->bits != b->bits) {
                return a->bits < b->bits;
            }
            // And finally by hash name (case-insensitive)
            if ((r = strcasecmp(a->name, b->name)) != 0) {
                return r < 0;
            }
            return false;
        });
    return hashes;
}

std::vector<const HashInfo *> findAllHashes() {
    HashMapOrder hashes = defaultSort(hashMap());
    return hashes;
}

const HashInfo * findHash( const char * name ) {
    std::string n = name;

    // Search without regards to case
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    // Since underscores can't be in names, the user must have meant a dash
    std::replace(n.begin(), n.end(), '_', '-');

    const auto it = hashMap().find(n);
    if (it == hashMap().end()) {
        return nullptr;
    }
    return it->second;
}

//-----------------------------------------------------------------------------
void listHashes( bool nameonly ) {
    if (!nameonly) {
        printf("Hashnames can be supplied using any case letters.\n\n");
        printf("%-25s %4s %-60s\n", "Name", "Bits", "Description");
        printf("%-25s %4s %-60s\n", "----", "----", "-----------");
    }
    for (const HashInfo * h: defaultSort(hashMap())) {
        if (!nameonly) {
            printf("\n");
            printf("%-25s %4d %-60s\n", h->name, h->bits, h->desc);
        } else {
            printf("%s\n", h->name);
        }
    }
}

//-----------------------------------------------------------------------------
// See Hashrefs.cpp.in for why these exist. You can very likely just ignore
// them.
unsigned refs();
static unsigned dummy = refs();