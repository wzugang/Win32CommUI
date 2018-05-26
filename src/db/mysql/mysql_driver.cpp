#include <windows.h>
#include <stdio.h>
#include "mysql_driver.h"

#include "D:/Program Files/MySQL/MySQL Server 5.1/include/mysql.h"
#pragma comment(lib, "D:/Program Files/MySQL/MySQL Server 5.1/lib/opt/libmysql.lib")

//------------------------MysqlResultSetMetaData-----------------------
MysqlResultSetMetaData::MysqlResultSetMetaData(void *obj, bool freeObj) {
	mResObj = obj;
	mFreeObj = freeObj;
}

int MysqlResultSetMetaData::getColumnSize( int column ) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mResObj, column);
	if (field->type == CT_TINY) return 1;
	if (field->type == CT_SHORT) return 2;
	if (field->type == CT_LONG || field->type == CT_FLOAT) return 4;
	if (field->type == CT_DOUBLE || field->type == CT_LONGLONG) return 8;
	// if (field->type >= CT_TINY_BLOB && field->type <= CT_BLOB) return 0;
	return field->length;
}

int MysqlResultSetMetaData::getColumnDisplaySize( int column ) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mResObj, column);
	return field->length;
}

ColumnType MysqlResultSetMetaData::getColumnType_2( int column ) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mResObj, column);
	return (ColumnType)(field->type);
}

int MysqlResultSetMetaData::getColumnCount() {
	return (int)mysql_num_fields((MYSQL_RES*)mResObj);
}

char* MysqlResultSetMetaData::getColumnLabel(int column) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mResObj, column);
	return field->name;
}

char* MysqlResultSetMetaData::getColumnName(int column) {
	MYSQL_FIELD *field = mysql_fetch_field_direct((MYSQL_RES*)mResObj, column);
	return field->org_name;
}

MysqlResultSetMetaData::~MysqlResultSetMetaData() {
	if (mFreeObj && mResObj)  mysql_free_result((MYSQL_RES*)mResObj);
}

SqlType MysqlResultSetMetaData::getColumnType(int column) {
	// TODO:
	return SQL_TYPE_NONE;
}

// ----------------------MysqlResultSet----------------------------------
MysqlResultSet::MysqlResultSet(Statement *stmt, void *obj) :mResObj(obj) ,mResRow(0) {
	mRow = -1;
	mClosed = false;
	mStmt = stmt;
}

MysqlResultSet::~MysqlResultSet() {
	close();
}

int MysqlResultSet::getRowsCount() {
	return (int)mysql_num_rows((MYSQL_RES*)mResObj);
}

bool MysqlResultSet::next() {
	mResRow = (void*)mysql_fetch_row((MYSQL_RES*)mResObj);
	if (mResRow != NULL) {
		++mRow;
	}
	return mResRow != NULL;
}

char *MysqlResultSet::getString(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mResRow;
	return row[columnIndex];
}

int MysqlResultSet::getInt(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mResRow;
	char *dat = row[columnIndex];
	return atoi(dat);
}

long long MysqlResultSet::getInt64(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mResRow;
	char *dat = row[columnIndex];
	return (long long)strtod(dat, 0);
}

void * MysqlResultSet::getBlob(int columnIndex, int *len) {
	// TODO:
	return NULL;
}

double MysqlResultSet::getDouble(int columnIndex) {
	MYSQL_ROW row = (MYSQL_ROW)mResRow;
	char *dat = row[columnIndex];
	return strtod(dat, 0);
}
float MysqlResultSet::getFloat(int columnIndex) {
	return (float)getDouble(columnIndex);
}
unsigned long * MysqlResultSet::getColumnsLength() {
	return mysql_fetch_lengths((MYSQL_RES*)mResObj);
}

ResultSetMetaData *MysqlResultSet::getMetaData() {
	return new MysqlResultSetMetaData(mResObj, false);
}

int MysqlResultSet::getRow() {
	return mRow;
}

Statement * MysqlResultSet::getStatement() {
	return mStmt;
}

void MysqlResultSet::close() {
	if (mClosed) {
		return;
	}
	mClosed = true;
	if (mResObj) {
		mysql_free_result((MYSQL_RES*)mResObj);
	}
	mResObj = NULL;
}

bool MysqlResultSet::isClosed() {
	return mClosed;
}

int MysqlResultSet::findColumn(const char *columnLabel) {
	// TODO:
	return -1;
}

