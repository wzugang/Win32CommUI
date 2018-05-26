#pragma once

class SqlConnection;
class Statement;

enum SqlType {
	SQL_TYPE_NONE, 
	SQL_TYPE_NULL,
	SQL_TYPE_BLOB, SQL_TYPE_DOUBLE, SQL_TYPE_FLOAT, 
	SQL_TYPE_INT, SQL_TYPE_INT64, SQL_TYPE_CHAR
};

class Blob {
public:
	/************************************************************************/
	/* @return the length of blob
	/************************************************************************/
	virtual int getLength();

	/************************************************************************/
	/* @return the length of really read data
	/************************************************************************/
	virtual int read(void *buf, int bufLen) = 0;
protected:
};

class ResultSetMetaData {
public:
	ResultSetMetaData() {}

	virtual int getColumnCount() = 0;

	/************************************************************************/
	/* @param column [0, 1, ...]
	/* @return Don't free 
	/************************************************************************/
	virtual char *getColumnLabel(int column) = 0;
	virtual char *getColumnName(int column) = 0;
	virtual SqlType getColumnType(int column) = 0;

	virtual ~ResultSetMetaData() {}
};

class ResultSet {
public:
	ResultSet() {}
	/************************************************************************/
	/* @return [0, 1, ...] or -1 for not find
	/************************************************************************/
	virtual int findColumn(const char *columnLabel) = 0;

	/************************************************************************/
	/* @param columnIndex [0, 1, ...]
	/* @param len [in , out] Given the length of ready to read, and return real read length
	/************************************************************************/
	virtual void *getBlob(int columnIndex, int *len) = 0;
	virtual double getDouble(int columnIndex) = 0;
	virtual float getFloat(int columnIndex) = 0;
	virtual int getInt(int columnIndex) = 0;
	virtual long long getInt64(int columnIndex) = 0;

	/************************************************************************/
	/* @return An string, Don't free 
	/************************************************************************/
	virtual char *getString(int columnIndex) = 0;

	virtual bool next() = 0;

	/************************************************************************/
	/* Get the current read row number
	/************************************************************************/
	virtual int getRow() = 0;
	virtual Statement *getStatement() = 0;
	virtual ResultSetMetaData *getMetaData() = 0;
	virtual void close() = 0;
	virtual bool isClosed() = 0;
	virtual ~ResultSet() {}
};

class Statement {
public:
	Statement() {}
	virtual ResultSet *executeQuery(const char *sql) = 0;
	/************************************************************************/
	/* @return The number of affected rows when INSERT, UPDATE, DELETE
	/*           or -1 when fail to execute sql
	/************************************************************************/
	virtual int executeUpdate(const char *sql) = 0;

	virtual SqlConnection *getConnection() = 0;
	// virtual ResultSet *getResultSet() = 0;
	virtual bool isClosed() = 0;
	virtual void close() = 0;
	virtual ~Statement() {}
};

class PreparedStatement : public Statement {
public:
	PreparedStatement() {}
	virtual ResultSet *executeQuery() = 0;
	virtual int executeUpdate() = 0;
	virtual ResultSetMetaData *getMetaData() = 0;
	
	virtual int getParameterCount() = 0;

	/************************************************************************/
	/* @param paramIdx [0, 1, 2, ...
	/************************************************************************/
	virtual int getParameterType(int paramIdx) = 0;
	virtual void setBlob(int parameIdx, void *blob, int len) = 0;
	virtual void setDouble(int parameIdx, double x) = 0;
	virtual void setFloat(int parameIdx, float x) = 0;
	virtual void setInt(int parameIdx, int x) = 0;
	virtual void setInt64(int parameIdx, long long x) = 0;
	virtual void setNull(int parameIdx, SqlType sqlType) = 0;
	virtual void setString(int parameIdx, const char *str) = 0;

	virtual int getInsertId() = 0;

	virtual ~PreparedStatement() {}
protected:
	virtual ResultSet *executeQuery(const char *sql);
	virtual int executeUpdate(const char *sql);
};

class SqlConnection {
public:
	/************************************************************************/
	/* @param url  mysql://host:port/my-db-name    sqlite://my-db-file-path
	/*			   Example: mysql://localhost:3306/test
	/*						sqlite://D:/abc/ddd.db
	/************************************************************************/
	static SqlConnection* open(const char *url, const char *usrName, const char *password);

	virtual Statement *createStatement() = 0;
	virtual PreparedStatement *prepareStatement(const char *sql) = 0;
	virtual bool getAutoCommit() = 0;
	virtual void setAutoCommit(bool autoCommit) = 0;
	virtual bool isClosed() = 0;
	virtual void close() = 0;
	virtual bool commit() = 0;
	virtual bool rollback() = 0;
	/************************************************************************/
	/* @return Error info, Not need free
	/************************************************************************/
	virtual char *getError() = 0;
	virtual ~SqlConnection();
protected:
	virtual void setError(const char *err) = 0;
	SqlConnection();
};
