//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//--------------------------------------------------------------------------------------------------

#include "yb/sql/ybsql-test-base.h"
#include "yb/sql/util/base_types.h"
#include "yb/sql/ptree/parse_tree.h"

namespace yb {
namespace sql {

using std::make_shared;
using std::string;
using std::unique_ptr;

class YbSqlTestType : public YbSqlTestBase {
 public:
  //------------------------------------------------------------------------------------------------
  // Test datatypes.
  class MCObjectType : public MCBase {
   public:
    explicit MCObjectType(int id = 0) : id_(id) {
    }

    int id() {
      return id_;
    }

   private:
    int id_;
  };

  //------------------------------------------------------------------------------------------------
  // Test functions.
  YbSqlTestType()
      : YbSqlTestBase(),
        memory_context_(unique_ptr<MemoryContext>(new MemoryContext())) {
  }

  MemoryContext *memory_context() {
    return memory_context_.get();
  }

 private:
  MemoryContext::UniPtr memory_context_;
};

// Testing datatype "MCString" which behaves as std::string but is allocated by the custom
// allocator "MCAllocator<char>".
TEST_F(YbSqlTestType, TestMCString) {
  MemoryContext::UniPtr local_memory_context = unique_ptr<MemoryContext>(new MemoryContext());

  MCString s1(memory_context());
  CHECK_EQ(s1, "");

  MCString s2(memory_context(), 10, 'a');
  CHECK_EQ(s2, "aaaaaaaaaa");
  const char *str2 = "New value of s2";
  s2.assign(str2);
  CHECK_EQ(s2, str2);

  MCString s3(memory_context(), "This is a SQL string");
  CHECK_EQ(s3, "This is a SQL string");
  s3 += " after being postfixed";
  CHECK_EQ(s3, "This is a SQL string after being postfixed");

  MCString s4(memory_context(), "This is to-be-truncated string", 10);
  CHECK_EQ(s4, "This is to");

  const char *str_to_be_compared = "Testing string comparison";
  s4.assign(str_to_be_compared);

  // Compare between strings of the same context.
  MCString::SharedPtr s5 = MCString::MakeShared(memory_context(), str_to_be_compared);
  CHECK_EQ(s4, *s5);

  // Compare between strings of the same context.
  MCString s6(memory_context(), str_to_be_compared);
  CHECK_EQ(s4, s6);

  // Compare between strings of different contexts.
  MCString s7(local_memory_context.get(), str_to_be_compared);
  CHECK_EQ(s4, s7);

  // Allocate from private memory_context().
  const char *str2_to_be_compared = "Testing negative string comparison";
  MCString s8(memory_context(), str2_to_be_compared);
  CHECK_EQ(s8, str2_to_be_compared);

  // Allocate from local.
  MCString s9(local_memory_context.get(), str2_to_be_compared);
  CHECK_EQ(s9, str2_to_be_compared);
  MCString::SharedPtr s10 = MCString::MakeShared(local_memory_context.get(), str2_to_be_compared);
  CHECK_EQ(*s10, str2_to_be_compared);
  CHECK_EQ(s9, *s10);

  // Negative comparisons.
  CHECK_NE(s4, s8);
  CHECK_NE(s4, s9);
  CHECK_NE(s4, *s10);
}

// Test MCVector and MCList.
TEST_F(YbSqlTestType, TestMCStl) {
  int i;
  const int max_entry = 99;

  // Constructing vector.
  MCVector<int> sql_vec(memory_context());
  sql_vec.reserve(max_entry);
  for (i = 0; i < max_entry; i++) {
    sql_vec[i] = i*7;
  }

  // Checking vector.
  i = 0;
  for (int entry : sql_vec) {
    CHECK_EQ(entry, i*7);
    i++;
  }

  // Constructing list.
  MCList<int> sql_list(memory_context());
  for (i = 0; i < max_entry; i++) {
    sql_list.push_back(i*3);
  }

  // Checking list.
  i = 0;
  for (int entry : sql_list) {
    CHECK_EQ(entry, i*3);
    i++;
  }
}

// Test MCObject.
TEST_F(YbSqlTestType, TestMCTreeNode) {
  int i;
  const int max_entry = 99;

  // Construct vector.
  MCVector<MCObjectType *> objects(memory_context());
  objects.reserve(max_entry);
  for (i = 0; i < max_entry; i++) {
    // Make SQL objects that share the same pool with "objects".
    objects[i] = new(memory_context()) MCObjectType(i*9);
  }

  // Checking vector and delete entries.
  i = 0;
  for (MCObjectType *entry : objects) {
    CHECK_EQ(entry->id(), i*9);
    delete entry;
    i++;
  }

  // Construct list.
  MCList<MCObjectType *> object_list(memory_context());
  for (i = 0; i < max_entry; i++) {
    // Make SQL objects that share the same pool with "object_list".
    MCObjectType *entry = new(memory_context()) MCObjectType(i*9);
    object_list.push_back(entry);
  }

  // Checking list.
  i = 0;
  for (MCObjectType *entry : object_list) {
    CHECK_EQ(entry->id(), i*9);
    delete entry;
    i++;
  }
}

}  // namespace sql
}  // namespace yb
