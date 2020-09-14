[English](https://github.com/NikoGal/zrfcxlsreader/blob/master/README.en.md)
# zrfcxlsreader - чтение xls файлов в ABAP с помощью libxls

zrfcxlsreader является Started RFC сервером по спецификации SAP NetWeaver RFC SDK, который обеспечивает возможность обработки бинарных xls фалов в ABAP с помощью библиотеки  libxls.

lixls - библиотека C для чтения файлов Excel в старом двоичном формате OLE.

Вызов ABAP довольно прост:
```abap
data: 
      lv_xls_xstring     type xstring,
      lt_xls_tab         type standard table of zsrfcxlsreader,
      lv_text            like sy-lisel,
      rfc_message(128).

* ...

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
```

## Установка

Скомпилированные библиотеки можно найти в разделе [Releases](https://github.com/nikogal/zrfcxlsreader/releases).

### Иерархия файлов
Пример:
```
├── etc
│   └── ld.so.conf.d
│       ├── libxlsreader.conf
│       └── nwrfcsdk.conf
├── opt
│   └── zrfcxlsreader
│       ├── sapnwrfc.ini - // hardcoded path for ini path (only for ini)
│       └── zrfcxlsreader
└── usr
    ├── local
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

Чтобы собрать из исходников:
```
./configure
make
make install
```

### Установка объектов ABAP словаря
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
ZWWW_ZRFCSLXREADER готов к загрузке с помощью ZWWW_MIGRATE.
Пакет можно скачать в разделе [Releases](https://github.com/nikogal/zrfcxlsreader/releases).

## Конфигурация
### Пути к файлам
```bash
$:/etc/ld.so.conf.d> cat libxlsreader.conf
/usr/local/lib64
$:/etc/ld.so.conf.d> cat nwrfcsdk.conf
/usr/sap/nwrfcsdk/lib

$:ldconfig

$:/opt/zrfcxlsreader> ldd zrfcxlsreader
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

### Создание технического пользователя
RFC Server получает метаданные из словаря ABAP, для этого требуется технический пользователь.

Создайте роль пользователя со следующей авторизацией (tcode: pfcg):

```
Role                 Z_RFCMETADATA
Group/Object/Authorization/Field        'From' - 'To'                  Text
---------------------------------------------------------------------------------------------------------
--   Object Class AAAB                                                 Authorization Objects
   ---   Authorization Object S_RFC                                    
	  |--      RFC_TYPE                  All values                    Type of RFC object to which access
	  |--      RFC_NAME                  DDIF_FIELDINFO_GET            Name (Whitelist) of RFC object 
	  |--      RFC_NAME                  RFC1                          Name (Whitelist) of RFC object 
	  |--      RFC_NAME                  RFCPING                       Name (Whitelist) of RFC object 
	  |--      RFC_NAME                  RFC_GET_FUNCTION_INTERFACE    Name (Whitelist) of RFC object 
	  |--      RFC_NAME                  SDIFRUNTIME                   Name (Whitelist) of RFC object 
	  |--      RFC_NAME                  SYST                          Name (Whitelist) of RFC object 
	  ---      ACTVT                     All activities                Activity
```

Создайте пользователя с ролью Z_RFCMETADATA (tcode:us01):
```
User name for example: ZRFCMETADATA 
User Type: C Communications Data
Roles: Z_RFCMETADATA
```

Обновите авторизацию пользователя в конфигурационном файле:
```
/opt/zrfcxlsreader/sapnwrfc.ini
```

### Создание RFC Destination (tcode:sm59)
```
RFC Destination: Z_RFCXLSREADER_SERVER
Connection Type: T (TCP/IP Connection)
Activation Type: Start on Application Server
Program: /opt/zrfcxlsreader/zrfcxlsreader
```

## Проверки и Тест

Можно просто запустить программу из консоли для проверки параметров sapnwrfc.ini:
```bash
$:/opt/zrfcxlsreader> ./zrfcxlsreader
Logging in... ...done
Fetching metadata... ...done
Logging out... ...done
 ...done
RfcListenAndDispatch() returned RFC_RETRY
```

Проверка RFC соединения (tcode:sm59):
```
RFC destination: Z_RFCXLSREADER_SERVER.
```
Тестовый ABAP отчет (tcode:sa38):
```
Z_RFCXLSREADER_TEST Test zrfcxlsreader server.
```

## Результат преобразования
Исходный файл

<img width="468" alt="xls_in_abap_source" src="https://github.com/NikoGal/zrfcxlsreader/blob/master/files/xls_in_abap_source.png">

Релультат преобразования

<img width="468" alt="xls_in_abap_result" src="https://github.com/NikoGal/zrfcxlsreader/blob/master/files/xls_in_abap_result.png">

## Варианты использования
Данный rfc сервер может быть полезен для реализация сценария интеграции с обработкой вложений:
```
mail server(IMAP) -> process integration (mail adapter) -> abap proxy (xls processing)
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

          " Attachment XSTRING content
          attach_xstring = attachment->get_binary_data( ).
        endloop.
* ...
```
