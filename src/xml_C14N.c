/*
 *  $Id: xml_C14N.c,v 1.12 2004/11/24 03:00:10 mgrouch Exp $
 *
 *  Canonical XML implementation test program
 *  (http://www.w3.org/TR/2001/REC-xml-c14n-20010315)
 *
 *  See Copyright for the status of this software.
 * 
 *  Author: Aleksey Sanin <aleksey@aleksey.com>
 */

#include <libxml/xmlversion.h>
#include <config.h>

#if defined(LIBXML_C14N_ENABLED)

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxml/c14n.h>

#include "xmlstar.h"

static void c14nUsage(exit_status status)
{
    extern void fprint_c14n_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_c14n_usage(o, get_arg(ARG0));
    fprintf(o, "%s", more_info);
    exit(status);
}

static xmlXPathObjectPtr
load_xpath_expr (xmlDocPtr parent_doc, const char* filename);

static xmlChar **parse_list(char *str);

#if 0
static void print_xpath_nodes(xmlNodeSetPtr nodes);
#endif

static int 
run_c14n(const char* xml_filename, int with_comments, int exclusive,
         const char* xpath_filename, xmlChar **inclusive_namespaces,
         int nonet) {
    xmlDocPtr doc;
    xmlXPathObjectPtr xpath = NULL; 
    int ret;

    /*
     * build an XML tree from a the file; we need to add default
     * attributes and resolve all character and entities references
     */

    doc = xmlReadFile(xml_filename, NULL,
        XML_PARSE_NOENT | XML_PARSE_DTDLOAD |
        XML_PARSE_DTDATTR | (nonet? XML_PARSE_NONET:0));
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n", xml_filename);
        return(EXIT_BAD_FILE);
    }
    
    /*
     * Check the document is of the right kind
     */    
    if(xmlDocGetRootElement(doc) == NULL) {
        fprintf(stderr,"Error: empty document for file \"%s\"\n", xml_filename);
        xmlFreeDoc(doc);
        return(EXIT_BAD_FILE);
    }

    /* 
     * load xpath file if specified 
     */
    if(xpath_filename) {
        xpath = load_xpath_expr(doc, xpath_filename);
        if(xpath == NULL) {
            fprintf(stderr,"Error: unable to evaluate xpath expression\n");
            xmlFreeDoc(doc); 
            return(EXIT_BAD_FILE);
        }
    }

    /*
     * Canonical form
     */
    set_stdout_binary();       /* avoid line ending conversion */
    ret = xmlC14NDocSave(doc,
        (xpath) ? xpath->nodesetval : NULL,
        exclusive, inclusive_namespaces,
        with_comments, "-", 0);
    if(ret < 0) {
        fprintf(stderr,"Error: failed to canonicalize XML file \"%s\" (ret=%d)\n",
            xml_filename, ret);
        xmlFreeDoc(doc);
        return(EXIT_FAILURE);
    }
 
    /*
     * Cleanup
     */ 
    if(xpath != NULL) xmlXPathFreeObject(xpath);
    xmlFreeDoc(doc);    

    return(ret >= 0? EXIT_SUCCESS : EXIT_FAILURE);
}

int c14nMain(void) {
    int ret = -1, nonet = 1;
    int with_comments = 0, exclusive = 0;
    const char* option;
    const char* xml_filename;
    const char* xpath_filename;
    xmlChar** incl_ns_list = NULL;
    char* incl_ns_arg;
    
    /*
     * Init libxml
     */     
    xmlInitParser();
    {LIBXML_TEST_VERSION}
    
    /*
     * Parse command line and process file
     */

    option = get_arg(OPTION_NEXT);
    if (option && strcmp(option, "--net") == 0) {
        nonet = 0;
        option = get_arg(OPTION_NEXT);
    }
    xml_filename = get_arg(ARG_NEXT);
    if (!xml_filename) xml_filename = "-";
    xpath_filename = get_arg(ARG_NEXT);


     if (!option || strcmp(option, "--with-comments") == 0) {
         with_comments = 1;
     } else if (strcmp(option, "--without-comments") == 0) {
         ;
     } else if(strcmp(option, "--exc-with-comments") == 0) {
         exclusive = 1;
         with_comments = 1;
     } else if(strcmp(option, "--exc-without-comments") == 0) {
         exclusive = 1;
     } else if (strcmp(option, "--help") == 0 || strcmp(option, "-h") == 0) {
        c14nUsage(EXIT_SUCCESS);
     } else {
         fprintf(stderr, "error: unknown mode: %s.\n", option);
         exit(EXIT_BAD_ARGS);
     }

     if (exclusive) {
         incl_ns_arg = get_arg(ARG_NEXT);
         if (incl_ns_arg) incl_ns_list = parse_list(incl_ns_arg);
     }
     ret = run_c14n(xml_filename, with_comments, exclusive,
         xpath_filename, incl_ns_list, nonet);
     free(incl_ns_list);

    /* 
     * Shutdown libxml
     */
    xmlCleanupParser();
    xmlMemoryDump();
    
    return ret;
}

/*
 * Macro used to grow the current buffer.
 */
