

#include <gtest/gtest.h>
#include <stack-graph-tree.h>
#include <stack-graph-engine.h>
#include <vector>
#include <iostream>

using stack_graph::StackGraphNode;
using stack_graph::StackGraphEngine;
using stack_graph::Coordinate;




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

TEST(StackGraphEngine, ResolvesReferenceCrossFile)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);
  engine.crossLink();

  auto resolution = engine.resolve(Coordinate("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/main.c", 10, 4));

  ASSERT_EQ("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/def1.h", std::get<0>(*resolution));
  ASSERT_EQ(3, std::get<1>(*resolution));
  ASSERT_EQ(11, std::get<2>(*resolution));
}

TEST(StackGraphEngine, WorksForSymbolsAsWell)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);
  engine.crossLink();

  auto resolution = engine.resolve(Coordinate("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/main.c", 6, 11));

  ASSERT_EQ("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/def2.h", std::get<0>(*resolution));
  ASSERT_EQ(6, std::get<1>(*resolution));
  ASSERT_EQ(7, std::get<2>(*resolution));
}


TEST(StackGraphEngine, ResolvesTypeUses)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);
  engine.crossLink();

  auto results = engine.findUsages(Coordinate("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/def2.h", 6, 7));

  ASSERT_EQ(4, results.size());
  for(auto & res : results){
      std::cout<< std::get<0>(*res) << " "<< std::get<1>(*res) << " " << std::get<2>(*res) << " " << std::endl;
  }
}

TEST(StackGraphEngine, ResolvesSymbolUsages)
{
  auto path = "/home/dominik/Code/intellisense/c-language-server/corpus/sample2";
  StackGraphEngine engine;

  engine.loadDirectoryRecursive(path);
  engine.crossLink();

  auto results = engine.findUsages(Coordinate("/home/dominik/Code/intellisense/c-language-server/corpus/sample2/main.c", 6, 24));

  ASSERT_EQ(1, results.size());
  for(auto & res : results){
      std::cout<< std::get<0>(*res) << " "<< std::get<1>(*res) << " " << std::get<2>(*res) << " " << std::endl;
  }
}




