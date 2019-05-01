/*
 * database.c
 *
 *  Created on: 2017年6月15日
 *      Author: cj
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"
#include "database.h"

static sqlite3 *db = NULL;

//获取databse中保存的版本
static int database_get_db_version(char version[100]) {
	sqlite3_stmt *stmt;
	int rt = 0;

	const char *sql = "select id,version from db_version where id=0";
	if (NULL == db) {
		return 0;
	}
	database_exec("CREATE TABLE IF NOT EXISTS db_version (id integer primary key, version varchar(30))");
	rt = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	rt = sqlite3_step(stmt);
	if (rt == SQLITE_ROW) { //if all is while
		//sqlite3_column_int(stmt, 0);
		strcpy(version, (char*) sqlite3_column_text(stmt, 1));
	} else if (rt == SQLITE_DONE) {

	} else {
		//error
		DB_ERR(rt, sql);
	}
	sqlite3_finalize(stmt);
	return rt;
}

//数据库备份
static void database_backup(const char *db_file) {
	char cmd[1024];
	memset(cmd, 0, sizeof(cmd));
	log_i(" 备份数据库 ############## revert database.");

	if (tools_file_exists(db_file)) {
		database_close();

		sprintf(cmd, "%s.bk", db_file);
		remove(cmd); //册除上次备份
		memset(cmd, 0, sizeof(cmd));
		//copy
		sprintf(cmd, "cp %s  %s.bk", db_file, db_file);
		system(cmd);

		sqlite3_open(db_file, &db);
	}
}
//数据库恢复
static void database_revert(const char *db_file) {
	char tmpFile[100];
	sprintf(tmpFile, "%s.bk", db_file);
	log_i(" 恢复数据库 ############## revert database.");
	if (tools_file_exists(db_file)) {
		//删除SQLITE文件
		database_close();

		remove(db_file);
		rename(tmpFile, db_file);
		//再次打开
		sqlite3_open(db_file, &db);
	}
}

//数据库软件版本验证
static void database_version_check(const char *db_file) {
	char version[100];

	memset(version, 0, sizeof(version));
	database_get_db_version(version);

	//在版本数据库中添加id=1的数据,测试数据库是否可写, SQLITE_CORRUPT = image is malformed,
	//无法认识的映像 用!=OK
	if ( SQLITE_OK != database_exec_args("REPLACE INTO db_version (id,version) values (1,'%s')", APP_VERSION)) {
		log_d("database is error ,please revert database.");
		database_revert(db_file); //回到上一版本
	}
	database_backup(db_file); //备份

	memset(version, 0, sizeof(version));
	database_get_db_version(version);

	//替换db_version中的版本为当前版本
	database_exec_args("REPLACE INTO db_version (id,version) values (0,'%s')", APP_DB_VERSION);

	tb_rooms_create(version);
	tb_zdevices_create(version);
	dao_scenes_create(version);
	dao_associate_create(version);
	dao_dtimer_create(version);

	if (strlen(version) == 0) {
		//这是新版本，刚刚创建的数据库文件

	} else if (strcmp(version, APP_DB_VERSION) != 0) {
		log_d("新版本-更新数据库表");
		tb_rooms_update(version, APP_DB_VERSION);
	}

	log_d("数据库-本版本:%s", version);

//	//替换db_version中的版本为当前版本
//	database_exec_args("REPLACE INTO db_version (id,version) values (0,'%d')",
//	APP_VERSION);
}

void database_init(const char *db_file) {
	int rt = 0;
	log_d("database:%s", db_file);
	if (0 == tools_file_exists(db_file)) {
		log_i("------ database new.  %s", db_file);
	}

	rt = sqlite3_open(db_file, &db);
	if (rt != SQLITE_OK) {
		log_e("database not open.......");
	}

	database_version_check(db_file); //版本检查
}

//关闭数据库
void database_close() {
	if (db != NULL)
		sqlite3_close(db);
	db = NULL;
}

sqlite3 *database_getdb() {
	return db;
}

//判断表是否存在
int database_table_exists(const char *table) {
	char sql[100];
	sprintf(sql, "SELECT COUNT(*) FROM sqlite_master where type='table' and name='%s'", table);
	return database_first_column_int(sql);
}

//判断表字段是否存在
int database_table_column_exists(const char *table, const char *column_name) {
	sqlite3_stmt *stmt;
	int rc = 0;
	int exists = 0;
	char sql[100];
	sprintf(sql, "select * from %s  limit 1", table);
	//rc = sqlite3_prepare_v2(db, "select * from sqlite_master where type = 'table'", -1,&stmt, 0);
	//rc = sqlite3_prepare_v2(db, "PRAGMA table_info([db_version])", -1,&stmt, 0);
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return 0;
	}
	int i = 0, count = sqlite3_column_count(stmt);
	for (i = 0; i < count; i++) {
		if (strcmp(sqlite3_column_name(stmt, i), column_name) != 0)
			continue;
		exists = 1;
	}
	sqlite3_finalize(stmt);
	return exists;
}

//用户自定义版本 id>=2
int database_get_userversion(int id, char version[30]) {
	sqlite3_stmt *stmt;
	int rt = 0;

	const char *sql = "select id,version from db_version where id=?";
	if (NULL == db) {
		return -1;
	}
	rt = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	rt = sqlite3_bind_int(stmt, 1, id);
	rt = sqlite3_step(stmt);
	if (rt == SQLITE_ROW) { //if all is while
		strcpy(version, (char*) sqlite3_column_text(stmt, 1));
	} else if (rt == SQLITE_DONE) {
		//not find
	} else {
		//error
		DB_ERR(rt, sql);
	}
	sqlite3_finalize(stmt);
	return rt;
}

//用户自定义版本 id >2 version<30
int database_set_userversion(int id, const char *version) {
	return database_exec_args("REPLACE INTO db_version (id,version) values (%d,%Q)", id, version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
int database_exec(const char *sql) {
	char *pErrMsg = NULL;
	int rt = 0;
	log_d("SQL:%s", sql);
	rt = sqlite3_exec(db, sql, NULL, NULL, &pErrMsg);
	if (rt != SQLITE_OK) {
		sqlite3_free(pErrMsg);
		DB_ERR(rt, sql);
	}
	return rt;
}
;

//返回sqlite3的错误码
int database_exec_args(const char *sqlFormat, ...) {
	char *pErrMsg = NULL;
	char *sql = NULL;
	int rt = 0;
	va_list ll;

	va_start(ll, sqlFormat);
	sql = sqlite3_vmprintf(sqlFormat, ll);
	log_d("SQL:%s", sql);
	rt = sqlite3_exec(db, sql, NULL, NULL, &pErrMsg);
	if (rt != SQLITE_OK) {
		sqlite3_free(pErrMsg);
		DB_ERR(rt, sql);
	}
	sqlite3_free(sql);
	va_end(ll);
	return rt;
}

//获取第一列的第一个返回数据,int类型的, 如: select count(*) form ...
int database_first_column_int(const char *sqlFormat, ...) {
	int count = 0;
	char *sql = NULL;
	sqlite3_stmt *stmt;
	int rt = 0;
	va_list ll;

	if (NULL == db) {
		return count;
	}

	va_start(ll, sqlFormat);
	sql = sqlite3_vmprintf(sqlFormat, ll);
	va_end(ll);

	//log_d("SQL:%s", sql);
	rt = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	rt = sqlite3_step(stmt);
	if (rt == SQLITE_ROW) { //if all is while
		count = sqlite3_column_int(stmt, 0);
	} else if (rt == SQLITE_DONE) {
		count = 0;
	} else {

	}
	sqlite3_finalize(stmt);
	sqlite3_free(sql);
	return count;
}

void database_exception_handler(const char *file, const char *function, int line, int rt, const char *sql) {
	if (rt == SQLITE_OK || SQLITE_ROW == rt || SQLITE_DONE == rt)
		return;
	//不须要释放sqlite3_errmsg, sqlite3内部是静态数组
	log_printf(L_WRING, file, function, line, ":DB:%d->%s:%s", rt, sql, sqlite3_errmsg(db));
	if (rt == SQLITE_CORRUPT) { //sqlite数据库出错
		//数据库出错了,怎么处理呢?
	}
}

/**
 * 数据库转json array
 */
