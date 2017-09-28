#include <stdio.h>
#include "XmlParser.h"
#include "UIFactory.h"

int aaaaa_main(int argc, char **argv) {
	XmlParser parser;
	parser.parseFile("D:\\CPP\\WinUI\\Debug\\aa.xml");

	XmlNode *root = parser.getRoot();
	if (root) {
		root->print(0);
	}
	printf(" hasError:%d  error : %s \n" , parser.hasError(), parser.getError());

	XmlPartLoader loader("D:\\CPP\\WinUI\\Debug\\ui-part.txt");
	
	return 0;
}