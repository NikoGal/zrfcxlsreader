#include <stdlib.h>

#undef __STRICT_ANSI__
#include <stdio.h>

#ifndef SAPwithUNICODE
#include "/usr/sap/nwrfcsdk/include/saptype.h"
#include "/usr/sap/nwrfcsdk/include/sapuc.h"
#endif
#include "/usr/sap/nwrfcsdk/include/sapnwrfc.h"

#undef _GLIBCXX_USE_C99_STDIO
#include <cstdint>
#include <string>


#include <sstream>

namespace patch
{
template < typename T > std::string to_string( const T& n )
{
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
}
}

using namespace std;

#include "/usr/local/include/xls.h"


#if defined(SAPonOS400) || defined(SAPwithPASE400)
/* o4fprintfU, o4fgetsU
 * calling o4xxxU instead of xxxU produces much smaller code,
 * because it directly expands to xxxU16, while xxxU expands to
 * as4_xxx  which links against the whole pipe check and handling for ILE.
 * This creates an executable containing almost the whole platform
 * specific kernel and needs the ILE O4PRTLIB in a special library.
 * Because we have no pipe usage of fxxxU here I use o4xxxU.
 * ATTENTION:
 * In any case the above mentioned functions are efectively used for
 * pipes, the redefinition below corrupts this functionality
 * because than the pipe handling is not called for our platform.
 */
#undef fprintfU
#define fprintfU o4fprintfU
#undef fgetsU
#define fgetsU o4fgetsU
#endif


//
// Helper functions
//

static inline std::u16string str2u16(const std::string &str) {
    std::setlocale(LC_ALL, "");
    std::u16string wstr = u"";
    char16_t c16str[3] = u"\0";
    mbstate_t mbs;
    for (const auto& it: str) {
        memset (&mbs, 0, sizeof (mbs));//set shift state to the initial state
        memmove(c16str, u"\0\0\0", 3);
        mbrtoc16 (c16str, &it, 3, &mbs);
        wstr.append(std::u16string(c16str));
    }//for
    return wstr;
}


RFC_RC errorHandling(RFC_RC rc, SAP_UC* description, RFC_ERROR_INFO* errorInfo, RFC_CONNECTION_HANDLE connection)
{
    printfU(cU("%s: %d\n"), description, rc);
    printfU(cU("%s: %s\n"), errorInfo->key, errorInfo->message);
    // It's better to close the TCP/IP connection cleanly, than to just let the
    // backend get a "Connection reset by peer" error...
    if (connection != NULL) RfcCloseConnection(connection, errorInfo);

    return errorInfo->code;

//    exit(1);
}


//
// Hardcoding Metadata Description
//

