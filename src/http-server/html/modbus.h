#ifndef _HTML_MODBUS_H_
#define _HTML_MODBUS_H_

#define HTML_SECTION_MODBUS \
    "<section id=\"modbus\">"\
        "<h1>Modbus RTU Settings</h1>"\
        "<p class=\"desc\">Configure Modbus RTU client and requests.</p>"\
        \
        "<!-- Modbus Client Configuration -->"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_modbus_client.cgi\">"\
            "<h2>Modbus RTU Client</h2>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"mb_enable\">Enable Modbus RTU</label>"\
                    "<select id=\"mb_enable\" name=\"enable\">"\
                        "<option value=\"0\">Disabled</option>"\
                        "<option value=\"1\">Enabled</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"mb_serial\">Serial Port</label>"\
                    "<select id=\"mb_serial\" name=\"serial_id\">"\
                        "<option value=\"0\">UART0 (Console)</option>"\
                        "<option value=\"1\">UART1 (Bridge)</option>"\
                    "</select>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Client Settings</button>"\
        "</form>"\
        \
        "<!-- Requests Configuration -->"\
        "<h2 style=\"margin-top: 30px;\">Modbus Requests</h2>"\
        "<p class=\"desc\" style=\"margin-bottom: 15px;\">Configure up to 10 Modbus requests for monitoring.</p>"\
        \
        "<!-- Request Tabs -->"\
        "<div style=\"margin-bottom: 20px; display: flex; flex-wrap: wrap; gap: 5px;\">"\
            "<button onclick=\"showRequest(0)\" id=\"dp0-btn\" class=\"tab-btn active\" style=\"padding: 8px 15px; cursor: pointer;\">Request 0</button>"\
            "<button onclick=\"showRequest(1)\" id=\"dp1-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 1</button>"\
            "<button onclick=\"showRequest(2)\" id=\"dp2-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 2</button>"\
            "<button onclick=\"showRequest(3)\" id=\"dp3-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 3</button>"\
            "<button onclick=\"showRequest(4)\" id=\"dp4-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 4</button>"\
            "<button onclick=\"showRequest(5)\" id=\"dp5-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 5</button>"\
            "<button onclick=\"showRequest(6)\" id=\"dp6-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 6</button>"\
            "<button onclick=\"showRequest(7)\" id=\"dp7-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 7</button>"\
            "<button onclick=\"showRequest(8)\" id=\"dp8-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 8</button>"\
            "<button onclick=\"showRequest(9)\" id=\"dp9-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 9</button>"\
        "</div>"\
        \
        "<!-- Forms for each request (0-9) -->"\
        "<form id=\"dp0-form\" onsubmit=\"submitRequestForm(event, this, 0)\" action=\"set_modbus_datapoint.cgi\" style=\"display:block;\">"\
            "<input type=\"hidden\" name=\"dp_idx\" value=\"0\">"\
            "<div class=\"row\"><div><label for=\"dp0_enable\">Enable</label><select id=\"dp0_enable\" name=\"enabled\"><option value=\"0\">Disabled</option><option value=\"1\">Enabled</option></select></div>"\
            "<div><label for=\"dp0_slave\">Slave Address (1-247)</label><input id=\"dp0_slave\" name=\"slave_address\" type=\"number\" min=\"1\" max=\"247\" value=\"1\"></div></div>"\
            "<div class=\"row\"><div><label for=\"dp0_type\">Data Type</label><select id=\"dp0_type\" name=\"data_type\"><option value=\"0\">Coil (R/W, 1-bit)</option><option value=\"1\">Discrete Input (RO, 1-bit)</option><option value=\"2\">Input Register (RO, 16-bit)</option><option value=\"3\">Holding Register (R/W, 16-bit)</option></select></div>"\
            "<div><label for=\"dp0_op\">Operation</label><select id=\"dp0_op\" name=\"operation\"><option value=\"0\">Read</option><option value=\"1\">Write</option></select></div></div>"\
            "<div class=\"row\"><div><label for=\"dp0_addr\">Start Address</label><input id=\"dp0_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\"></div>"\
            "<div><label for=\"dp0_count\">Count (1-10)</label><input id=\"dp0_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\"></div></div>"\
            "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
                "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers to tags):</label>"\
                "<div class=\"row\">"\
                    "<div><label for=\"dp0_tag0\">Reg/Coil 0:</label><select id=\"dp0_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"dp0_tag1\">Reg/Coil 1:</label><select id=\"dp0_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"dp0_tag2\">Reg/Coil 2:</label><select id=\"dp0_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"dp0_tag3\">Reg/Coil 3:</label><select id=\"dp0_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"dp0_tag4\">Reg/Coil 4:</label><select id=\"dp0_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"dp0_tag5\">Reg/Coil 5:</label><select id=\"dp0_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"dp0_tag6\">Reg/Coil 6:</label><select id=\"dp0_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"dp0_tag7\">Reg/Coil 7:</label><select id=\"dp0_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"dp0_tag8\">Reg/Coil 8:</label><select id=\"dp0_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"dp0_tag9\">Reg/Coil 9:</label><select id=\"dp0_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Request 0</button>"\
        "</form>"\
        HTML_REQUEST_FORM(1) \
        HTML_REQUEST_FORM(2) \
        HTML_REQUEST_FORM(3) \
        HTML_REQUEST_FORM(4) \
        HTML_REQUEST_FORM(5) \
        HTML_REQUEST_FORM(6) \
        HTML_REQUEST_FORM(7) \
        HTML_REQUEST_FORM(8) \
        HTML_REQUEST_FORM(9) \
    "</section>"