//-----------------------------------------------------------
class MysqlPrepareStatement::Buffer {
public:
	Buffer(int sz) {
		mLen = 0;
		mCapacity = sz;
		mBuf = 0;
	}
	~Buffer() {
		free(mBuf);
	}
	void createBuf() {
		if (mBuf == 0) mBuf = (char*)malloc(mCapacity);
	}
	char *curBuf() {
		return mBuf + mLen;
	}
	void clear() {
		mLen = 0;
	}
	void inc(int sz) {
		mLen += sz;
	}
	void* append(int v) {
		createBuf();
		char *cur = curBuf();
		memcpy(cur, &v, sizeof(int));
		inc(sizeof(int));
		return cur;
	}
	void* append(long long v) {
		createBuf();
		char *cur = curBuf();
		memcpy(cur, &v, sizeof(long long));
		inc(sizeof(long long));
		return cur;
	}
	void* append(double v) {
		createBuf();
		char *cur = curBuf();
		memcpy(curBuf(), &v, sizeof(double));
		inc(sizeof(double));
		return cur;
	}
	void* append(const char* v) {
		createBuf();
		char *cur = curBuf();
		if (v == NULL) v = "";
		int len = strlen(v) + 1;
		strcpy(cur, v);
		inc(len);
		return cur;
	}
	void* appendLen(int len) {
		createBuf();
		char *cur = curBuf();
		inc(len);
		return cur;
	}

	char *mBuf;
	int mCapacity;
	int mLen;
};

//-------------------------MysqlPrepareStatement----------------------
MysqlPrepareStatement::MysqlPrepareStatement(void *obj, MysqlConnection *con) : mStmtObj(obj) {
	mParamsCount = getParameterCount();
	mResultColCount = getColumnCount();
	int SZ = sizeof(MYSQL_BIND) * mParamsCount;
	mParams = malloc(SZ);
	memset(mParams, 0, SZ);
	SZ = sizeof(MYSQL_BIND) * mResultColCount;
	mResults = malloc(SZ);
	memset(mResults, 0, SZ);
	mParamBuf = new Buffer(256);
	mHasBindParam = FALSE;
	mHasBindResult = FALSE;
	mNeedReset = false;
	mColsed = false;
	mCon = con;

	mResBuf = NULL;
	MysqlResultSetMetaData *rs = (MysqlResultSetMetaData *)getMetaData();
	if (rs == NULL) return;
	int resBufLen = 0;
	for (int i = 0; i < mResultColCount; ++i) {
		ColumnType ct = rs->getColumnType_2(i);
		bool isBlob = ct >= CT_TINY_BLOB && ct <= CT_BLOB;
		resBufLen += rs->getColumnSize(i);
		resBufLen += 16;
	}
	mResBuf = new Buffer(resBufLen);

	for (int i = 0; i < mResultColCount; ++i) {
		ColumnType ct = rs->getColumnType_2(i);
		bool isBlob = ct >= CT_TINY_BLOB && ct <= CT_BLOB;
		setResult(i, ct, rs->getColumnSize(i));
	}
	delete rs;
}

MysqlPrepareStatement::~MysqlPrepareStatement() {
	close();
}

void MysqlPrepareStatement::setInt(int paramIdx, int val) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_LONG;
	b->buffer = mParamBuf->append(val);
}
void MysqlPrepareStatement::setInt64(int paramIdx, long long int val) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_LONGLONG;
	b->buffer = mParamBuf->append(val);
}
void MysqlPrepareStatement::setDouble(int paramIdx, double val) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_DOUBLE;
	b->buffer = mParamBuf->append(val);
}
void MysqlPrepareStatement::setFloat(int paramIdx, float val) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_FLOAT;
	b->buffer = mParamBuf->append(val);
}
void MysqlPrepareStatement::setString(int paramIdx, const char* val) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_STRING;
	b->buffer = (void *)val;
	b->buffer_length = val ? strlen(val) : 0;
}
void MysqlPrepareStatement::setBlob(int paramIdx, void *val, int len) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_BLOB;
	b->buffer = val;
	b->buffer_length = len;
}
void MysqlPrepareStatement::setNull(int paramIdx, SqlType sqlType) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = MYSQL_TYPE_NULL;
	b->buffer = NULL;
	b->buffer_length = 0;
}
void MysqlPrepareStatement::setParam( int paramIdx, ColumnType ct, void *val, int len ) {
	checkReset();
	MYSQL_BIND *b = (MYSQL_BIND*)mParams + paramIdx;
	b->buffer_type = enum_field_types(ct);
	b->buffer = val;
	b->buffer_length = len;
}
int MysqlPrepareStatement::getParameterCount() {
	return (int)mysql_stmt_param_count((MYSQL_STMT*)mStmtObj);
}
void MysqlPrepareStatement::setResult(int colIdx, ColumnType ct, int maxLen) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + colIdx;
	b->buffer_type = enum_field_types(ct);
	b->length = (unsigned long *)mResBuf->appendLen(sizeof(unsigned long *));
	b->buffer_length = maxLen;
	b->buffer = mResBuf->appendLen(maxLen + 2);
}