RFC_FUNCTION_DESC_HANDLE CreateMyFunctionDesc(RFC_ERROR_INFO* errorInfo)
{
    RFC_RC rc = RFC_OK;


    // Create the type description
    
    RFC_TYPE_DESC_HANDLE hMyTypeDesc = RfcCreateTypeDesc(cU("ZSRFCXLSREADER"), errorInfo);
    if (hMyTypeDesc == NULL) return NULL;


    // Define the field descriptions
    
    RFC_FIELD_DESC fields[] = {
        { iU("SHEET_NAME"),	RFCTYPE_STRING,	8,	0,	8,	0,  0,0,0 },
        { iU("SHEET_NUM"),	RFCTYPE_INT,	4,	8,	4,	8,  0,0,0 },
        { iU("ROW_NUM"),	RFCTYPE_INT,	4,	12,	4,	12, 0,0,0 },
        { iU("COLUMN_NUM"),	RFCTYPE_INT,	4,	16,	4,	16, 0,0,0 },
        { iU("CELL_ID"),	RFCTYPE_STRING,	8,	24,	8,	24, 0,0,0 },
        { iU("CELL_STR"),	RFCTYPE_STRING,	8,	32,	8,	32, 0,0,0 }
    };


    // Add the field descriptions to the types
    
    for(int i=0; i<6; ++i) {
        rc = RfcAddTypeField(hMyTypeDesc, &fields[i], errorInfo);
        if (rc != RFC_OK) {
            RfcDestroyTypeDesc(hMyTypeDesc, NULL);
            return NULL;
        }
    }


    // Set total length of the type. (This locks the handle, so it can
    // no longer be modified.
    
    rc = RfcSetTypeLength(hMyTypeDesc, 40, 40, errorInfo);
    if (rc != RFC_OK) {
        RfcDestroyTypeDesc(hMyTypeDesc, NULL);
        return NULL;
    }


    // Create the function description

    RFC_FUNCTION_DESC_HANDLE hMyFunctionDesc = RfcCreateFunctionDesc(cU("Z_RFCXLSREADER"), errorInfo);
    if (hMyFunctionDesc == NULL) {
        RfcDestroyTypeDesc(hMyTypeDesc, NULL);
        return NULL;
    }


    // Define the parameter descriptions
    
    RFC_PARAMETER_DESC parameters[] = {
        {iU("IV_XLS_XSTRING"), RFCTYPE_XSTRING,  RFC_IMPORT,  0,   0, 0, 0,         iU(""),iU(""), 0, 0},
        {iU("ET_XLS_TAB"),     RFCTYPE_TABLE, RFC_EXPORT,  40, 40, 0, hMyTypeDesc, iU(""),iU(""), 0, 0}
    };


    // Add the parameter descriptions to the function
   
    for (int i=0; i<2; ++i) {
        rc = RfcAddParameter(hMyFunctionDesc, &parameters[i], errorInfo);
        if (rc != RFC_OK) {
            RfcDestroyTypeDesc(hMyTypeDesc, NULL);
            RfcDestroyFunctionDesc(hMyFunctionDesc, NULL);
            return NULL;
        }

    }

    return hMyFunctionDesc;
}


