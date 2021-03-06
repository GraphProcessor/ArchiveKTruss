#include "common.hpp"

#ifndef TREE_INDEX_HPP
#define TREE_INDEX_HPP

void update_inode_size(const inode_id_type root_iid, iidinode_map &index_tree) {
    inode_id_type iid = root_iid;
    while(iid != -1) {
        index_tree[iid].size ++; 
        iid = index_tree[iid].parent;
    } 
}

void update_inode_parent(inode_id_type lower_iid, 
        const inode_id_type iid, iidinode_map &index_tree) {
    if (lower_iid != -1) {
        index_tree[lower_iid].parent = iid;
    }
}

void add_new_inode(inode_id_type id,
        const inode_id_type parent,
        const size_t size,
        const int k,
        iidinode_map &index_tree,
        eiid_map &index_hash, 
        const bool virtual_inode = false) {
    // add a special case here, if trussness of the edge is 2, 
    // we dont create inode for it
    if (k == 2 && id != -1) {
        index_hash.insert(make_pair(edge_extractor(id), -1));
        return;
    }
    // create inode based on id, here the id is the vertex or edge used to create inode
    inode in(parent, size, k);
    // add inode to the index tree and update hashtable, negative id means virtual vertices
    index_tree.insert(make_pair(id, in));
    if (!virtual_inode) {
        index_hash.insert(make_pair(edge_extractor(id), id));
        update_inode_size(parent, index_tree);
    }
}

void construct_index_tree(const PUNGraph &mst, 
        const eint_map &edge_trussness,
        const eint_map &triangle_trussness,
        iidinode_map &index_tree,
        eiid_map &index_hash) {
    // we have a virtual root which is the parent of all the roots 
    // create a container to store unvisited vertices, O(V_mst) = O(E)
    unordered_set<vid_type> unvisited_vertices;
    for (TUNGraph::TNodeI NI = mst->BegNI(); NI < mst->EndNI(); NI++) {
        unvisited_vertices.insert(NI.GetId());
    }
    // add the hyper inode first, with the minimum k of 2
    add_new_inode(-1, -1, 0, 2, index_tree, index_hash, true);
    // perform the BFS to construct tree index, O(E_mst) < O(V_mst) = O(E)
    queue<vid_type> fifo; // fifo for BFS
    unordered_map<vid_type, vid_type> parents; // parent for each vertex
    while(!unvisited_vertices.empty()) {
        // this loop is for the forest, as there might be multiple connected components
        vid_type next_root = *(unvisited_vertices.begin());
        fifo.push(next_root);
        unvisited_vertices.erase(next_root);
        while(!fifo.empty()) {
            // this loop is for the tree, a single connected component
            vid_type u = fifo.front();
            eid_type e_u = edge_extractor(u);
            eint_map::const_iterator citer = edge_trussness.find(e_u);
            // an edge has minimum truss of 2
            int node_k = citer->second;
            fifo.pop();
            int u_deg = mst->GetNI(u).GetDeg();
            // add children to fifo
            for (int i = 0; i < u_deg; i++) {
                vid_type v = mst->GetNI(u).GetNbrNId(i);
                if (unvisited_vertices.find(v) == unvisited_vertices.end())
                    continue;
                fifo.push(v);
                unvisited_vertices.erase(v);
                parents.insert(make_pair(v, u));
            }
            // add itself to the tree index, assume its parent during BFS is v
            bool processed = false;
            int edge_k = -1;
            vid_type v = -1;
            inode_id_type v_iid = -1;
            int v_k = -1;
            inode_id_type lower_iid = -1;
            size_t lower_size = 0;
            // if there is no parent of u, create an inode for u, 
            // with u as inode id 
            if (parents.find(u) ==  parents.end()) {
                add_new_inode(u, -1, 1, node_k, index_tree, index_hash);
                processed = true;
            } 
            // if there is a parent v, find its status
            else {
                v = parents[u];
                eid_type e = edge_composer(u,v);
                eint_map::const_iterator citer = triangle_trussness.find(e);
                edge_k = citer->second;
                v_iid = index_hash[edge_extractor(v)];
                v_k = index_tree[v_iid].k;
            }
            while (!processed) {
                // first go up the existing tree to find insert point
                if (v_k > edge_k) { 
                    // not a valid insert point, 
                    // moving to parent node on index tree
                    lower_iid = v_iid;
                    lower_size = index_tree[lower_iid].size;
                    v_iid = index_tree[v_iid].parent;
                    v_k = index_tree[v_iid].k;
                } 
                // insert depends on the trussness of itself, 
                // the insert point and the edge
                else if (v_k < edge_k) {
                    if (edge_k == node_k) {
                        // create inode based on u
                        add_new_inode(u, v_iid, lower_size + 1, 
                                node_k, index_tree, index_hash);
                        // link lower id with this cluster
                        update_inode_parent(lower_iid, u, index_tree);
                        // mark as processed
                        processed = true;
                    }
                    else if (edge_k < node_k) {
                        // create inode based on e as 
                        // u does not belong to this inode
                        inode_id_type e_iid = -u + reserve_interval;
                        add_new_inode(e_iid, v_iid, lower_size, 
                                edge_k, index_tree, index_hash, true);
                        // link lower id with this edge cluster
                        update_inode_parent(lower_iid, e_iid, index_tree);
                        // create inode based on u
                        add_new_inode(u, e_iid, 1, node_k, 
                                index_tree, index_hash);
                        // mark as processed
                        processed = true;
                    }
                    else {
                        cerr << "wrong case 1" << endl;
                        processed = true;
                    }
                }
                else { // v_k == edge_k
                    if (edge_k == node_k) {
                        // update hash table
                        index_hash.insert(make_pair(
                                    edge_extractor(u), v_iid));
                        update_inode_size(v_iid, index_tree);
                        // mark as processed
                        processed = true;
                    }
                    else if (edge_k < node_k) { 
                        // create inode based on u
                        add_new_inode(u, v_iid, 1, node_k, 
                                index_tree, index_hash);
                        // mark as processed
                        processed = true;
                    }
                    else{
                        cerr << "wrong case 2" << endl;
                        processed = true;
                    }
                }
            }

            // add edge (v,u) of mst to the adj list
            if (v != -1) {
                inode_id_type u_iid = index_hash[e_u];
                v_iid = index_hash[edge_extractor(v)];
                if (index_tree[u_iid].adj_list.find(u) == 
                        index_tree[u_iid].adj_list.end())
                    index_tree[u_iid].adj_list[u] = vector<vid_type>();
                index_tree[u_iid].adj_list[u].push_back(v);
                if (index_tree[v_iid].adj_list.find(v) == 
                        index_tree[v_iid].adj_list.end())
                    index_tree[v_iid].adj_list[v] = vector<vid_type>();
                index_tree[v_iid].adj_list[v].push_back(u);
            }
        }
    }

    // add children to each node
    for (auto iter = index_tree.begin();
            iter != index_tree.end();
            ++ iter) {
        if (iter->second.parent != -1) 
            index_tree[iter->second.parent].children.push_back(iter->first);
    }

}

#endif