int database_select_to_jsonarray(cJSON *jsonArray, const char *sqlformat, ...) {
	sqlite3_stmt *stmt;
	int rt = 0, i = 0, j = 0, columnType;
	const char *name = NULL;
	cJSON *json = NULL;
	char *sql = NULL;
	va_list ll;

	if (db == NULL) {
		return -1;
	}

	va_start(ll, sqlformat);
	sql = sqlite3_vmprintf(sqlformat, ll);
	va_end(ll);

	rt = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (SQLITE_OK != rt) {
		goto exit;
	}

	while ((rt = sqlite3_step(stmt)) == SQLITE_ROW) {
		json = cJSON_CreateObject();
		j = sqlite3_column_count(stmt);

		for (i = 0; i < j; i++) {
			name = sqlite3_column_name(stmt, i);
			columnType = sqlite3_column_type(stmt, i);
			switch (columnType) {
			case SQLITE_FLOAT:
				cJSON_AddNumberToObject(json, name, sqlite3_column_double(stmt, i));
				break;
			case SQLITE_INTEGER:
				cJSON_AddNumberToObject(json, name, sqlite3_column_int64(stmt, i));
				break;
			case SQLITE_TEXT: //3
				cJSON_AddStringToObject(json, name, (const char * )sqlite3_column_text(stmt, i));
				break;
			case SQLITE_NULL://5
				break;
			case SQLITE_BLOB: //4
				break;
			default:
				break;
			}
		}
		cJSON_AddItemToArray(jsonArray, json);
	}
	if (rt != SQLITE_DONE) {
		rt = -1;
	} else {
		rt = 0;
	}
	exit: if (stmt)
		sqlite3_finalize(stmt);
	if (sql)
		sqlite3_free(sql);
	return rt;
}