RFC_RC SAP_API MyFunctionImplementation(RFC_CONNECTION_HANDLE rfcHandle, RFC_FUNCTION_HANDLE funcHandle, RFC_ERROR_INFO* errorInfoP)
{
    RFC_ATTRIBUTES attributes;
    RFC_TABLE_HANDLE importTab = 0;

    RFC_STRUCTURE_HANDLE tabLine = 0;
    RFC_TABLE_HANDLE exportTab = 0;
    RFC_TABLE_HANDLE exportXlsTab = 0;
    RFC_ERROR_INFO errorInfo ;
    RFC_CHAR buffer[257]; //One for the terminating zero
    RFC_INT intValue;
    RFC_RC rc = RFC_OK;
    unsigned tabLen = 0, strLen = 0, xstrLen = 0, xstrLenActual = 0;
    unsigned  i = 0;
    buffer[256] = 0;

    printfU(cU("\n*** Got request from the following system: ***\n"));

    RfcGetConnectionAttributes(rfcHandle, &attributes, &errorInfo);
    printfU(cU("System ID: %s\n"), attributes.sysId);
    printfU(cU("System No: %s\n"), attributes.sysNumber);
    printfU(cU("Mandant  : %s\n"), attributes.client);
    printfU(cU("Host     : %s\n"), attributes.partnerHost);
    printfU(cU("User     : %s\n"), attributes.user);


    //
    // Importing Parameter
    //

    printfU(cU("\nImporting Parameter:\n"));

    rc = RfcGetStringLength(funcHandle, cU("IV_XLS_XSTRING"), &xstrLen, &errorInfo);
    if(RFC_OK != rc) {
    	return rc;
    }

    SAP_RAW* pBufferX = (SAP_RAW*)malloc(xstrLen);
    if(!pBufferX) 
    {
        printf("Error malloc, requested xstrLen = %u\n", xstrLen);

        errorInfoP->code = RFC_ABAP_RUNTIME_FAILURE;
        errorInfoP->group = ABAP_APPLICATION_FAILURE;
        strncpyU(errorInfoP->key, cU("MALLOC_ERROR"), 12);
        strncpyU(errorInfoP->message, cU("Error memory allocation"), 23);

        return RFC_ABAP_RUNTIME_FAILURE;
    }

    rc = RfcGetXString(funcHandle, cU("IV_XLS_XSTRING"), pBufferX, xstrLen, &xstrLenActual, &errorInfo);
    if(RFC_OK != rc) 
    {
        printfU(cU("Error reading xstring; xstrLen = %u, xstrLenActual = %u\n"), xstrLen, xstrLenActual);
    
	free(pBufferX);
	return rc;
    }


    //
    // libxls works
    //
    
    printf("Opening xls file ...\n");
    
    xls::xls_error_t error = xls::LIBXLS_OK;
    xls::xlsWorkBook* wb;
    
    wb = xls::xls_open_buffer(pBufferX, xstrLen, "UTF-8", &error);

    if (NULL == wb)
    {
        errorInfoP->code = RFC_ABAP_RUNTIME_FAILURE;
        errorInfoP->group = ABAP_APPLICATION_FAILURE;
        strncpyU(errorInfoP->key, cU("LIBXLS_ERROR"), 12);
        strncpyU(errorInfoP->message, cU("Error open xls"), 14);

        printf("Error reading file: %s\n", xls::xls_getError(error));
        
	rc = RFC_ABAP_RUNTIME_FAILURE;
    }
    else
    {
        RfcGetTable(funcHandle, cU("ET_XLS_TAB"), &exportXlsTab, &errorInfo);

        for (int i=0; i<wb->sheets.count; i++)   // sheets
        {
            xls::xlsWorkSheet *work_sheet = xls::xls_getWorkSheet(wb, i);
            error = xls::xls_parseWorkSheet(work_sheet);

            printf("*** Sheet name: %s *******\n", wb->sheets.sheet[i].name);

            for (int j=0; j<=work_sheet->rows.lastrow; j++)   // rows
            {
                xls::xlsRow *row = xls::xls_row(work_sheet, j);
                for (int k=0; k <= work_sheet->rows.lastcol; k++)   // columns
                {
                    xls::xlsCell *cell = &row->cells.cell[k];

                    tabLine = RfcAppendNewRow(exportXlsTab, &errorInfo);

                    strLen = strlenU(str2u16(wb->sheets.sheet[i].name).c_str()) ;
                    RfcSetString(tabLine, cU("SHEET_NAME"), str2u16(wb->sheets.sheet[i].name).c_str(), strLen, &errorInfo);

                    rc = RfcSetInt(tabLine, cU("SHEET_NUM"),  i, &errorInfo);
                    if (rc != RFC_OK)
                    {
                        printfU(cU("Invalid value for SHEET_NUM: %u\n"), i);
                    }
                    RfcSetInt(tabLine, cU("ROW_NUM"),    j, &errorInfo);
                    RfcSetInt(tabLine, cU("COLUMN_NUM"), k, &errorInfo);


                    // do something with cell
                    if (cell->id == XLS_RECORD_BLANK)
                    {
                        // do something with a blank cell
                        strLen = strlenU(cU("XLS_RECORD_BLANK"));
                        RfcSetString(tabLine, cU("CELL_ID"), cU("XLS_RECORD_BLANK"), strLen, &errorInfo);
                    }
                    else if (cell->id == XLS_RECORD_NUMBER)
                    {
                        // use cell->d, a double-precision number
                        strLen = strlenU(cU("XLS_RECORD_NUMBER"));
                        RfcSetString(tabLine, cU("CELL_ID"), cU("XLS_RECORD_NUMBER"), strLen, &errorInfo);

                        std::string str_double = patch::to_string(cell->d);
                        strLen = strlenU(str2u16( str_double ).c_str());
                        RfcSetString(tabLine, cU("CELL_STR"), str2u16( str_double ).c_str(), strLen, &errorInfo);
                    }
                    else if (cell->id == XLS_RECORD_FORMULA)
                    {
                        strLen = strlenU(cU("XLS_RECORD_FORMULA"));
                        RfcSetString(tabLine, cU("CELL_ID"), cU("XLS_RECORD_FORMULA"), strLen, &errorInfo);

                        if (strcmp(cell->str, "bool") == 0)
                        {
                            // its boolean, and test cell->d > 0.0 for true
                            if(cell-> d > 0.0) {
                                RfcSetString(tabLine, cU("CELL_STR"), cU("true"), 4, &errorInfo);
                            } else {
                                RfcSetString(tabLine, cU("CELL_STR"), cU("false"), 5, &errorInfo);
                            }
                        }
                        else if (strcmp(cell->str, "error") == 0)
                        {
                            // formula is in error
                        }
                        else
                        {
                            // cell->str is valid as the result of a string formula.

                            strLen = strlenU(str2u16( cell->str ).c_str());
                            RfcSetString(tabLine, cU("CELL_STR"), str2u16( cell->str ).c_str(), strLen, &errorInfo);
                        }
                    }
                    else if (cell->str != NULL)
                    {
                        // cell->str contains a string value

                        strLen = strlenU(cU("XLS_RECORD_STR"));
                        RfcSetString(tabLine, cU("CELL_ID"), cU("XLS_RECORD_STR"), strLen, &errorInfo);

                        printf("Cell value: %s\t", cell->str);
                        printfU(cU("Cell valueU: %s\n"), str2u16(cell->str).c_str());

                        strLen = strlenU(str2u16( cell->str ).c_str());
                        RfcSetString(tabLine, cU("CELL_STR"), str2u16( cell->str ).c_str(), strLen, &errorInfo);

                    } else {
                        strLen = strlenU(cU("XLS_RECORD_UNKNOWN"));
                        RfcSetString(tabLine, cU("CELL_ID"), cU("XLS_RECORD_UNKNOWN"), strLen, &errorInfo);
                    }
                }
            }
            xls_close_WS(work_sheet);
        }
        xls_close_WB(wb);
    }

    free(pBufferX);

    printfU(cU("**** Processing of xls finished ***\n"));

    return rc;
}

