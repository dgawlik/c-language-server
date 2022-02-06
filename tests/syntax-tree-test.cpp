#include <gtest/gtest.h>
#include <stack-graph-tree.h>
#include <vector>

using stack_graph::StackGraphNode;
using stack_graph::StackGraphNodeKind;
using std::vector;

extern "C" TSLanguage *tree_sitter_c();

void _enumerate_nodes(shared_ptr<StackGraphNode> node, vector<shared_ptr<StackGraphNode>> &lst)
{
  lst.push_back(node);

  for (auto ch : node->children)
  {
    _enumerate_nodes(ch, lst);
  }
}

class StackGraphTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
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

    sg_root = stack_graph::build_stack_graph_tree(root_node, source_code);
    _enumerate_nodes(sg_root, all_nodes);
  }

  shared_ptr<StackGraphNode> sg_root;
  vector<shared_ptr<StackGraphNode>> all_nodes;
};

shared_ptr<StackGraphNode> _find(vector<shared_ptr<StackGraphNode>> nodes, string name, StackGraphNodeKind kind)
{
  for (auto n : nodes)
  {
    if (n->kind == kind && n->symbol == name)
    {
      return n;
    }
  }
  return nullptr;
}

bool _contains(vector<shared_ptr<StackGraphNode>> nodes, string name, StackGraphNodeKind kind)
{
   return _find(nodes, name, kind) != nullptr;
}


TEST_F(StackGraphTest, ContainsAllNamedScopes)
{
  EXPECT_TRUE(_contains(all_nodes, "Address", StackGraphNodeKind::NAMED_SCOPE));
  EXPECT_TRUE(_contains(all_nodes, "Organization", StackGraphNodeKind::NAMED_SCOPE));
  EXPECT_TRUE(_contains(all_nodes, "Employee", StackGraphNodeKind::NAMED_SCOPE));
  EXPECT_TRUE(_contains(all_nodes, "fact", StackGraphNodeKind::NAMED_SCOPE));
  EXPECT_TRUE(_contains(all_nodes, "main", StackGraphNodeKind::NAMED_SCOPE));
}

TEST_F(StackGraphTest, ContainsAllSymbols)
{
  EXPECT_TRUE(_contains(all_nodes, "line1", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "line2", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "name", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "surname", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "emp", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "type", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "addr", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "x", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "argc", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "argv", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "y", StackGraphNodeKind::SYMBOL));
  EXPECT_TRUE(_contains(all_nodes, "org", StackGraphNodeKind::SYMBOL));
}

TEST_F(StackGraphTest, ContainsAllReferences)
{
  EXPECT_TRUE(_contains(all_nodes, "x", StackGraphNodeKind::REFERENCE));
  EXPECT_TRUE(_contains(all_nodes, "fact", StackGraphNodeKind::REFERENCE));
  EXPECT_TRUE(_contains(all_nodes, "org.emp.name", StackGraphNodeKind::REFERENCE));
}

TEST_F(StackGraphTest, JumpsAreCorrect)
{
  auto emp_node = _find(all_nodes, "emp", StackGraphNodeKind::SYMBOL);
  EXPECT_EQ(emp_node->jump_to->symbol, "Employee");

  auto addr_node = _find(all_nodes, "addr", StackGraphNodeKind::SYMBOL);
  EXPECT_EQ(addr_node->jump_to->symbol, "Address");

  auto org_node = _find(all_nodes, "org", StackGraphNodeKind::SYMBOL);
  EXPECT_EQ(org_node->jump_to->symbol, "Organization");
}



