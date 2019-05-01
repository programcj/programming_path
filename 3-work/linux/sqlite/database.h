/*
 * database.h
 *
 *  Created on: 2017年6月15日
 *      Author: cj
 */

#ifndef FILE_SRC_DAO_DATABASE_H_
#define FILE_SRC_DAO_DATABASE_H_

#include "sqlite3.h"

//AUTOINCREMENT

void database_init(const char *db_file);

//关闭数据库
void database_close();

sqlite3 *database_getdb();
/////////////////////////////////////////////////////////////////////////////////////////////////////

//判断表是否存在
int database_table_exists(const char *table);

//判断表字段是否存在，存在返回1
int database_table_column_exists(const char *table, const char *column_name);

/////////////////////////////////////////////////////////////////////////////////////////////////////

int database_exec(const char *sql);

//获取第一列的第一个返回数据,int类型的, 如: select count(*) form ...
int database_first_column_int(const char *sqlFormat, ...);

//返回sqlite3的错误码
int database_exec_args(const char *sqlFormat, ...);

//////////////////////////////////////////////////////////////////////////////////////////////////////
//数据库操作如果出现错位就回到这里处理： 文件名，函数名，行号，rt:sqlite3的返回值, 所操作的sql语句
void database_exception_handler(const char *file, const char *function,
		int line, int rt, const char *sql);

//用户自定义版本 id>=2
int database_get_userversion(int id, char version[30]);

//用户自定义版本 id >2 version<30
int database_set_userversion(int id, const char *version);

//数据库错误时调用这里: errcode: sqlite3的返回值, sql:所操作的sql语句
#define DB_ERR(errcode,sql) database_exception_handler(__FILE__, __FUNCTION__, __LINE__, errcode, sql)

/**
 * 数据库转json array
 */
int database_select_to_jsonarray(cJSON *jsonArray,const char *sqlformat, ...);

int database_select_first_to_jsonobj(cJSON *object, const char *sqlformat, ...);

//sqlite3_stmt的bind_text的改写,不用
//int sbind_text(sqlite3_stmt *stmt, int index, const char *buff);
#define  sqlite3_bind_textex(stmt, index, buff)  sqlite3_bind_text(stmt, index, buff, buff!=NULL?strlen(buff): 0, SQLITE_STATIC)

//int sbind_int(sqlite3_stmt *stmt, int index, int v);
#define sqlite3_bind_intex(stmt,index, v)     sqlite3_bind_int(stmt, index,(int) v)


#define DATABASE_TABLE_CREATE(name)   void name##_create(const char *version)
#define DATABASE_TABLE_UPDATE(name)   void name##_update(const char *oldversion, const char *newversion)

#endif /* FILE_SRC_DAO_DATABASE_H_ */