int mainU(int argc, SAP_UC** argv)
{
    RFC_RC rc;
    RFC_FUNCTION_DESC_HANDLE hMyFunctionDesc;
    RFC_CONNECTION_PARAMETER repoCon;
    RFC_CONNECTION_HANDLE repoHandle, serverHandle;
    RFC_ERROR_INFO errorInfo;


    //
    // sapnwrfc.ini
    // Default work dir is /usr/sap/<SID>/<Instance No.>/work

    // key for link RfcOpenConnection and config section in sapnwrfc.ini
    repoCon.name = cU("dest");
    repoCon.value = cU("rfcxls");
    
    //RfcSetTraceDir((SAP_UC*)getModulePath().c_str(), &errorInfo);
    //RfcSetIniPath(getModulePaht().c_str(), &errorInfo);


    //
    // Hardcoded metadata setting Up
    //
    
    printfU(cU("Hardcoded metadata..."));
    hMyFunctionDesc = CreateMyFunctionDesc(&errorInfo);
    if (hMyFunctionDesc == NULL) {
        return errorHandling(errorInfo.code, cU("hFunctionDes is null "), &errorInfo, repoHandle);
    }

    rc = RfcInstallServerFunction(NULL, hMyFunctionDesc, MyFunctionImplementation, &errorInfo);
    if (rc != RFC_OK) {
        return errorHandling(rc, cU("Error Setting "), &errorInfo, repoHandle);
    }


    //
    // Starting Server
    //

    serverHandle = RfcStartServer(argc, argv,  &repoCon, 1, &errorInfo);
    if (serverHandle == NULL) {
        return errorHandling(errorInfo.code, cU("Error Starting RFC Server"), &errorInfo, NULL);
    }
    printfU(cU(" ...done\n"));

    rc = RfcListenAndDispatch(serverHandle, -1, &errorInfo);
    printfU(cU("RfcListenAndDispatch() returned %s\n"), RfcGetRcAsString(rc));

    if(rc != RFC_OK) {
        printfU(cU("%s -- %s\n"), errorInfo.key, errorInfo.message);
    }

    if (rc == RFC_OK || rc == RFC_ABAP_EXCEPTION) {
        RfcCloseConnection(serverHandle, NULL);
    }

    return 0;
}
