#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>
#include <iterator>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <climits>
#include <time.h>
#include <iostream>
#include <boost/functional/hash.hpp>

#include <cassert>

#include "Snap.h"

#ifndef COMMON_HPP
#define COMMON_HPP

#define SHIFTSIZE 31
#define SHIFTMASK INT_MAX

using namespace std;

typedef int vid_type;
typedef long eid_type;
// we actually can use int32 for the inode id type in this implementation
typedef long inode_id_type;
struct inode{
    inode_id_type parent;
    size_t size;
    int k;
    vector<inode_id_type> children;
    unordered_map<vid_type, vector<vid_type>> adj_list;
    
    inode(inode_id_type parent = -1, size_t size = -1, int k = -1): 
        parent(parent), size(size), k(k) { }

    bool operator==(const inode& rhs) const {
        return this->parent == rhs.parent && 
            this->size == rhs.size &&
            this->k == rhs.k;
    }
};
typedef unordered_map<eid_type, int> eint_map;
typedef set< pair<int, eid_type> > slow_sorted_type;
typedef vector< list<eid_type> > counting_sorted_type;
typedef unordered_map<eid_type, inode_id_type> eiid_map;
typedef unordered_map<inode_id_type, inode> iidinode_map;

inline eid_type edge_composer(vid_type u, vid_type v) {
    if (u < v) 
        return (eid_type(u) << SHIFTSIZE) + v;
    else
        return (eid_type(v) << SHIFTSIZE) + u;
}

inline pair<vid_type, vid_type> vertex_extractor(eid_type e) {
    vid_type v = vid_type(e & SHIFTMASK);
    vid_type u = (e - v) >> SHIFTSIZE;
    return make_pair(u, v);
}

void get_low_high_deg_vertices(const PUNGraph &net, 
        vid_type src, vid_type dst, 
        vid_type &low, vid_type &high) {
    if (net->GetNI(src).GetDeg() > net->GetNI(dst).GetDeg()) {
        low = dst;
        high = src;
    }
    else {
        low = src;
        high = dst;
    }
}

/* 
 * Convert eid_type (long) to vid_type (int), 
 * because SNAP only supports int32 as vertex id.
 * Due to this limitation, this implemetation cannot handle 
 * graphs with more than INT_MAX (around 2 billions) edges
 */
unordered_map<eid_type, vid_type> encode_table;
unordered_map<vid_type, eid_type> decode_table;
int vid_cnt = 0;

inline vid_type mst_vid_composer(eid_type e) {
    if (encode_table.find(e) == encode_table.end()) {
        encode_table.insert(make_pair(e, vid_cnt));
        decode_table.insert(make_pair(vid_cnt, e));
        vid_cnt ++;
    }
    return encode_table[e];
}

inline eid_type edge_extractor(vid_type x) {
    return decode_table[x];
}

/*
 * virtual node id starts at -2 as 0 and -1 is reserved
 * virtual node is for communities that contains edges from mst but no vertex. 
 */
inode_id_type reserve_interval = -2; 

// definitions for tcp index
struct TCPIndex {
    PUNGraph ego_graph;
    eint_map ego_triangle_trussness;
};

typedef TCPIndex tcp_index_type;
typedef unordered_map<vid_type, tcp_index_type> tcp_index_table_type;

// definitions for equitruss index
struct EquiIndexNode {
    vid_type snID;
    int k;
    vector<eid_type> edge_list;
    
    EquiIndexNode(vid_type snID = -1, int k = -1): snID(snID), k(k) {} 
};

struct EquiIndex {
    PUNGraph super_graph;
    unordered_map<vid_type, EquiIndexNode> super_nodes;

    EquiIndex() {
        super_graph = TUNGraph::New();
    }
    // SNAP uses smart-pointers, no need to explicitly free graph objects.
};

typedef EquiIndex equi_index_type;
typedef unordered_map<vid_type, unordered_set<vid_type>> equi_hash_type;

// definitions for queries
struct QR {
    inode_id_type iid;
    size_t size;
    int k;

    QR(inode_id_type iid = -1, int size = -1, int k = -1): 
        iid(iid), size(size), k(k) { }

    bool operator==(const QR& rhs) const {
        return this->iid == rhs.iid;
    }

    void set(inode_id_type iid, int size, int k) {
        this->iid = iid;
        this->size = size;
        this->k = k;
    }
};

typedef vector< pair<vid_type, vid_type> > community_type;
typedef QR qr_type;
typedef vector<qr_type> qr_set_type;
typedef vector<community_type> exact_qr_set_type;

inline bool is_vertex_in_mst_vertex(vid_type mstv, vid_type v) {
    pair<vid_type, vid_type> vpair = 
        vertex_extractor(edge_extractor(mstv));
    return (v == vpair.first || v == vpair.second);
}

class Timer {
    public:
        Timer() {
            last_time = clock();
        }

        void print_n_update_timer() {
            cout << "\telapsed time: " << update_timer() 
                << "ms." << endl;
        }

        double update_timer() {
            double time_diff = double(clock() - last_time) / 
                    (double(CLOCKS_PER_SEC) / 1000);
            last_time = clock();
            return time_diff;
        }

    private:
        clock_t last_time;
};

#endif