bool MysqlPrepareStatement::reset(bool clearBLOB) {
	const int SZ = sizeof(MYSQL_BIND) * mParamsCount;
	mNeedReset = false;
	memset(mParams, 0, SZ);
	// mParamBuf->clear();
	// mResBuf->clear();
	mHasBindParam = false;
	mysql_stmt_free_result((MYSQL_STMT*)mStmtObj);
	if (clearBLOB) {
		return mysql_stmt_reset((MYSQL_STMT*)mStmtObj) == 0;
	}
	return TRUE;
}
bool MysqlPrepareStatement::exec() {
	bool ok = true;
	if (mParamsCount > 0 && !mHasBindParam) {
		mHasBindParam = true;
		ok = mysql_stmt_bind_param((MYSQL_STMT*)mStmtObj, (MYSQL_BIND*)mParams) == 0;
	}
	if (ok && mResultColCount > 0) ok = mysql_stmt_bind_result((MYSQL_STMT*)mStmtObj, (MYSQL_BIND*)mResults) == 0;
	if (ok) ok = mysql_stmt_execute((MYSQL_STMT*)mStmtObj) == 0;
	if (ok && mResultColCount > 0) ok = mysql_stmt_store_result((MYSQL_STMT*)mStmtObj) == 0; // mysql_stmt_result_metadata((MYSQL_STMT*)mObj) != NULL
	mNeedReset = false;
	if (! ok) {
		mCon->setError(getStmtError());
	}
	return ok;
}

bool MysqlPrepareStatement::fetch() {
	int cc = mysql_stmt_fetch((MYSQL_STMT*)mStmtObj);
	return cc == 0;
}
char *MysqlPrepareStatement::getString(int columnIndex) {
	static char empty[4] = {0};
	*empty = 0;
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return empty;
	if (b->buffer_type == MYSQL_TYPE_VAR_STRING || b->buffer_type == MYSQL_TYPE_STRING) {
		char *p = (char*)b->buffer;
		unsigned long len = *(b->length);
		p[len] = p[len + 1] = 0;
		return p;
	}
	return empty;
}
int MysqlPrepareStatement::getInt(int columnIndex) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return 0;
	if (b->buffer_type == MYSQL_TYPE_LONG)
		return *(int*)(b->buffer);
	if (b->buffer_type == MYSQL_TYPE_TINY)
		return *(char *)b->buffer;
	if (b->buffer_type == MYSQL_TYPE_SHORT)
		return *(short *)b->buffer;
	if (b->buffer_type == MYSQL_TYPE_LONGLONG)
		return *(long long*)(b->buffer);
	return 0;
}
long long MysqlPrepareStatement::getInt64(int columnIndex) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return 0;
	if (b->buffer_type == MYSQL_TYPE_LONGLONG)
		return *(long long*)(b->buffer);
	return getInt(columnIndex);
}
double MysqlPrepareStatement::getDouble(int columnIndex) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (b->is_null_value)
		return 0;
	if (b->buffer_type == MYSQL_TYPE_DOUBLE)
		return *(double*)(b->buffer);
	if (b->buffer_type == MYSQL_TYPE_FLOAT)
		return *(float*)(b->buffer);
	return 0;
}
void * MysqlPrepareStatement::getRow( int columnIndex, int *len) {
	MYSQL_BIND *b = (MYSQL_BIND*)mResults + columnIndex;
	if (len) *len = *b->length;
	if (b->is_null_value)
		return 0;
	return b->buffer;
}
int MysqlPrepareStatement::getInsertId() {
	return (int)mysql_stmt_insert_id((MYSQL_STMT*)mStmtObj);
}
int MysqlPrepareStatement::getColumnCount() {
	return (int)mysql_stmt_field_count((MYSQL_STMT*)mStmtObj);
}
int MysqlPrepareStatement::getRowsCount() {
	return (int)mysql_stmt_num_rows((MYSQL_STMT*)mStmtObj);
}
const char* MysqlPrepareStatement::getStmtError() {
	return mysql_stmt_error((MYSQL_STMT*)mStmtObj);
}
ResultSetMetaData* MysqlPrepareStatement::getMetaData() {
	void *d = mysql_stmt_result_metadata((MYSQL_STMT*)mStmtObj);
	if (d == NULL) {
		mCon->setError(getStmtError());
		return NULL;
	}
	return new MysqlResultSetMetaData(d, true);
}

