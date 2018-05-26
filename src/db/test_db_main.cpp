#include <stdio.h>
#include <string.h>
#include "SqlDriver.h"

int db_main(int argc, char **argv) {
	const char *mysqlDriver = "mysql://localhost:3306/test";
	const char *sqliteDriver = "sqlite://test.db";

	SqlConnection *db = SqlConnection::open(sqliteDriver, "root", "root");

#if 0
	Statement *stmta = db->createStatement();
	stmta->executeUpdate("create table _big_data(_name varchar(24), _blog blob)");
	int ssx = stmta->executeUpdate("insert into _big_data values ('a', 'Here it is a big data A')");

	PreparedStatement *s = db->prepareStatement("insert into _big_data values (?, ?)");
	s->setString(0, "D");
	const char *xxx = "This is a string D";
	s->setBlob(1, (void *)xxx, strlen(xxx));
	s->executeUpdate();
	delete s;
#endif

	PreparedStatement *stmt = db->prepareStatement("select * from _big_data");
	ResultSetMetaData *meta = stmt->getMetaData();
	for (int i = 0; i < meta->getColumnCount(); ++i) {
		printf(" %10s : %3d \n", meta->getColumnName(i), meta->getColumnType(i));
	}
	delete meta;
	ResultSet *rs = stmt->executeQuery();
	while (rs->next()) {
		int len = 0;
		char *blobData = (char *)(rs->getBlob(1, &len));
		if (blobData) blobData[len] = 0;
		printf(" [%s]  [%s]  \n", rs->getString(0), blobData);
	}
	delete rs;
	delete stmt;

	return 0;
}