

#include <gtest/gtest.h>
#include <stack-graph-tree.h>
#include <stack-graph-engine.h>
#include <vector>

using stack_graph::StackGraphNode;
using stack_graph::StackGraphEngine;




TEST(StackGraphEngine, ScansDirectory)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);

  ASSERT_EQ(4, engine.translation_units.size());
}

TEST(StackGraphEngine, FindsImportsForTU)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);

  ASSERT_EQ("def1.h", engine.importsForTranslationUnit("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/def2.h")[0]);
}

TEST(StackGraphEngine, FindsExportedDefinitions)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);

  auto defs = engine.exportedDefinitionsForTranslationUnit("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/def2.h");

  ASSERT_EQ("Organization", defs[0]->symbol);
}

TEST(StackGraphEngine, FindsSymbolsInTU)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);

  auto symbols = engine.symbolsForTranslationUnit("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/def2.h");

}