bool MysqlPrepareStatement::sendBLOB( int paramIdx, void *data, int length ) {
	bool ok = true;
	if (mParamsCount > 0 && !mHasBindParam) {
		mHasBindParam = true;
		ok = mysql_stmt_bind_param((MYSQL_STMT*)mStmtObj, (MYSQL_BIND*)mParams) == 0;
		if (! ok) return false;
	}
	return mysql_stmt_send_long_data((MYSQL_STMT*)mStmtObj, paramIdx, (const char *)data, length) == 0;
}

ResultSet * MysqlPrepareStatement::executeQuery() {
	mNeedReset = true;
	if (exec()) {
		return new MysqlPrepareResultSet(this);
	}
	mCon->setError(getStmtError());
	return NULL;
}

int MysqlPrepareStatement::executeUpdate() {
	if (exec()) {
		return mCon->getAffectedRows();
	}
	mCon->setError(getStmtError());
	return -1;
}

int MysqlPrepareStatement::getParameterType(int paramIdx) {
	// TODO:
	return 0;
}

void MysqlPrepareStatement::checkReset() {
	if (mNeedReset) {
		reset(true);
	}
}

SqlConnection * MysqlPrepareStatement::getConnection() {
	return mCon;
}

bool MysqlPrepareStatement::isClosed() {
	return mColsed;
}

void MysqlPrepareStatement::close() {
	if (mColsed) {
		return;
	}
	mColsed = true;
	if (mStmtObj) {
		mysql_stmt_free_result((MYSQL_STMT*)mStmtObj);
		mysql_stmt_close((MYSQL_STMT*)mStmtObj);
	}
	delete mParamBuf;
	delete mResBuf;
	mParamBuf = NULL;
	mResBuf = NULL;
}

//-----------MysqlPrepareResultSet------------------
MysqlPrepareResultSet::MysqlPrepareResultSet(MysqlPrepareStatement *stmt) {
	mStmt = stmt;
	mRow = -1;
	mClosed = false;
}

int MysqlPrepareResultSet::findColumn(const char *columnLabel) {
	// TODO:
	return -1;
}

void * MysqlPrepareResultSet::getBlob(int columnIndex, int *len) {
	return mStmt->getRow(columnIndex, len);
}

double MysqlPrepareResultSet::getDouble(int columnIndex) {
	return mStmt->getDouble(columnIndex);
}

float MysqlPrepareResultSet::getFloat(int columnIndex) {
	return (float)getDouble(columnIndex);
}

int MysqlPrepareResultSet::getInt(int columnIndex) {
	return mStmt->getInt(columnIndex);
}

long long MysqlPrepareResultSet::getInt64(int columnIndex) {
	return mStmt->getInt64(columnIndex);
}

char * MysqlPrepareResultSet::getString(int columnIndex) {
	return mStmt->getString(columnIndex);
}

bool MysqlPrepareResultSet::next() {
	bool ok = mStmt->fetch();
	if (ok) ++mRow;
	return ok;
}

int MysqlPrepareResultSet::getRow() {
	return mRow;
}

Statement * MysqlPrepareResultSet::getStatement() {
	return mStmt;
}

ResultSetMetaData * MysqlPrepareResultSet::getMetaData() {
	return mStmt->getMetaData();
}

void MysqlPrepareResultSet::close() {
	mStmt->reset(true);
	mClosed = true;
}

bool MysqlPrepareResultSet::isClosed() {
	return mClosed;
}

// ---------------------MysqlConnection-------------------------------------
MysqlConnection::MysqlConnection() {
	mError = NULL;
	mErrorLen = 0;
	mMysqlObj = malloc(sizeof (MYSQL));
	mAutoCommit = true;
	mClosed = false;
	mysql_init((MYSQL*)mMysqlObj);
}

MysqlConnection::~MysqlConnection() {
	mysql_close((MYSQL*)mMysqlObj);
	free(mMysqlObj);
	if (mError != NULL) {
		free(mError);
	}
}

