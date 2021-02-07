#include <stdio.h>
#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <unistd.h>

void print_node(xmlNode* start, int depth)
{
	xmlNode* node = NULL;

	for (node = start ; node; node = node->next)
	{
		if (node->type == XML_ELEMENT_NODE)
		{
			int i;
			for (i = 0 ; i < depth ;i++) printf("  ");
			printf("node->name: %s\n", node->name);
			 xmlNewTextChild((xmlNode*)node, NULL, (xmlChar*)"child2", (xmlChar*)"jeewon");
		}

		if (node->type == XML_TEXT_NODE)
		{
			 printf("Content:%s \n",xmlNodeGetContent(node));
		}

		print_node(node->children, depth + 1);
	}

}

const char* xmlFile="../libxml/test.xml";
//const char* xmlFile="../dmr_description.xml";

int test_read()
{
	xmlDoc* doc = xmlParseFile(xmlFile);
	xmlNode* root = NULL;
	char buf[200];

	printf("==== TEST READ =========\n");

	if (doc == NULL)
	{
		getcwd(buf, 200);
		printf("cannot parse file(errno=%d) %s\n",errno, buf);
		return -1;
	}

	root = xmlDocGetRootElement(doc);
	if (root == NULL)
	{
		printf("cannot get root element\n");
		return -1;
	}

	print_node(root, 0);

	xmlSaveFormatFile("read.xml", doc, 1);


	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;
}

void test_xpath()
{
	//xmlChar* xpath =(xmlChar*)"/xmlns:root/xmlns:device/xmlns:friendlyName";
	xmlChar* xpath =(xmlChar*)"/root/child";
	xmlDocPtr doc = NULL;
	xmlXPathContextPtr context = NULL;
	xmlXPathObjectPtr result = NULL;
	xmlNodeSetPtr nodeset = NULL;
	xmlChar* keyword = NULL;
	int i;
	xmlNode node;

	printf("==== TEST XPath =========\n");

	doc = xmlParseFile(xmlFile);
	if (doc == NULL)
	{
		printf("[%s:%d]xmlParseFile failed\n", __FUNCTION__, __LINE__);
		goto CLEAN_UP;
	}

	context = xmlXPathNewContext(doc);
	if(context == NULL)
	{
		printf("[%s:%d]xmlXPathNewContext failed\n", __FUNCTION__, __LINE__);
		goto CLEAN_UP;
	}

//    xmlXPathRegisterNs(context, (xmlChar*)"xmlns" , (xmlChar*)"urn:schemas-upnp-org:device-1-0");
//    xmlXPathRegisterNs(context, (xmlChar*)"dlna" , (xmlChar*)"urn:schemas-dlna-org:device-1-0");


	result = xmlXPathEvalExpression(xpath, context);
	if (result == NULL)
	{
		printf("Error in xmlXPathEvalExpression\n");
		goto CLEAN_UP;
	}

	if (xmlXPathNodeSetIsEmpty(result->nodesetval))
	{
		printf("result->nodesetval is empty\n");
		goto CLEAN_UP;
	}

	nodeset = result->nodesetval;
	for (i = 0 ; i < nodeset->nodeNr ; i++)
	{
		//keyword = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
		//printf("keyword: %s\n", keyword);
		//xmlFree(keyword);
		//printf("type=%d keyword: %s\n", nodeset->nodeTab[i]->xmlChildrenNode->type, nodeset->nodeTab[i]->xmlChildrenNode->name);
		xmlNewTextChild((xmlNode*)nodeset->nodeTab[i]->xmlChildrenNode, NULL, (xmlChar*)"child2", (xmlChar*)"jeewon");
	}

	xmlSaveFormatFile("../libxml/read.xml", doc, 1);

CLEAN_UP:
	if (result != NULL)
	{
		xmlXPathFreeObject(result);
	}

	if (context != NULL)
	{
	   xmlXPathFreeContext(context);
	}

	if (doc != NULL)
	{
		xmlFreeDoc(doc);
	}

	xmlCleanupParser();
}

int main(int argc, char *argv[])
{

//test_read();

   test_xpath();


	printf("exit...\n");

	return 0;
}