int database_select_first_to_jsonobj(cJSON *object, const char *sqlformat, ...) {
	char *sql = NULL;
	sqlite3_stmt *stmt;
	va_list ll;
	int ret = 0;
	int i, j, columnType;
	const char *name = NULL;
	//cJSON *json = NULL;

	if (db == NULL) {
		return -1;
	}

	va_start(ll, sqlformat);
	sql = sqlite3_vmprintf(sqlformat, ll);
	va_end(ll);

	ret = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK) {
		DB_ERR(ret, sql);
		goto exit;
	}
	if ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
		j = sqlite3_column_count(stmt);
		for (i = 0; i < j; i++) {
			name = sqlite3_column_name(stmt, i);
			columnType = sqlite3_column_type(stmt, i);
			switch (columnType) {
			case SQLITE_FLOAT:
				cJSON_AddNumberToObject(object, name, sqlite3_column_double(stmt, i));
				break;
			case SQLITE_INTEGER:
				cJSON_AddNumberToObject(object, name, sqlite3_column_int64(stmt, i));
				break;
			case SQLITE_TEXT:
				cJSON_AddStringToObject(object, name, (const char * )sqlite3_column_text(stmt, i));
				break;
			case SQLITE_NULL:
				break;
			case SQLITE_BLOB:
				break;
			default:
				break;
			}
		}
	}

	if (ret == SQLITE_ROW)
		ret = 0;

	exit: if (stmt)
		sqlite3_finalize(stmt);

	if (sql)
		sqlite3_free(sql);

	return ret;
}
