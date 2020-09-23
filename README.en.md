[Русский](https://github.com/NikoGal/zrfcxlsreader/blob/master/README.md)
# zrfcxlsreader - read binary Excel files from ABAP with the libxls

This is zrfcxlsreader, Started RFC Server builded with the SAP NetWeaver RFC SDK specification and with the libxls.

lixls is a nice C library for reading Excel files in the nasty old binary OLE format.

The ABAP call is pretty simple:
```abap
data: 
      lv_xls_xstring     type xstring,
      lt_xls_tab         type standard table of zsrfcxlsreader,
      rfc_message(128).

* ...

*
* Transform XLS to internal table
*

call function 'Z_RFCXLSREADER' destination 'Z_RFCXLSREADER_SERVER'
  exporting
    iv_xls_xstring        = lv_xls_xstring
  importing
    et_xls_tab            = lt_xls_tab
  exceptions
    invalid_input         = 1
    system_failure        = 2 message rfc_message
    communication_failure = 3 message rfc_message.
```
## Transformation result
Source file

<img width="468" alt="xls_in_abap_source" src="https://github.com/NikoGal/zrfcxlsreader/blob/master/files/xls_in_abap_source.png">

Result

<img width="468" alt="xls_in_abap_result" src="https://github.com/NikoGal/zrfcxlsreader/blob/master/files/xls_in_abap_result.png">

## Installation

Check out the [Releases](https://github.com/nikogal/zrfcxlsreader/releases) section.

### Deployment tree
Example:
```
├── etc
│   └── ld.so.conf.d
│       ├── libxlsreader.conf
│       └── nwrfcsdk.conf
└── usr
    ├── local
    |   └── bin
    │   │   └── zrfcxlsreader
    |   │
    │   └── lib64
    │       ├── libxlsreader.la
    │       ├── libxlsreader.so -> libxlsreader.so.1.5.3
    │       ├── libxlsreader.so.1 -> libxlsreader.so.1.5.3
    │       └── libxlsreader.so.1.5.3
    └── sap
        └── nwrfcsdk
            └── lib
                ├── libicudata.so.50
                ├── libicudecnumber.so
                ├── libicui18n.so.50
                ├── libicuuc.so.50
                ├── libsapnwrfc.so
                └── libsapucum.so
```

To build from source:
```
./configure
make
make install
```

### ABAP part
```
-------------------------------------------------------
|Package      |Obj.|Short text    |Object Name        |
-------------------------------------------------------
|ZRFCXLSREADER|DEVC|Package       |ZRFCXLSREADER      |
|ZRFCXLSREADER|FUGR|Function Group|ZRFCXLSREADER      |
|ZRFCXLSREADER|PROG|Program       |Z_RFCXLSREADER_TEST|
|ZRFCXLSREADER|TABL|Table         |ZSRFCXLSREADER     |
|ZRFCXLSREADER|TTYP|Table Type    |ZTTRFCXLSREADER    |
-------------------------------------------------------
```
ZWWW_ZRFCSLXREADER prepared for upload with ZWWW_MIGRATE tool.
Check out the [Releases](https://github.com/nikogal/zrfcxlsreader/releases) section.

## Configuration
### Libraries path
```bash
$:/etc/ld.so.conf.d> cat libxlsreader.conf
/usr/local/lib64
$:/etc/ld.so.conf.d> cat nwrfcsdk.conf
/usr/sap/nwrfcsdk/lib

$:ldconfig

$:/usr/local/bin> ldd zrfcxlsreader
        linux-vdso.so.1                                          
        libsapucum.so     => /usr/sap/nwrfcsdk/lib/libsapucum.so 
        libsapnwrfc.so    => /usr/sap/nwrfcsdk/lib/libsapnwrfc.so
        libxlsreader.so.1 => /usr/local/lib64/libxlsreader.so.1  
        libstdc++.so.6    => /usr/lib64/libstdc++.so.6           
        libm.so.6         => /lib64/libm.so.6                    
        libgcc_s.so.1     => /lib64/libgcc_s.so.1                
        libpthread.so.0   => /lib64/libpthread.so.0              
        libc.so.6         => /lib64/libc.so.6                    
        libdl.so.2        => /lib64/libdl.so.2                   
        librt.so.1        => /lib64/librt.so.1                   
        libuuid.so.1      => /usr/lib64/libuuid.so.1             
        /lib64/ld-linux-x86-64.so.2     
```

### Create RFC Destination (tcode:sm59)
```
RFC Destination: Z_RFCXLSREADER_SERVER
Connection Type: T (TCP/IP Connection)
Activation Type: Start on Application Server
Program: /usr/local/bin/zrfcxlsreader
```

## Test

Check RFC connection via tcode:sm59 for:
```
RFC destination: Z_RFCXLSREADER_SERVER.
```
Run test report in ABAP (tcode:sa38):
```
Z_RFCXLSREADER_TEST Test zrfcxlsreader server.
```

## Use Case
It can be usefull for integration scenario with attachment processing:
```
mail server(IMAP)->process integration (mail adapter) -> abap proxy (xls processing)
```

```abap
    try.
        gr_server_context = cl_proxy_access=>get_server_context( ).
        gr_proto_msg_id ?= gr_server_context->get_protocol( if_wsprotocol=>message_id ).
        gs_msg_id = gr_proto_msg_id->get_message_id( ).


        " get attachment protocol
        attachment_protocol ?= gr_server_context->get_protocol( if_wsprotocol=>attachments ).

        attachments = attachment_protocol->get_attachments( ).
        loop at attachments into attachment.
          content_type = attachment->get_content_type( ).
          document_name = attachment->get_document_name( ).

          " Attachment is XSTRING content
          attach_xstring = attachment->get_binary_data( ).
          
          call function 'Z_RFCXLSREADER' destination 'Z_RFCXLSREADER_SERVER'
            exporting
              iv_xls_xstring        = attach_xstring
            importing
              et_xls_tab            = lt_xls_tab
            exceptions
              invalid_input         = 1
              system_failure        = 2 message rfc_message
              communication_failure = 3 message rfc_message.
              
            " ...
              
        endloop.
* ...
```
