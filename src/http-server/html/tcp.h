#ifndef _HTML_TCP_H_
#define _HTML_TCP_H_

#define HTML_SECTION_TCP \
    "<section id=\"tcp\">"\
        "<h1>Operation Mode</h1>"\
        "<p class=\"desc\">Configure TCP server settings.</p>"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_tcp.cgi\">"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"lport\">Local Port</label>"\
                    "<input id=\"lport\" name=\"lport\" type=\"number\" min=\"1\" max=\"65535\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"maxconn\">Max Connections (1-4)</label>"\
                    "<input id=\"maxconn\" name=\"maxconn\" type=\"number\" min=\"1\" max=\"4\">"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"keepalive\">TCP Alive Check Time (s)</label>"\
                    "<input id=\"keepalive\" name=\"keepalive\" type=\"number\" min=\"0\" max=\"3600\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"timeout\">Idle Timeout (s)</label>"\
                    "<input id=\"timeout\" name=\"timeout\" type=\"number\" min=\"0\" max=\"3600\">"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save TCP</button>"\
        "</form>"\
    "</section>"

#endif // _HTML_TCP_H_
