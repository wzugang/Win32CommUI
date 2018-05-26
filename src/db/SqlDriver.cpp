#include "SqlDriver.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mysql/mysql_driver.h"
#include "sqlite3/sqlite_driver.h"

SqlConnection::SqlConnection() {
}

SqlConnection::~SqlConnection() {
}

SqlConnection* SqlConnection::open(const char *url, const char *usrName, const char *password) {
	char tmp[128];
	if (url == NULL) {
		return NULL;
	}
	strcpy(tmp, url);
	if (memcmp(url, "mysql", 5) == 0) {
		char *host = &tmp[8];
		char *p = strchr(host, ':');
		if (p == NULL) {
			return NULL;
		}
		*p = 0;
		char *port = p + 1;
		p = strchr(port, '/');
		if (p == NULL) {
			return NULL;
		}
		*p = 0;
		char *db = p + 1;
		MysqlConnection *con = new MysqlConnection();
		bool ok = con->connect(host, atoi(port), db, usrName, password);
		if (! ok) {
			delete con;
			return NULL;
		}
		return con;
	}
	if (memcmp(url, "sqlite", 6) == 0) {
		const char *file = url + 9;
		SqliteConnection *con = new SqliteConnection();
		if (! con->connect(file)) {
			delete con;
			return NULL;
		}
		return con;
	}
	return NULL;
}

ResultSet * PreparedStatement::executeQuery(const char *sql) {
	return NULL;
}

int PreparedStatement::executeUpdate(const char *sql) {
	return -1;
}
