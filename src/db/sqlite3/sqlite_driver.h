#pragma once
#include "../SqlDriver.h"

class SqliteConnection;
class SqlitePreparedStatement;
struct sqlite3;
struct sqlite3_stmt;

class SqliteResultSetMetaData : public ResultSetMetaData{
public:
	SqliteResultSetMetaData(sqlite3_stmt *stmt);

	virtual int getColumnCount();

	/************************************************************************/
	/* @param column [0, 1, ...]
	/* @return Don't free 
	/************************************************************************/
	virtual char *getColumnLabel(int column);
	virtual char *getColumnName(int column);
	virtual SqlType getColumnType(int column);

	virtual ~SqliteResultSetMetaData();

	sqlite3_stmt *mStmtObj;
};

class SqliteResultSet : public ResultSet {
public:
	SqliteResultSet(SqlitePreparedStatement *stmt);
	/************************************************************************/
	/* @return [0, 1, ...] or -1 for not find
	/************************************************************************/
	virtual int findColumn(const char *columnLabel);

	/************************************************************************/
	/* @param columnIndex [0, 1, ...]
	/* @param len [in , out] Given the length of ready to read, and return real read length
	/************************************************************************/
	virtual void *getBlob(int columnIndex, int *len);
	virtual double getDouble(int columnIndex);
	virtual float getFloat(int columnIndex);
	virtual int getInt(int columnIndex);
	virtual long long getInt64(int columnIndex);

	/************************************************************************/
	/* @return An string, Don't free 
	/************************************************************************/
	virtual char *getString(int columnIndex);

	virtual bool next();

	/************************************************************************/
	/* Get the current read row number
	/************************************************************************/
	virtual int getRow();
	virtual Statement *getStatement();
	virtual ResultSetMetaData *getMetaData();
	virtual void close();
	virtual bool isClosed();
	virtual ~SqliteResultSet();

	int mRow;
	SqlitePreparedStatement *mStmt;
};

#if 0
class SqliteStatement : public Statement {
public:
	SqliteStatement(SqliteConnection *con);
	virtual ResultSet *executeQuery(const char *sql);
	/ ************************************************************************ /
	/ * @return The number of affected rows when INSERT, UPDATE, DELETE
	/ *           or -1 when fail to execute sql
	/ ************************************************************************ /
	virtual int executeUpdate(const char *sql);

	virtual SqlConnection *getConnection();
	// virtual ResultSet *getResultSet() = 0;
	virtual bool isClosed();
	virtual void close();
	virtual ~SqliteStatement();

	SqliteConnection *mCon;
	SqlitePreparedStatement *mQueryStmt;
};
#endif

class SqlitePreparedStatement : public PreparedStatement {
public:
	SqlitePreparedStatement(SqliteConnection *con, const char *sql = NULL);
	virtual ResultSet *executeQuery(const char *sql);
	/************************************************************************/
	/* @return The number of affected rows when INSERT, UPDATE, DELETE
	/*           or -1 when fail to execute sql
	/************************************************************************/
	virtual int executeUpdate(const char *sql);

	virtual SqlConnection *getConnection();
	// virtual ResultSet *getResultSet() = 0;
	virtual bool isClosed();
	virtual void close();

	virtual ResultSet *executeQuery();
	virtual int executeUpdate();
	virtual ResultSetMetaData *getMetaData();
	
	virtual int getParameterCount();

	virtual int getParameterType(int paramIdx);
	virtual void setBlob(int parameIdx, void *blob, int len);
	virtual void setDouble(int parameIdx, double x);
	virtual void setFloat(int parameIdx, float x);
	virtual void setInt(int parameIdx, int x);
	virtual void setInt64(int parameIdx, long long x);
	virtual void setNull(int parameIdx, SqlType sqlType);
	virtual void setString(int parameIdx, const char *str);

	virtual int getInsertId();

	bool prepare();
	bool reset();

	virtual ~SqlitePreparedStatement();

	SqliteConnection *mCon;
	char *mSql;
	sqlite3_stmt *mStmtObj;
	bool mClosed;
	bool mNeedReset;
};

class SqliteConnection : public SqlConnection {
public:
	SqliteConnection();
	bool connect(const char *file);
	virtual Statement *createStatement();
	virtual PreparedStatement *prepareStatement(const char *sql);
	virtual bool getAutoCommit();
	virtual void setAutoCommit(bool autoCommit);
	virtual bool isClosed();
	virtual void close();
	virtual bool commit();
	virtual bool rollback();

	/************************************************************************/
	/* @return Error info, Not need free
	/************************************************************************/
	virtual char *getError();
	virtual ~SqliteConnection();

	virtual void setError(const char *err);
	void setSqliteError();

	char *mError;
	int mErrorLen;
	sqlite3 *mObj;
	bool mAutoCommit;
	bool mClosed;
};