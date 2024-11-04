/* Copyright (c) 2021OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/insert_stmt.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

InsertStmt::InsertStmt(Table *table, std::vector<const Value *> values, std::vector<int> value_amount)
    : table_(table), values_(values), value_amount_(value_amount)
{}

RC InsertStmt::create(Db *db, const InsertSqlNode &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name.c_str();
  if (nullptr == db || nullptr == table_name || inserts.values.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, value_num=%d",
        db, table_name, static_cast<int>(inserts.values.size()));
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  std::vector<const Value *> valuess;
  std::vector<int>           vector_nums;
  const TableMeta           &table_meta = table->table_meta();
  const int                  field_num  = table_meta.field_num() - table_meta.sys_field_num();
  for (auto &vs : inserts.values) {
    const Value *values    = vs.data();
    const int    value_num = static_cast<int>(vs.size());
    if (field_num != value_num) {
      LOG_WARN("schema mismatch. value num=%d, field num in schema=%d", value_num, field_num);
      return RC::SCHEMA_FIELD_MISSING;
    }
    for (int i = 0; i < value_num; i++) {
      const FieldMeta *field_meta = table_meta.field(i + table_meta.sys_field_num());
      if (values[i].attr_type() == AttrType::NULLS && field_meta->nullable())
        continue;
      if (values[i].attr_type() != field_meta->type())
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
      if (values[i].attr_type() == AttrType::CHARS && values[i].length() > field_meta->len())
        return RC::INVALID_ARGUMENT;
    }
    valuess.emplace_back(values);
    vector_nums.emplace_back(value_num);
  }

  stmt = new InsertStmt(table, valuess, vector_nums);
  return RC::SUCCESS;
}
