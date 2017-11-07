#include "common.hpp"
#include <algorithm>

#ifndef QUERY_HPP
#define QUERY_HPP


void truss_raw_edge_query(
        community_type &truss_community,
        eid_type query_e,
        int query_k, 
        PUNGraph &net,
        eint_map &edge_trussness,
        unordered_set<eid_type, boost::hash<eid_type> > &visited_edges) {
    // truss discovery for a single edge
    queue<eid_type> fifo;
    fifo.push(query_e);
    visited_edges.insert(query_e);
    while (!fifo.empty()) {
        eid_type e = fifo.front();
        fifo.pop();
        truss_community.insert(e);
        for (int j = 0; j < net->GetNI(e.first).GetDeg(); j++) {
            vid_type w = net->GetNI(e.first).GetNbrNId(j);
            // test if it is a triangle
            if (net->IsEdge(e.second, w)) {
                eid_type adj_e1 = edge_composer(e.first, w);
                eid_type adj_e2 = edge_composer(e.second, w);
                if (edge_trussness[adj_e1] >= query_k &&
                        edge_trussness[adj_e2] >= query_k) {
                    if (visited_edges.find(adj_e1) == visited_edges.end()) {
                        fifo.push(adj_e1);
                        visited_edges.insert(adj_e1);
                    }
                    if (visited_edges.find(adj_e2) == visited_edges.end()) {
                        fifo.push(adj_e2);
                        visited_edges.insert(adj_e2);
                    }
                }
            }
        }
    }
}

void truss_raw_query(exact_qr_set_type &truss_communities,
        vid_type query_vid, 
        int query_k, 
        PUNGraph &net, 
        eint_map &edge_trussness) {
    // this is the k-truss query based on the edge trussness index
    unordered_set<eid_type, boost::hash<eid_type> > visited_edges;
    for (int i = 0; i < net->GetNI(query_vid).GetDeg(); i++) {
        vid_type u = net->GetNI(query_vid).GetNbrNId(i);
        eid_type e = edge_composer(u, query_vid);
        if (edge_trussness[e] >= query_k &&
                visited_edges.find(e) == visited_edges.end()) {
            community_type truss_community;
            truss_raw_edge_query(
                    truss_community, e, query_k, net, edge_trussness, visited_edges);
            truss_communities.push_back(truss_community);
        }
    }
}

void branch_search(vector<inode_id_type> &branch_nodes,
        inode_id_type iid, 
        iidinode_map &index_tree,
        int query_k = -1) {
    // use vector because in some query the order matters
    while(iid != -1 && index_tree[iid].k >= query_k) {
        branch_nodes.push_back(iid);
        iid = index_tree[iid].parent;
    }
}

void truss_k_query_single_v(qr_set_type &res,
        vid_type query_vid,
        int query_k,
        PUNGraph &net,
        iidinode_map &index_tree,
        eiid_map &index_hash) {
    for (int i = 0; i < net->GetNI(query_vid).GetDeg(); i++) {
        vid_type v = net->GetNI(query_vid).GetNbrNId(i);
        inode_id_type iid = index_hash[edge_composer(query_vid, v)];
        vector<inode_id_type> branch_nodes;
        branch_search(branch_nodes, iid, index_tree, query_k); 
        inode_id_type last_iid = -1;
        if (!branch_nodes.empty())
            last_iid = branch_nodes[branch_nodes.size() - 1];
        // if there is not a truss with exact k, use the one with higher k
        if (last_iid != -1 && index_tree[last_iid].k >= query_k) {
            qr_type res_node(last_iid, index_tree[last_iid].size, index_tree[last_iid].k);
            res.insert(res_node);
        }
    }
}

void truss_k_query(qr_set_type &res,
        vector<vid_type>& query_vids, 
        int query_k,
        PUNGraph &net,
        iidinode_map &index_tree,
        eiid_map &index_hash) {
    for (vector<vid_type>::const_iterator 
            citer = query_vids.begin();
            citer != query_vids.end();
            ++ citer) {
        vid_type query_vid = *citer;
        truss_k_query_single_v(res, query_vid, query_k, net, index_tree, index_hash);
    }
}

void truss_forest_single_v (set<inode_id_type> &single_forest,
        vid_type query_vid,
        PUNGraph &net,
        iidinode_map &index_tree,
        eiid_map &index_hash) {
    // find all the iids of the single vertex
    for (int i = 0; i < net->GetNI(query_vid).GetDeg(); i++) {
        vid_type v = net->GetNI(query_vid).GetNbrNId(i);
        eid_type e = edge_composer(query_vid, v);
        inode_id_type iid = index_hash[e];
        vector<inode_id_type> branch_nodes;
        branch_search(branch_nodes, iid, index_tree);
        single_forest.insert(branch_nodes.begin(), branch_nodes.end());
    }
}

void truss_maxk_query(qr_set_type &res,
        vector<vid_type>& query_vids, 
        PUNGraph &net,
        iidinode_map &index_tree,
        eiid_map &index_hash) {
    set<inode_id_type> intersection_forest;
    bool init = false;
    for (vector<vid_type>::const_iterator 
            citer = query_vids.begin();
            citer != query_vids.end();
            ++ citer) {
        set<inode_id_type> single_forest;
        vid_type query_vid = *citer;
        // find maxk community of a single vertex
        truss_forest_single_v(single_forest, query_vid,
                net, index_tree, index_hash);
        // store intersection of both forest
        if (init) {
            set<inode_id_type> new_intersection_forest;
            std::set_intersection(single_forest.begin(), single_forest.end(),
                    intersection_forest.begin(), intersection_forest.end(),
                    std::inserter(new_intersection_forest, 
                        new_intersection_forest.begin()));
            intersection_forest = new_intersection_forest;
        }
        else {
            intersection_forest = single_forest;
            init = true;
        }
    }
    // find truss communtiy with max k in the remaining forest
    int max_k = -1;
    for (set<inode_id_type>::iterator iter = intersection_forest.begin();
            iter != intersection_forest.end();
            ++ iter) {
        if (index_tree[*iter].k > max_k) {
            max_k = index_tree[*iter].k;
            res.clear();
            qr_type res_node(*iter, index_tree[*iter].size, max_k);
            res.insert(res_node);
        }
        else if (index_tree[*iter].k == max_k) {
            qr_type res_node(*iter, index_tree[*iter].size, max_k);
            res.insert(res_node);
        }
    }
}

void truss_anyk_query(qr_set_type &res,
        vector<vid_type>& query_vids, 
        PUNGraph &net,
        iidinode_map &index_tree,
        eiid_map &index_hash) {
    for (vector<vid_type>::const_iterator 
            citer = query_vids.begin();
            citer != query_vids.end();
            ++ citer) {
        vid_type u = *citer;
        int u_deg = net->GetNI(u).GetDeg();
        for (int i = 0; i < u_deg; i++) {
            vid_type v = net->GetNI(u).GetNbrNId(i);
            inode_id_type iid = index_hash[edge_composer(u, v)];
            vector<inode_id_type> branch_nodes = branch_search(iid, index_tree); 
            for (size_t j = 0; j < branch_nodes.size(); j ++) {
                inode_id_type jiid = branch_nodes[j];
                qr_type res_node(jiid, index_tree[jiid].size, index_tree[jiid].k);
                res.insert(res_node);
            }
        }
    }
}

#endif