int MysqlConnection::getAffectedRows() {
	return (int)mysql_affected_rows((MYSQL*)mMysqlObj);
}

int MysqlConnection::getInsertId() {
	return (int)mysql_insert_id((MYSQL*)mMysqlObj);
}

void MysqlConnection::setError(const char *err) {
	if (err == NULL) {
		err = "";
	}
	int len = strlen(err);
	if (mErrorLen < len || mErrorLen == 0) {
		mErrorLen = len;
		mError = (char *)realloc(mError, mErrorLen + 1);
	}
	strcpy(mError, err);
}

bool MysqlConnection::setCharset(const char *charsetName) {
	return mysql_set_character_set((MYSQL*)mMysqlObj, charsetName) == 0;
}

bool MysqlConnection::connect(const char *host, int port, const char *db, const char *usrName, const char *password) {
	void *obj = mysql_real_connect((MYSQL*)mMysqlObj, host, usrName, password, db, port, 0, 0);
	if (obj == NULL) {
		setError(mysql_error((MYSQL*)mMysqlObj));
		return false;
	}
	return true;
}

bool MysqlConnection::selectDatabase(const char *db) {
	return mysql_select_db((MYSQL*)mMysqlObj, db) == 0;
}

void MysqlConnection::close() {
	mClosed = true;
	mysql_close((MYSQL*)mMysqlObj);
}

bool MysqlConnection::exec(const char *sql) {
	return mysql_query((MYSQL*)mMysqlObj, sql) == 0;
}

PreparedStatement *MysqlConnection::prepareStatement(const char *sql) {
	MYSQL_STMT *stmt = mysql_stmt_init((MYSQL*)mMysqlObj);
	int code = mysql_stmt_prepare(stmt, sql, sql == NULL ? 0 : strlen(sql));
	if (code != 0) {
		printf("Mysql::prepare err: %s\n", getError());
		saveError();
		mysql_stmt_close(stmt); // free stmt ?
		return NULL;
	}
	return new MysqlPrepareStatement(stmt, this);
}

void MysqlConnection::setAutoCommit(bool autoMode) {
	my_bool ok = mysql_autocommit((MYSQL*)mMysqlObj, (my_bool)autoMode);
	if (ok) {
		mAutoCommit = autoMode;
	}
}

bool MysqlConnection::commit() {
	if (mAutoCommit) {
		return false;
	}
	return mysql_commit((MYSQL*)mMysqlObj) == 0;
}

bool MysqlConnection::rollback() {
	return mysql_rollback((MYSQL*)mMysqlObj) == 0;
}

const char * MysqlConnection::getCharset() {
	return mysql_character_set_name((MYSQL*)mMysqlObj);
}

bool MysqlConnection::createDatabase( const char *dbName ) {
	char sql[48];
	sprintf(sql, "create database %s ", dbName);
	return mysql_query((MYSQL*)mMysqlObj, sql) == 0;
}

Statement * MysqlConnection::createStatement() {
	return new MysqlStatement(this);
}

bool MysqlConnection::getAutoCommit() {
	return mAutoCommit;
}

void MysqlConnection::saveError() {
	setError(mysql_error((MYSQL*)mMysqlObj));
}

bool MysqlConnection::isClosed() {
	return mClosed;
}

char * MysqlConnection::getError() {
	return mError;
}

//------------------MysqlStatement----------------------------
MysqlStatement::MysqlStatement(MysqlConnection *con) {
	mCon = con;
}

ResultSet * MysqlStatement::executeQuery(const char *sql) {
	MYSQL_RES *res = NULL;
	int code = mysql_query((MYSQL*)mCon->mMysqlObj, sql);
	if (code != 0) {
		mCon->saveError();
		return NULL;
	}
	res = mysql_store_result((MYSQL*)mCon->mMysqlObj); // 一次读取全部数据
	if (res == NULL) {
		mCon->saveError();
		return NULL;
	}
	return new MysqlResultSet(this, (void*)res);

	// mysql_use_result -> mysql_fetch_row  // 每次调用mysql_fetch_row时，才读取数据; 用mysql_errno判断是否读取成功
}

int MysqlStatement::executeUpdate(const char *sql) {
	if (mCon->exec(sql)) {
		return mCon->getAffectedRows();
	}
	return -1;
}

SqlConnection * MysqlStatement::getConnection() {
	return mCon;
}

bool MysqlStatement::isClosed() {
	return false;
}

void MysqlStatement::close() {
	// Nothing to do
}

int MysqlStatement::getInsertId() {
	return mCon->getInsertId();
}

