#include <windows.h>
#include <stdio.h>
#include "sqlite_driver.h"
#include "sqlite3.h"

SqliteConnection::SqliteConnection() {
	mError = NULL;
	mErrorLen = 0;
	mObj = NULL;
	mAutoCommit = true;
	mClosed = true;
}

Statement * SqliteConnection::createStatement() {
	return new SqlitePreparedStatement(this);
}

PreparedStatement * SqliteConnection::prepareStatement(const char *sql) {
	SqlitePreparedStatement *stmt = new SqlitePreparedStatement(this, sql);
	if (stmt->prepare()) {
		return stmt;
	}
	delete stmt;
	return NULL;
}

bool SqliteConnection::getAutoCommit() {
	return mAutoCommit;
}

void SqliteConnection::setAutoCommit(bool autoCommit) {
	if (autoCommit == mAutoCommit) {
		return;
	}
	char *errMsg = NULL;
	if (mAutoCommit) {
		int err = sqlite3_exec(mObj, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
		if (err == SQLITE_OK) {
			setError(NULL);
		}
	} else {
		int err = sqlite3_exec(mObj, "COMMIT", NULL, NULL, &errMsg);
		if (err == SQLITE_OK) {
			setError(NULL);
		}
	}
	autoCommit = mAutoCommit;
}

bool SqliteConnection::commit() {
	if (mAutoCommit) {
		return false;
	}
	char *errMsg = NULL;
	int err = sqlite3_exec(mObj, "COMMIT", NULL, NULL, &errMsg);
	if (err == SQLITE_OK) {
		setError(NULL);
	} else {
		setError(errMsg);
	}
	return err == SQLITE_OK;
}

bool SqliteConnection::rollback() {
	if (mAutoCommit) {
		return false;
	}
	char *errMsg = NULL;
	int err = sqlite3_exec(mObj, "ROLLBACK", NULL, NULL, &errMsg);
	if (err == SQLITE_OK) {
		setError(NULL);
	} else {
		setError(errMsg);
	}
	return err == SQLITE_OK;
}

bool SqliteConnection::isClosed() {
	return mClosed;
}

void SqliteConnection::close() {
	if (! mClosed) {
		mClosed = true;
		sqlite3_close(mObj);
	}
}

char * SqliteConnection::getError() {
	return mError;
}

SqliteConnection::~SqliteConnection() {
	if (mError != NULL) {
		free(mError);
	}
	close();
}

void SqliteConnection::setError(const char *err) {
	if (err == NULL) {
		err = "";
	}
	int len = strlen(err);
	if (len > mErrorLen || mErrorLen == 0) {
		mError = (char *)realloc(mError, len + 1);
		mErrorLen = len;
	}
	strcpy(mError, err);
}

bool SqliteConnection::connect(const char *file) {
	int err = sqlite3_open(file, &mObj);
	if (err != SQLITE_OK) {
		setError("Open db file fail");
		mClosed = false;
		return false;
	}
	mClosed = false;
	return true;
}

void SqliteConnection::setSqliteError() {
	setError(sqlite3_errmsg(mObj));
}

//--------------------SqlitePreparedStatement-------------------------
SqlitePreparedStatement::SqlitePreparedStatement(SqliteConnection *con, const char *sql) {
	mCon = con;
	if (sql != NULL) {
		int len = strlen(sql) + 1;
		mSql = (char *)malloc(len);
		strcpy(mSql, sql);
	} else {
		mSql = NULL;
	}
	mStmtObj = NULL;
	mClosed = true;
	mNeedReset = false;
}

ResultSet * SqlitePreparedStatement::executeQuery(const char *sql) {
	if (sql == NULL) {
		mCon->setError("Query sql is NULL");
		return NULL;
	}
	int len = strlen(sql) + 1;
	mSql = (char *)malloc(len);
	strcpy(mSql, sql);
	if (! prepare()) {
		return NULL;
	}
	return executeQuery();
}

ResultSet* SqlitePreparedStatement::executeQuery() {
	reset();
	mNeedReset = true;
	return new SqliteResultSet(this);
}

int SqlitePreparedStatement::executeUpdate(const char *sql) {
	char *errs = NULL;
	int err = sqlite3_exec(mCon->mObj, sql, NULL, NULL, &errs);
	if (err == SQLITE_OK) {
		mCon->setError(NULL);
		return sqlite3_changes(mCon->mObj);
	}
	mCon->setError(errs);
	return -1;
}

int SqlitePreparedStatement::executeUpdate() {
	reset();
	mNeedReset = true;
	int err = sqlite3_step(mStmtObj);
	if (err == SQLITE_OK || err == SQLITE_DONE) {
		mCon->setError(NULL);
		return sqlite3_changes(mCon->mObj);
	}
	mCon->setSqliteError();
	return -1;
}

bool SqlitePreparedStatement::prepare() {
	mClosed = true;
	if (sqlite3_prepare(mCon->mObj, mSql, strlen(mSql), &mStmtObj, NULL) == SQLITE_OK) {
		mClosed = false;
		mCon->setError(NULL);
		return true;
	}
	mCon->setSqliteError();
	return false;
}

SqlConnection * SqlitePreparedStatement::getConnection() {
	return mCon;
}

bool SqlitePreparedStatement::isClosed() {
	return mClosed;
}

void SqlitePreparedStatement::close() {
	if (! mClosed) {
		mClosed = true;
		sqlite3_finalize(mStmtObj);
	}
}

ResultSetMetaData * SqlitePreparedStatement::getMetaData() {
	return new SqliteResultSetMetaData(mStmtObj);
}

int SqlitePreparedStatement::getParameterCount() {
	return sqlite3_bind_parameter_count(mStmtObj);
}

int SqlitePreparedStatement::getParameterType(int paramIdx) {
	// TODO:
	return 0;
}

void SqlitePreparedStatement::setBlob(int parameIdx, void *blob, int len) {
	reset();
	sqlite3_bind_blob(mStmtObj, parameIdx + 1, blob, len, NULL);
}

void SqlitePreparedStatement::setDouble(int parameIdx, double x) {
	reset();
	sqlite3_bind_double(mStmtObj, parameIdx + 1, x);
}

void SqlitePreparedStatement::setFloat(int parameIdx, float x) {
	reset();
	sqlite3_bind_double(mStmtObj, parameIdx + 1, (double)x);
}

void SqlitePreparedStatement::setInt(int parameIdx, int x) {
	reset();
	sqlite3_bind_int(mStmtObj, parameIdx + 1, x);
}

void SqlitePreparedStatement::setInt64(int parameIdx, long long x) {
	reset();
	sqlite3_bind_int64(mStmtObj, parameIdx + 1, x);
}

void SqlitePreparedStatement::setNull(int parameIdx, SqlType sqlType) {
	reset();
	sqlite3_bind_null(mStmtObj, parameIdx + 1);
}

void SqlitePreparedStatement::setString(int parameIdx, const char *str) {
	reset();
	int len = str == NULL ? 0 : strlen(str);
	sqlite3_bind_text(mStmtObj, parameIdx + 1, str, len, NULL);
}

int SqlitePreparedStatement::getInsertId() {
	return (int)sqlite3_last_insert_rowid(mCon->mObj);
}

bool SqlitePreparedStatement::reset() {
	if (! mNeedReset) {
		return true;
	}
	mNeedReset = false;
	if (sqlite3_clear_bindings(mStmtObj) != SQLITE_OK || sqlite3_reset(mStmtObj) != SQLITE_OK) {
		mCon->setSqliteError();
		return false;
	}
	return true;
}

SqlitePreparedStatement::~SqlitePreparedStatement() {
	close();
	if (mSql != NULL) {
		free(mSql);
	}
}

//------------------------SqliteResultSet------------------
SqliteResultSet::SqliteResultSet(SqlitePreparedStatement *stmt) {
	mStmt = stmt;
	mRow = -1;
}

int SqliteResultSet::findColumn(const char *columnLabel) {
	if (columnLabel == NULL || *columnLabel == 0) {
		return -1;
	}
	int col = sqlite3_column_count(mStmt->mStmtObj);
	for (int i = 0; i < col; ++i) {
		char *name = (char *)sqlite3_column_name(mStmt->mStmtObj, i);
		if (strcmp(name, columnLabel) == 0) {
			return i;
		}
	}
	return -1;
}

void * SqliteResultSet::getBlob(int columnIndex, int *len) {
	if (len != NULL) {
		*len = sqlite3_column_bytes(mStmt->mStmtObj, columnIndex);
	}
	return (void *)sqlite3_column_blob(mStmt->mStmtObj, columnIndex);
}

double SqliteResultSet::getDouble(int columnIndex) {
	return sqlite3_column_double(mStmt->mStmtObj, columnIndex);
}

float SqliteResultSet::getFloat(int columnIndex) {
	return (float)getDouble(columnIndex);
}

int SqliteResultSet::getInt(int columnIndex) {
	return sqlite3_column_int(mStmt->mStmtObj, columnIndex);
}

long long SqliteResultSet::getInt64(int columnIndex) {
	return sqlite3_column_int64(mStmt->mStmtObj, columnIndex);
}

char * SqliteResultSet::getString(int columnIndex) {
	return (char *)sqlite3_column_text(mStmt->mStmtObj, columnIndex);
}

bool SqliteResultSet::next() {
	int err = sqlite3_step(mStmt->mStmtObj);
	if (err == SQLITE_ROW) {
		++mRow;
		return true;
	}
	if (err == SQLITE_DONE) {
		mStmt->mCon->setError(NULL);
		return false;
	}
	mStmt->mCon->setSqliteError();
	return false;
}

int SqliteResultSet::getRow() {
	return mRow;
}

Statement * SqliteResultSet::getStatement() {
	return mStmt;
}

ResultSetMetaData * SqliteResultSet::getMetaData() {
	return new SqliteResultSetMetaData(mStmt->mStmtObj);
}

void SqliteResultSet::close() {
	// Nothing to do
}

bool SqliteResultSet::isClosed() {
	return false;
}

SqliteResultSet::~SqliteResultSet() {
}

//----------------------SqliteResultSetMetaData---------------------
SqliteResultSetMetaData::SqliteResultSetMetaData(sqlite3_stmt *stmt) {
	mStmtObj = stmt;
}

int SqliteResultSetMetaData::getColumnCount() {
	return sqlite3_column_count(mStmtObj);
}

char * SqliteResultSetMetaData::getColumnLabel(int column) {
	return (char *)sqlite3_column_name(mStmtObj, column);
}

char * SqliteResultSetMetaData::getColumnName(int column) {
	return getColumnLabel(column);
	// return (char *)sqlite3_column_origin_name(mStmt, column);
}

SqlType SqliteResultSetMetaData::getColumnType(int column) {
	int type = sqlite3_column_type(mStmtObj, column);
	switch (type) {
	case SQLITE_INTEGER:
		return SQL_TYPE_INT;
	case SQLITE_FLOAT:
		return SQL_TYPE_DOUBLE;
	case SQLITE_BLOB:
		return SQL_TYPE_BLOB;
	case SQLITE_NULL:
		return SQL_TYPE_NULL;
	case SQLITE_TEXT:
		return SQL_TYPE_CHAR;
	}
	return SQL_TYPE_NONE;
}

SqliteResultSetMetaData::~SqliteResultSetMetaData() {

}
