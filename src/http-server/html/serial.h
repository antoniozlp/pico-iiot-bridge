#ifndef _HTML_SERIAL_H_
#define _HTML_SERIAL_H_

#define HTML_SECTION_SERIAL \
    "<section id=\"serial\">"\
        "<h1>Serial Settings</h1>"\
        "<p class=\"desc\">Configure UART interface parameters.</p>"\
        "<div style=\"margin-bottom: 20px;\">"\
            "<button onclick=\"showSerial(0)\" id=\"serial0-btn\" class=\"tab-btn active\" style=\"margin-right: 10px; padding: 10px 20px; cursor: pointer;\">Serial 0 (Console)</button>"\
            "<button onclick=\"showSerial(1)\" id=\"serial1-btn\" class=\"tab-btn\" style=\"padding: 10px 20px; cursor: pointer;\">Serial 1 (Bridge)</button>"\
        "</div>"\
        "<form id=\"serial0-form\" onsubmit=\"submitSerialForm(event, this, 0)\" action=\"set_serial.cgi\" style=\"display: block;\">"\
            "<input type=\"hidden\" name=\"uart\" value=\"0\">"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"baud0\">Baud Rate</label>"\
                    "<select id=\"baud0\" name=\"baud\">"\
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
                    "<label for=\"databits0\">Data Bits</label>"\
                    "<select id=\"databits0\" name=\"databits\">"\
                        "<option value=\"5\">5</option>"\
                        "<option value=\"6\">6</option>"\
                        "<option value=\"7\">7</option>"\
                        "<option value=\"8\">8</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"parity0\">Parity</label>"\
                    "<select id=\"parity0\" name=\"parity\">"\
                        "<option value=\"none\">None</option>"\
                        "<option value=\"even\">Even</option>"\
                        "<option value=\"odd\">Odd</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"stopbits0\">Stop Bits</label>"\
                    "<select id=\"stopbits0\" name=\"stopbits\">"\
                        "<option value=\"1\">1</option>"\
                        "<option value=\"2\">2</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<label>Flow Control</label>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<input type=\"checkbox\" id=\"flowcts0\" name=\"flowcts\" value=\"1\">"\
                    "<label for=\"flowcts0\">CTS Flow Control</label>"\
                "</div>"\
                "<div>"\
                    "<input type=\"checkbox\" id=\"flowrts0\" name=\"flowrts\" value=\"1\">"\
                    "<label for=\"flowrts0\">RTS Flow Control</label>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Serial 0</button>"\
        "</form>"\
        "<form id=\"serial1-form\" onsubmit=\"submitSerialForm(event, this, 1)\" action=\"set_serial.cgi\" style=\"display: none;\">"\
            "<input type=\"hidden\" name=\"uart\" value=\"1\">"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"baud1\">Baud Rate</label>"\
                    "<select id=\"baud1\" name=\"baud\">"\
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
                    "<label for=\"databits1\">Data Bits</label>"\
                    "<select id=\"databits1\" name=\"databits\">"\
                        "<option value=\"5\">5</option>"\
                        "<option value=\"6\">6</option>"\
                        "<option value=\"7\">7</option>"\
                        "<option value=\"8\">8</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"parity1\">Parity</label>"\
                    "<select id=\"parity1\" name=\"parity\">"\
                        "<option value=\"none\">None</option>"\
                        "<option value=\"even\">Even</option>"\
                        "<option value=\"odd\">Odd</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"stopbits1\">Stop Bits</label>"\
                    "<select id=\"stopbits1\" name=\"stopbits\">"\
                        "<option value=\"1\">1</option>"\
                        "<option value=\"2\">2</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<label>Flow Control</label>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<input type=\"checkbox\" id=\"flowcts1\" name=\"flowcts\" value=\"1\">"\
                    "<label for=\"flowcts1\">CTS Flow Control</label>"\
                "</div>"\
                "<div>"\
                    "<input type=\"checkbox\" id=\"flowrts1\" name=\"flowrts\" value=\"1\">"\
                    "<label for=\"flowrts1\">RTS Flow Control</label>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Serial 1</button>"\
        "</form>"\
    "</section>"

#endif // _HTML_SERIAL_H_
