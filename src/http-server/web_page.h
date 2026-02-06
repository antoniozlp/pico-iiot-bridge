/**
    Copyright (c) 2021 WIZnet Co.,Ltd

    SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _WEB_PAGE_H_
#define _WEB_PAGE_H_

#include "html/css.h"
#include "html/js.h"
#include "html/layout.h"
#include "html/home.h"
#include "html/network.h"
#include "html/serial.h"
#include "html/tcp.h"
#include "html/modbus.h"

#define index_page \
    HTML_HEAD_START \
    HTML_CSS \
    HTML_HEAD_END \
    HTML_SIDEBAR \
    HTML_CONTENT_START \
    HTML_SECTION_HOME \
    HTML_SECTION_NETWORK \
    HTML_SECTION_SERIAL \
    HTML_SECTION_S2TCP \
    HTML_SECTION_MODBUS \
    HTML_CONTENT_END \
    HTML_JS \
    HTML_BODY_END

#endif /* _WEB_PAGE_H_ */
