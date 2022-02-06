#include <stack-graph-tree.h>
#include <tree_sitter/api.h>

extern "C" TSLanguage *tree_sitter_c();

using std::shared_ptr;
using std::string;
using std::stringstream;
using std::vector;

int main()
{
    // Create a parser.
    TSParser *parser = ts_parser_new();

    // Set the parser's language (JSON in this case).
    ts_parser_set_language(parser, tree_sitter_c());

    // Build a syntax tree based on source code stored in a string.
    const char *source_code = R"raw(

      struct Address {                  // named_scope[Address]
          string line1;                 // symbol[line1]
          string line2;                 // symbol[line2]
      };

      struct Organization {             // named_scope[Organization]
          struct Employee {             // named_scope[emp]
              string name;              // symbol[name]
              string surname;           // symbol[surname]
          } emp;

          int type;                     // symbol[type]
          struct Address addr;          // symbol[addr]
      };

      int fact(int x){                  // named_scope(fact), symbol[x]
          if(x <= 1){                   // symbol_reference[x]
              return 1;
          }
          else return fact(x-1);        // symbol_reference[x]
      };

      int main(int argc, char* argv[]){
          int x = 1;                    // symbol[x]

          int y = fact(10);             // symbol[y], named_scope_reference[fact]

          struct Organization org;      // symbol[org]
          org.emp.name = "Dominik";     // symbol_reference[org] named_scope_references[Organization] symbol_reference[emp] 
                                        // named_scope_reference[emp] symbol_reference[name]
      }
  )raw";

    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        source_code,
        strlen(source_code));

    TSNode root_node = ts_tree_root_node(tree);

    auto sg_root = stack_graph::build_stack_graph_tree(root_node, source_code);

    std::cout << sg_root->repr() << std::endl;

    return 0;
}