// Macro to generate request form HTML (used 10 times above)
#define HTML_REQUEST_FORM(N) \
    "<form id=\"dp" #N "-form\" onsubmit=\"submitRequestForm(event, this, " #N ")\" action=\"set_modbus_datapoint.cgi\" style=\"display:none;\">"\
        "<input type=\"hidden\" name=\"dp_idx\" value=\"" #N "\">"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"dp" #N "_enable\">Enable</label>"\
                "<select id=\"dp" #N "_enable\" name=\"enabled\">"\
                    "<option value=\"0\">Disabled</option>"\
                    "<option value=\"1\">Enabled</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"dp" #N "_slave\">Slave Address (1-247)</label>"\
                "<input id=\"dp" #N "_slave\" name=\"slave_address\" type=\"number\" min=\"1\" max=\"247\" value=\"1\">"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"dp" #N "_type\">Data Type</label>"\
                "<select id=\"dp" #N "_type\" name=\"data_type\">"\
                    "<option value=\"0\">Coil (R/W, 1-bit)</option>"\
                    "<option value=\"1\">Discrete Input (RO, 1-bit)</option>"\
                    "<option value=\"2\">Input Register (RO, 16-bit)</option>"\
                    "<option value=\"3\">Holding Register (R/W, 16-bit)</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"dp" #N "_op\">Operation</label>"\
                "<select id=\"dp" #N "_op\" name=\"operation\">"\
                    "<option value=\"0\">Read</option>"\
                    "<option value=\"1\">Write</option>"\
                "</select>"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"dp" #N "_addr\">Start Address</label>"\
                "<input id=\"dp" #N "_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\">"\
            "</div>"\
            "<div>"\
                "<label for=\"dp" #N "_count\">Count (1-10)</label>"\
                "<input id=\"dp" #N "_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\">"\
            "</div>"\
        "</div>"\
        "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
            "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers to tags):</label>"\
            "<div class=\"row\">"\
                "<div><label for=\"dp" #N "_tag0\">Reg/Coil 0:</label><select id=\"dp" #N "_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"dp" #N "_tag1\">Reg/Coil 1:</label><select id=\"dp" #N "_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"dp" #N "_tag2\">Reg/Coil 2:</label><select id=\"dp" #N "_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"dp" #N "_tag3\">Reg/Coil 3:</label><select id=\"dp" #N "_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"dp" #N "_tag4\">Reg/Coil 4:</label><select id=\"dp" #N "_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"dp" #N "_tag5\">Reg/Coil 5:</label><select id=\"dp" #N "_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"dp" #N "_tag6\">Reg/Coil 6:</label><select id=\"dp" #N "_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"dp" #N "_tag7\">Reg/Coil 7:</label><select id=\"dp" #N "_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"dp" #N "_tag8\">Reg/Coil 8:</label><select id=\"dp" #N "_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"dp" #N "_tag9\">Reg/Coil 9:</label><select id=\"dp" #N "_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
        "</div>"\
        "<button type=\"submit\">Save Request " #N "</button>"\
    "</form>"

#endif // _HTML_MODBUS_H_
