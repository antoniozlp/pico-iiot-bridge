#ifndef _HTML_NETWORK_H_
#define _HTML_NETWORK_H_

#define HTML_SECTION_NETWORK \
    "<section id=\"network\">"\
        "<h1>Network Settings</h1>"\
        "<p class=\"desc\">Configure Ethernet network parameters.</p>"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_network.cgi\">"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"mac\">MAC Address</label>"\
                    "<input id=\"mac\" name=\"mac\" type=\"text\" placeholder=\"00:08:DC:12:34:56\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"ip\">IP Address</label>"\
                    "<input id=\"ip\" name=\"ip\" type=\"text\" placeholder=\"192.168.1.100\">"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"sn\">Subnet Mask</label>"\
                    "<input id=\"sn\" name=\"sn\" type=\"text\" placeholder=\"255.255.255.0\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"gw\">Gateway</label>"\
                    "<input id=\"gw\" name=\"gw\" type=\"text\" placeholder=\"192.168.1.1\">"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"dns\">DNS Server</label>"\
                    "<input id=\"dns\" name=\"dns\" type=\"text\" placeholder=\"8.8.8.8\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"dhcp\">DHCP Mode</label>"\
                    "<select id=\"dhcp\" name=\"dhcp\">"\
                        "<option value=\"0\">Static</option>"\
                        "<option value=\"1\">DHCP</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Network</button>"\
        "</form>"\
    "</section>"

#endif // _HTML_NETWORK_H_
