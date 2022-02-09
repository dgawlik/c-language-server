#include <stack-graph-tree.h>
#include <stack-graph-engine.h>
#include <tree_sitter/api.h>
#include <iostream>
#include <tuple>

using std::shared_ptr;
using std::string;
using std::stringstream;
using std::vector;

using stack_graph::Coordinate;
using stack_graph::Point;
using stack_graph::StackGraphEngine;

int main()
{
    auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
    StackGraphEngine engine;

    engine.loadDirectoryRecursive(path);
    engine.crossLink();

    for(auto cl : engine.cross_links){
        std::cout << cl.repr() << std::endl;
    }

    return 0;
}