/*  $Id: xml_validate.c,v 1.8 2002/12/08 00:15:08 mgrouch Exp $  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trans.h"

/*
 *   TODO: Use cases
 *   1. find malfomed XML documents in a given set of XML files 
 *   2. find XML documents which do not match DTD/XSD in a given set of XML files
 *   3. precompile DTD once
 */

typedef struct _valOptions {
    char *dtd;                /* External DTD URL or file name */
    int   wellFormed;         /* Check if well formed only */
    int   listGood;           /* >0 list good, <0 list bad */
} valOptions;

typedef valOptions *valOptionsPtr;

static const char validate_usage_str[] =
"XMLStarlet Toolkit: Validate XML document(s)\n"
"Usage: xml val <options> [ <xml-file> ... ]\n"
"where <options>\n"
"   -d or --dtd <dtd-file>  - validate against DTD\n"
"   -s or --xsd <xsd-file>  - validate against schema\n"
"   -n or --line-num        - print line numbers for validation errors\n"
"   -x or --xml-out         - print result as xml\n"
"   -b or --list-bad        - list only files which do not validate\n"
"   -g or --list-good       - list only files which validate\n"
"   -w or --well-formed     - check only if XML is well-formed\n\n";

/**
 *  display short help message
 */
void
valUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, validate_usage_str);
    fprintf(o, more_info);
    exit(1);
}

/**
 *  Initialize global command line options
 */
void
valInitOptions(valOptionsPtr ops)
{
    ops->wellFormed = 0;
    ops->listGood = 0;
    ops->dtd = 0;
}

/**
 *  Parse global command line options
 */
int
valParseOptions(valOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while(i < argc)
    {
        if (!strcmp(argv[i], "--well-formed") || !strcmp(argv[i], "-w"))
        {
            ops->wellFormed = 1;
            i++;
        }
        if (!strcmp(argv[i], "--list-good") || !strcmp(argv[i], "-g"))
        {
            ops->listGood = 1;
            i++;
        }
        if (!strcmp(argv[i], "--list-bad") || !strcmp(argv[i], "-b"))
        {
            ops->listGood = -1;
            i++;
        }
        if (!strcmp(argv[i], "--dtd") || !strcmp(argv[i], "-d"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv);
            ops->dtd = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
        {
            valUsage(argc, argv);
        }
        else if (!strcmp(argv[i], "-"))
        {
            i++;
            break;
        }
        else if (argv[i][0] == '-')
        {
            valUsage(argc, argv);
        }
        else
        {
            i++;
            break;
        }
    }

    return i-1;
}

int
valPrintErr(FILE *stream, const char *format, ...)
{
}

/**
 *  Validate XML document against DTD
 */
int
valAgainstDtd(valOptionsPtr ops, char* dtdvalid, xmlDocPtr doc, char* filename)
{
    int result = 0;

    if (dtdvalid != NULL)
    {
        xmlDtdPtr dtd;

        dtd = xmlParseDTD(NULL, (const xmlChar *)dtdvalid);
        if (dtd == NULL)
        {
            xmlGenericError(xmlGenericErrorContext,
            "Could not parse DTD %s\n", dtdvalid);
            result = 2;
        }
        else
        {
            xmlValidCtxt cvp;
            cvp.userData = (void *) stderr;
            if (ops->listGood == 0)
            {
                cvp.error    = (xmlValidityErrorFunc) fprintf;
                cvp.warning  = (xmlValidityWarningFunc) fprintf;
            }
            else
            {
                cvp.error    = (xmlValidityErrorFunc) NULL;
                cvp.warning  = (xmlValidityWarningFunc) NULL;
            }
            
            
            if (!xmlValidateDtd(&cvp, doc, dtd))
            {
                if (ops->listGood < 0)
                {
                    fprintf(stdout, "%s\n", filename);
                }
                else if (ops->listGood == 0)
                    xmlGenericError(xmlGenericErrorContext,
                                    "%s: does not match %s\n",
                                    filename, dtdvalid);
                result = 3;
            }
            else
            {
                if (ops->listGood > 0)
                {
                    fprintf(stdout, "%s\n", filename);
                }
            }
            xmlFreeDtd(dtd);
        }
    }

    return result;
}

/**
 *  This is the main function for 'validate' option
 */
int
valMain(int argc, char **argv)
{
    int start;
    static valOptions ops;
    int invalidFound = 0;
    
    if (argc <=2) valUsage(argc, argv);
    valInitOptions(&ops);
    start = valParseOptions(&ops, argc, argv);

    if (ops.dtd)
    {
        int i;
/*
        printf("validate against %s", ops.dtd);
*/
        for (i=start; i<argc; i++)
        {
            xmlDocPtr doc;
            int ret;

            doc = xmlParseFile(argv[i]);
            if (doc)
            {
                /* TODO: precompile DTD once */
                ret = valAgainstDtd(&ops, ops.dtd, doc, argv[i]);
            }
            else
            {
                ret = 1; /* Malformed XML or could not open file */
            }
            if (ret) invalidFound = 1;     
        }
    }

    return invalidFound;
}  
