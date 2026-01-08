#ifndef _HTML_SERIAL_H_
#define _HTML_SERIAL_H_

#define HTML_SECTION_SERIAL \
    "<section id=\"serial\">"\
        "<h1>Serial Settings</h1>"\
        "<p class=\"desc\">Configure UART interface parameters.</p>"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_serial.cgi\">"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"baud\">Baud Rate</label>"\
                    "<select id=\"baud\" name=\"baud\">"\
                        "<option value=\"9600\">9600</option>"\
                        "<option value=\"19200\">19200</option>"\
                        "<option value=\"38400\">38400</option>"\
                        "<option value=\"57600\">57600</option>"\
                        "<option value=\"115200\">115200</option>"\
                        "<option value=\"230400\">230400</option>"\
                        "<option value=\"460800\">460800</option>"\
                        "<option value=\"921600\">921600</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"databits\">Data Bits</label>"\
                    "<select id=\"databits\" name=\"databits\">"\
                        "<option value=\"5\">5</option>"\
                        "<option value=\"6\">6</option>"\
                        "<option value=\"7\">7</option>"\
                        "<option value=\"8\">8</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"parity\">Parity</label>"\
                    "<select id=\"parity\" name=\"parity\">"\
                        "<option value=\"none\">None</option>"\
                        "<option value=\"even\">Even</option>"\
                        "<option value=\"odd\">Odd</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"stopbits\">Stop Bits</label>"\
                    "<select id=\"stopbits\" name=\"stopbits\">"\
                        "<option value=\"1\">1</option>"\
                        "<option value=\"2\">2</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<label>Flow Control</label>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<input type=\"checkbox\" id=\"flowcts\" name=\"flowcts\" value=\"1\">"\
                    "<label for=\"flowcts\">CTS Flow Control</label>"\
                "</div>"\
                "<div>"\
                    "<input type=\"checkbox\" id=\"flowrts\" name=\"flowrts\" value=\"1\">"\
                    "<label for=\"flowrts\">RTS Flow Control</label>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Serial</button>"\
        "</form>"\
    "</section>"

#endif // _HTML_SERIAL_H_
