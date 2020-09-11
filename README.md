# zrfcxlsreader - Read binary Excel files from ABAP - Started RFC Server for libxls
==
This is zrfcxlsreader, Started RFC Server builded with the SAP NetWeaver RFC SDK specification with libxls for reading Excel files in the nasty old binary OLE format.  

The ABAP call is pretty simple:
'''abap
*
* Transform XLS to internal table
*

call function 'Z_RFCXLSREADER' destination 'Z_RFCXLSREADER_SERVER'
  exporting
    iv_xls_xstring        = lv_xls_xstring
  importing
    ev_resptext           = lv_text
    et_xls_tab            = lt_xls_tab
  exceptions
    invalid_input         = 1
    system_failure        = 2 message rfc_message
    communication_failure = 3 message rfc_message.
'''