#define growBufferReentrant() {                                         \
    buffer_size *= 2;                                                   \
    buffer = (xmlChar **)                                               \
                    xmlRealloc(buffer, buffer_size * sizeof(xmlChar*)); \
    if (buffer == NULL) {                                               \
        perror("realloc failed");                                       \
        return(NULL);                                                   \
    }                                                                   \
}

static xmlChar **
parse_list(char *str) {
    xmlChar **buffer;
    xmlChar **out = NULL;
    int buffer_size = 0;
    int len;

    if(str == NULL) {
        return(NULL);
    }

    len = xmlStrlen(BAD_CAST str);
    if((str[0] == '\'') && (str[len - 1] == '\'')) {
        str[len - 1] = '\0';
        str++;
        len -= 2;
    }
    /*
     * allocate an translation buffer.
     */
    buffer_size = 1000;
    buffer = xmlMalloc(buffer_size * sizeof(xmlChar*));
    out = buffer;
    
    while(*str != '\0') {
        if (out - buffer > buffer_size - 10) {
            int indx = out - buffer;

            growBufferReentrant();
            out = &buffer[indx];
        }
        (*out++) = BAD_CAST str;
        while(*str != ',' && *str != '\0') ++str;
        if(*str == ',') *(str++) = '\0';
    }
    (*out) = NULL;
    return buffer;
}

static xmlXPathObjectPtr
load_xpath_expr (xmlDocPtr parent_doc, const char* filename) {
    xmlXPathObjectPtr xpath; 
    xmlDocPtr doc;
    xmlChar *expr;
    xmlXPathContextPtr ctx; 
    xmlNodePtr node;
    xmlNsPtr ns;
    
    /*
     * load XPath expr as a file
     */
    xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault(1);

    doc = xmlReadFile(filename, NULL, XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
        return(NULL);
    }
    
    /*
     * Check the document is of the right kind
     */    
    if(xmlDocGetRootElement(doc) == NULL) {
        fprintf(stderr,"Error: empty document for file \"%s\"\n", filename);
        xmlFreeDoc(doc);
        return(NULL);
    }

    node = doc->children;
    while(node != NULL && !xmlStrEqual(node->name, (const xmlChar *)"XPath")) {
        node = node->next;
    }
    
    if(node == NULL) {   
        fprintf(stderr,"Error: XPath element expected in the file  \"%s\"\n", filename);
        xmlFreeDoc(doc);
        return(NULL);
    }

    expr = xmlNodeGetContent(node);
    if(expr == NULL) {
        fprintf(stderr,"Error: XPath content element is NULL \"%s\"\n", filename);
        xmlFreeDoc(doc);
        return(NULL);
    }

    ctx = xmlXPathNewContext(parent_doc);
    if(ctx == NULL) {
        fprintf(stderr,"Error: unable to create new context\n");
        xmlFree(expr); 
        xmlFreeDoc(doc); 
        return(NULL);
    }

    /*
     * Register namespaces
     */
    ns = node->nsDef;
    while(ns != NULL) {
        if(xmlXPathRegisterNs(ctx, ns->prefix, ns->href) != 0) {
            fprintf(stderr,"Error: unable to register NS with prefix=\"%s\" and href=\"%s\"\n", ns->prefix, ns->href);
                xmlFree(expr); 
            xmlXPathFreeContext(ctx); 
            xmlFreeDoc(doc); 
            return(NULL);
        }
        ns = ns->next;
    }

    /*  
     * Evaluate xpath
     */
    xpath = xmlXPathEvalExpression(expr, ctx);
    if(xpath == NULL) {
        fprintf(stderr,"Error: unable to evaluate xpath expression\n");
            xmlFree(expr); 
        xmlXPathFreeContext(ctx); 
        xmlFreeDoc(doc); 
        return(NULL);
    }

    /* print_xpath_nodes(xpath->nodesetval); */

    xmlFree(expr); 
    xmlXPathFreeContext(ctx); 
    xmlFreeDoc(doc); 
    return(xpath);
}

#if 0
static void
print_xpath_nodes(xmlNodeSetPtr nodes) {
    xmlNodePtr cur;
    int i;
    
    if(nodes == NULL ){ 
        fprintf(stderr, "Error: no nodes set defined\n");
        return;
    }
    
    fprintf(stderr, "Nodes Set:\n-----\n");
    for(i = 0; i < nodes->nodeNr; ++i) {
        if(nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            xmlNsPtr ns;
            
            ns = (xmlNsPtr)nodes->nodeTab[i];
            cur = (xmlNodePtr)ns->next;
            fprintf(stderr, "namespace \"%s\"=\"%s\" for node %s:%s\n", 
                    ns->prefix, ns->href,
                    (cur->ns) ? cur->ns->prefix : BAD_CAST "", cur->name);
        } else if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
            cur = nodes->nodeTab[i];    
            fprintf(stderr, "element node \"%s:%s\"\n", 
                    (cur->ns) ? cur->ns->prefix : BAD_CAST "", cur->name);
        } else {
            cur = nodes->nodeTab[i];    
            fprintf(stderr, "node \"%s\": type %d\n", cur->name, cur->type);
        }
    }
}
#endif

#else
#include <stdio.h>
int c14nMain(int argc, char **argv) {
    printf("%s : XPath/Canonicalization support not compiled in\n", argv[0]);
    return 2;
}
#endif /* LIBXML_C14N_ENABLED */

