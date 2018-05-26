#pragma once
#include "../SqlDriver.h"
class MysqlConnection;
class MysqlPrepareStatement;

enum ColumnType {
	CT_DECIMAL, CT_TINY, CT_SHORT, CT_LONG, CT_FLOAT,  CT_DOUBLE, CT_NULL, CT_TIMESTAMP,
	CT_LONGLONG,CT_INT24, CT_DATE, CT_TIME, CT_DATETIME, CT_YEAR,
	CT_NEWDATE, CT_VARCHAR, CT_BIT, CT_NEWDECIMAL=246, CT_ENUM=247, CT_SET=248,
	CT_TINY_BLOB=249, CT_MEDIUM_BLOB=250, CT_LONG_BLOB=251, CT_BLOB=252,
	CT_VAR_STRING=253, CT_STRING=254, CT_GEOMETRY=255
};

class MysqlResultSetMetaData : public ResultSetMetaData {
public:
	MysqlResultSetMetaData(void *obj, bool freeObj);
	~MysqlResultSetMetaData();

	int getColumnDisplaySize(int column); // 列宽(显示数)
	int getColumnSize(int column); // 列宽(字节数)
	ColumnType getColumnType_2(int column);

	virtual int getColumnCount();
	virtual char *getColumnLabel(int column);
	virtual char *getColumnName(int column);
	virtual SqlType getColumnType(int column);
private:
	void *mResObj;
	bool mFreeObj;
};

class MysqlResultSet : public ResultSet {
public:
	MysqlResultSet(Statement *stmt, void *resObj);
	~MysqlResultSet();
	int getRowsCount(); // 总行数
	unsigned long * getColumnsLength(); // 当前行各列的数据长度, 返回一个数组（每项是一列的长度）

	virtual int findColumn(const char *columnLabel);
	virtual void *getBlob(int columnIndex, int *len);
	virtual double getDouble(int columnIndex);
	virtual float getFloat(int columnIndex);
	virtual int getInt(int columnIndex);
	virtual long long getInt64(int columnIndex);
	virtual char *getString(int columnIndex);

	virtual bool next();
	virtual int getRow();
	virtual Statement *getStatement();
	virtual ResultSetMetaData *getMetaData();
	virtual void close();
	virtual bool isClosed();
private:
	void *mResObj;
	void *mResRow;
	int mRow;
	Statement *mStmt;
	bool mClosed;
};

class MysqlPrepareResultSet : public ResultSet {
public:
	MysqlPrepareResultSet(MysqlPrepareStatement *stmt);
	virtual int findColumn(const char *columnLabel);
	virtual void *getBlob(int columnIndex, int *len);
	virtual double getDouble(int columnIndex);
	virtual float getFloat(int columnIndex);
	virtual int getInt(int columnIndex);
	virtual long long getInt64(int columnIndex);
	virtual char *getString(int columnIndex);

	virtual bool next();
	virtual int getRow();
	virtual Statement *getStatement();
	virtual ResultSetMetaData *getMetaData();
	virtual void close();
	virtual bool isClosed();
protected:
	MysqlPrepareStatement *mStmt;
	int mRow;
	bool mClosed;
};

class MysqlStatement : public Statement {
public:
	MysqlStatement(MysqlConnection *con);
	virtual ResultSet *executeQuery(const char *sql);
	virtual int executeUpdate(const char *sql);
	virtual int getInsertId();
	virtual SqlConnection *getConnection();
	// virtual ResultSet *getResultSet();
	virtual bool isClosed();
	virtual void close();
protected:
	MysqlConnection *mCon;
};

class MysqlPrepareStatement : public PreparedStatement {
public:
	MysqlPrepareStatement(void *obj, MysqlConnection *con);
	~MysqlPrepareStatement();

	virtual SqlConnection *getConnection();
	// virtual ResultSet *getResultSet();
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

	void setParam(int paramIdx, ColumnType ct, void *val, int len);
	bool sendBLOB(int paramIdx, void *data, int length); // 调用之前，所有的params必需先设好

	virtual int getInsertId();
	int getColumnCount();
	int getRowsCount();
	const char* getStmtError();

	bool reset(bool clearBLOB = 0);
	bool exec();
	bool fetch();

	char *getString(int columnIndex);
	int getInt(int columnIndex);
	long long getInt64(int columnIndex);
	double getDouble(int columnIndex);
	void *getRow(int columnIndex, int *len = NULL);
	
protected:
	void setResult(int colIdx, ColumnType ct, int maxLen);
	void checkReset();
protected:
	void *mStmtObj;
	void *mParams;
	void *mResults;
	class Buffer;
	Buffer *mParamBuf;
	Buffer *mResBuf;
	bool mHasBindParam;
	bool mHasBindResult;
	int mParamsCount;
	int mResultColCount;
	MysqlConnection *mCon;
	bool mNeedReset;
	bool mColsed;
	friend class MysqlPrepareResultSet;
};

class MysqlConnection : public SqlConnection {
public:
	MysqlConnection();

	virtual Statement *createStatement();
	virtual PreparedStatement *prepareStatement(const char *sql);
	virtual bool getAutoCommit();
	virtual void setAutoCommit(bool autoCommit);
	virtual bool isClosed();
	virtual void close();
	virtual char *getError();
	bool connect(const char *host, int port, const char *db, const char *usrName, const char *password);
	virtual ~MysqlConnection();

	virtual void setError(const char *err);
	void saveError();
	bool selectDatabase(const char *db);
	
	int getAffectedRows();
	int getInsertId();
	
	bool exec(const char *sql);
	
	bool commit();
	bool rollback();

	// 为当前连接返回默认的字符集
	const char *getCharset();
	bool setCharset(const char *charsetName);

	bool createDatabase(const char *dbName);
	// mysql_use_result -> mysql_fetch_row
	
private:
	void *mMysqlObj;
	char *mError;
	int mErrorLen;
	bool mAutoCommit;
	bool mClosed;
	friend class MysqlStatement;
};
















