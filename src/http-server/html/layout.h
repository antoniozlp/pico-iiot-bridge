#ifndef _HTML_LAYOUT_H_
#define _HTML_LAYOUT_H_

#define HTML_HEAD_START \
    "<!DOCTYPE html>"\
    "<html lang=\"en\">"\
    "<head>"\
        "<meta charset=\"UTF-8\">"\
        "<title>Pico IIOT Bridge</title>"

#define HTML_HEAD_END \
    "</head>"\
    "<body>"

#define HTML_SIDEBAR \
    "<div class=\"sidebar\">"\
        "<h2>Menu</h2>"\
        "<div class=\"menu-item active\" onclick=\"showPage('home')\">Home</div>"\
        "<div class=\"menu-item\" onclick=\"showPage('network')\">Network</div>"\
        "<div class=\"menu-item\" onclick=\"showPage('serial')\">Serial</div>"\
        "<div class=\"menu-item\" onclick=\"showPage('s2tcp')\">Serial to TCP</div>"\
    "</div>"

#define HTML_CONTENT_START \
    "<div class=\"content\">"

#define HTML_CONTENT_END \
    "</div>"

#define HTML_BODY_END \
    "</body>"\
    "</html>"

#endif // _HTML_LAYOUT_H_
