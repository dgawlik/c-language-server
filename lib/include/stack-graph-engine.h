#include <utility>
#include <string>
#include <stack-graph-tree.h>
#include <unordered_map>
#include <tuple>

using std::string;
using std::tuple;
using std::unordered_map;

#ifndef STACK_GRAPH_ENGINE_H
#define STACK_GRAPH_ENGINE_H

namespace stack_graph
{
    typedef tuple<uint32_t, uint32_t, string> Coordinate;

    class CustomHash
    {
    public:
        // Implement a hash function
        std::size_t operator()(const Coordinate &k) const
        {
            std::size_t h1 = std::hash<uint32_t>{}(std::get<0>(k));
            std::size_t h2 = std::hash<uint32_t>{}(std::get<1>(k));
            std::size_t h3 = std::hash<string>{}(std::get<2>(k));
            return 31 * 31 * h3 + 31 * h2 + h1;
        }
    };

    

    struct StackGraphEngine
    {
        tuple<string, shared_ptr<StackGraphNode>> symbol_tree;
        unordered_map<Coordinate, shared_ptr<StackGraphNode>, CustomHash> node_table;

        void loadFile(string path);

        shared_ptr<Coordinate> resolve(Coordinate c);
    };
}

#endif