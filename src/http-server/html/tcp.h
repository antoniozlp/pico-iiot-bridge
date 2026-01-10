#ifndef _HTML_TCP_H_
#define _HTML_TCP_H_

#define HTML_SECTION_S2TCP \
    "<section id=\"s2tcp\">"\
        "<h1>Serial to TCP Converter</h1>"\
        "<p class=\"desc\">Configure transparent serial-to-TCP bridge mode.</p>"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_s2tcp.cgi\">"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"s2tcp_enable\">Enable Serial to TCP</label>"\
                    "<select id=\"s2tcp_enable\" name=\"enable\">"\
                        "<option value=\"0\">Disabled</option>"\
                        "<option value=\"1\">Enabled</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"s2tcp_serial\">Serial Port</label>"\
                    "<select id=\"s2tcp_serial\" name=\"serial\">"\
                        "<option value=\"0\">Serial 0</option>"\
                        "<option value=\"1\">Serial 1</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"s2tcp_mode\">Operation Mode</label>"\
                    "<select id=\"s2tcp_mode\" name=\"mode\" onchange=\"toggleClientFields()\">"\
                        "<option value=\"0\">Server (Listen)</option>"\
                        "<option value=\"1\">Client (Connect)</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"s2tcp_lport\">Local Port</label>"\
                    "<input id=\"s2tcp_lport\" name=\"lport\" type=\"number\" min=\"1024\" max=\"65535\">"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"s2tcp_timeout\">Idle Timeout (s)</label>"\
                    "<input id=\"s2tcp_timeout\" name=\"timeout\" type=\"number\" min=\"1\" max=\"3600\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"s2tcp_keepalive\">TCP Alive Check (s)</label>"\
                    "<input id=\"s2tcp_keepalive\" name=\"keepalive\" type=\"number\" min=\"1\" max=\"600\">"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"s2tcp_maxconn\">Max Connections (1-4)</label>"\
                    "<input id=\"s2tcp_maxconn\" name=\"maxconn\" type=\"number\" min=\"1\" max=\"4\">"\
                "</div>"\
            "</div>"\
            "<div id=\"client-fields\" style=\"display: none;\">"\
                "<h3>Client Mode Settings</h3>"\
                "<div class=\"row\">"\
                    "<div>"\
                        "<label for=\"s2tcp_remoteip\">Remote IP Address</label>"\
                        "<input id=\"s2tcp_remoteip\" name=\"remoteip\" type=\"text\" placeholder=\"192.168.1.100\">"\
                    "</div>"\
                    "<div>"\
                        "<label for=\"s2tcp_remoteport\">Remote Port</label>"\
                        "<input id=\"s2tcp_remoteport\" name=\"remoteport\" type=\"number\" min=\"1024\" max=\"65535\">"\
                    "</div>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Serial to TCP</button>"\
        "</form>"\
    "</section>"

#endif // _HTML_TCP_H_
