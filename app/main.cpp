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
using stack_graph::StackGraphEngine;
using stack_graph::Point;

int main()
{
    StackGraphEngine engine;

    engine.loadFile("/home/dominik/Code/intellisense/c-language-server/corpus/sample1.c");

    Coordinate coord = Coordinate(36, 4 , "org.emp.name");

    auto ret = engine.resolve(coord);

    std::cout << "Result: " << "line - " << std::get<0>(*ret) << " column -" << std::get<1>(*ret) << std::endl;
    

    return 0;
}