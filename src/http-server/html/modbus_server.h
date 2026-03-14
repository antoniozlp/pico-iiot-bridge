#ifndef _HTML_MODBUS_SERVER_H_
#define _HTML_MODBUS_SERVER_H_

#define HTML_SECTION_MODBUS_SERVER \
    "<section id=\"modbus_server\">"\
        "<h1>Modbus RTU Server Settings</h1>"\
        "<p class=\"desc\">Configure the Modbus RTU slave (server) and its memory map.</p>"\
        \
        "<!-- Server Global Configuration -->"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_modbus_server.cgi\">"\
            "<h2>Modbus RTU Server</h2>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"mbs_enable\">Enable Modbus RTU Server</label>"\
                    "<select id=\"mbs_enable\" name=\"enable\">"\
                        "<option value=\"0\">Disabled</option>"\
                        "<option value=\"1\">Enabled</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"mbs_serial\">Serial Port</label>"\
                    "<select id=\"mbs_serial\" name=\"serial_id\">"\
                        "<option value=\"0\">UART0 (Console)</option>"\
                        "<option value=\"1\">UART1 (Bridge)</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"mbs_address\">Server RTU Address (1-247)</label>"\
                    "<input id=\"mbs_address\" name=\"server_address\" type=\"number\" min=\"1\" max=\"247\" value=\"1\">"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Server Settings</button>"\
        "</form>"\
        \
        "<!-- Memory Blocks Configuration -->"\
        "<h2 style=\"margin-top: 30px;\">Server Memory Map (Memory Blocks)</h2>"\
        "<p class=\"desc\" style=\"margin-bottom: 15px;\">Map Modbus address ranges to Tag Database tags. Requests from a master must fall within one memory block.</p>"\
        \
        "<!-- Memory Block Tabs -->"\
        "<div style=\"margin-bottom: 20px; display: flex; flex-wrap: wrap; gap: 5px;\">"\
            "<button onclick=\"showServerDp(0)\" id=\"sdp0-btn\" class=\"tab-btn active\" style=\"padding: 8px 15px; cursor: pointer;\">Block 0</button>"\
            "<button onclick=\"showServerDp(1)\" id=\"sdp1-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 1</button>"\
            "<button onclick=\"showServerDp(2)\" id=\"sdp2-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 2</button>"\
            "<button onclick=\"showServerDp(3)\" id=\"sdp3-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 3</button>"\
            "<button onclick=\"showServerDp(4)\" id=\"sdp4-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 4</button>"\
            "<button onclick=\"showServerDp(5)\" id=\"sdp5-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 5</button>"\
            "<button onclick=\"showServerDp(6)\" id=\"sdp6-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 6</button>"\
            "<button onclick=\"showServerDp(7)\" id=\"sdp7-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 7</button>"\
            "<button onclick=\"showServerDp(8)\" id=\"sdp8-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 8</button>"\
            "<button onclick=\"showServerDp(9)\" id=\"sdp9-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 9</button>"\
        "</div>"\
        \
        "<form id=\"sdp0-form\" onsubmit=\"submitServerBlockForm(event, this, 0)\" action=\"set_modbus_server_memory_block.cgi\" style=\"display:block;\">"\
            "<input type=\"hidden\" name=\"block_idx\" value=\"0\">"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp0_enable\">Enable</label><select id=\"sdp0_enable\" name=\"enabled\"><option value=\"0\">Disabled</option><option value=\"1\">Enabled</option></select></div>"\
                "<div><label for=\"sdp0_type\">Data Type</label><select id=\"sdp0_type\" name=\"data_type\"><option value=\"0\">Coil (R/W, 1-bit)</option><option value=\"1\">Discrete Input (RO, 1-bit)</option><option value=\"2\">Input Register (RO, 16-bit)</option><option value=\"3\">Holding Register (R/W, 16-bit)</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp0_writable\">Allow Master Writes</label><select id=\"sdp0_writable\" name=\"writable\"><option value=\"0\">No (read-only)</option><option value=\"1\">Yes (read/write)</option></select></div>"\
                "<div><label for=\"sdp0_encoding\">32-bit Encoding</label><select id=\"sdp0_encoding\" name=\"encoding\"><option value=\"0\">ABCD (big-endian)</option><option value=\"1\">BADC (word swap)</option><option value=\"2\">CDAB (byte swap)</option><option value=\"3\">DCBA (word+byte swap)</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp0_addr\">Start Address</label><input id=\"sdp0_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\"></div>"\
                "<div><label for=\"sdp0_count\">Count (1-10)</label><input id=\"sdp0_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\"></div>"\
            "</div>"\
            "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
                "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers/coils to tags):</label>"\
                "<div class=\"row\">"\
                    "<div><label for=\"sdp0_tag0\">Reg/Coil 0:</label><select id=\"sdp0_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"sdp0_tag1\">Reg/Coil 1:</label><select id=\"sdp0_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"sdp0_tag2\">Reg/Coil 2:</label><select id=\"sdp0_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"sdp0_tag3\">Reg/Coil 3:</label><select id=\"sdp0_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"sdp0_tag4\">Reg/Coil 4:</label><select id=\"sdp0_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"sdp0_tag5\">Reg/Coil 5:</label><select id=\"sdp0_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"sdp0_tag6\">Reg/Coil 6:</label><select id=\"sdp0_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"sdp0_tag7\">Reg/Coil 7:</label><select id=\"sdp0_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"sdp0_tag8\">Reg/Coil 8:</label><select id=\"sdp0_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"sdp0_tag9\">Reg/Coil 9:</label><select id=\"sdp0_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Memory Block 0</button>"\
        "</form>"\
        HTML_SERVER_BLOCK_FORM(1) \
        HTML_SERVER_BLOCK_FORM(2) \
        HTML_SERVER_BLOCK_FORM(3) \
        HTML_SERVER_BLOCK_FORM(4) \
        HTML_SERVER_BLOCK_FORM(5) \
        HTML_SERVER_BLOCK_FORM(6) \
        HTML_SERVER_BLOCK_FORM(7) \
        HTML_SERVER_BLOCK_FORM(8) \
        HTML_SERVER_BLOCK_FORM(9) \
    "</section>"

#define HTML_SERVER_BLOCK_FORM(N) \
    "<form id=\"sdp" #N "-form\" onsubmit=\"submitServerBlockForm(event, this, " #N ")\" action=\"set_modbus_server_memory_block.cgi\" style=\"display:none;\">"\
        "<input type=\"hidden\" name=\"block_idx\" value=\"" #N "\">"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"sdp" #N "_enable\">Enable</label>"\
                "<select id=\"sdp" #N "_enable\" name=\"enabled\">"\
                    "<option value=\"0\">Disabled</option>"\
                    "<option value=\"1\">Enabled</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"sdp" #N "_type\">Data Type</label>"\
                "<select id=\"sdp" #N "_type\" name=\"data_type\">"\
                    "<option value=\"0\">Coil (R/W, 1-bit)</option>"\
                    "<option value=\"1\">Discrete Input (RO, 1-bit)</option>"\
                    "<option value=\"2\">Input Register (RO, 16-bit)</option>"\
                    "<option value=\"3\">Holding Register (R/W, 16-bit)</option>"\
                "</select>"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"sdp" #N "_writable\">Allow Master Writes</label>"\
                "<select id=\"sdp" #N "_writable\" name=\"writable\">"\
                    "<option value=\"0\">No (read-only)</option>"\
                    "<option value=\"1\">Yes (read/write)</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"sdp" #N "_encoding\">32-bit Encoding</label>"\
                "<select id=\"sdp" #N "_encoding\" name=\"encoding\">"\
                    "<option value=\"0\">ABCD (big-endian)</option>"\
                    "<option value=\"1\">BADC (word swap)</option>"\
                    "<option value=\"2\">CDAB (byte swap)</option>"\
                    "<option value=\"3\">DCBA (word+byte swap)</option>"\
                "</select>"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"sdp" #N "_addr\">Start Address</label>"\
                "<input id=\"sdp" #N "_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\">"\
            "</div>"\
            "<div>"\
                "<label for=\"sdp" #N "_count\">Count (1-10)</label>"\
                "<input id=\"sdp" #N "_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\">"\
            "</div>"\
        "</div>"\
        "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
            "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers/coils to tags):</label>"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp" #N "_tag0\">Reg/Coil 0:</label><select id=\"sdp" #N "_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"sdp" #N "_tag1\">Reg/Coil 1:</label><select id=\"sdp" #N "_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp" #N "_tag2\">Reg/Coil 2:</label><select id=\"sdp" #N "_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"sdp" #N "_tag3\">Reg/Coil 3:</label><select id=\"sdp" #N "_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp" #N "_tag4\">Reg/Coil 4:</label><select id=\"sdp" #N "_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"sdp" #N "_tag5\">Reg/Coil 5:</label><select id=\"sdp" #N "_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp" #N "_tag6\">Reg/Coil 6:</label><select id=\"sdp" #N "_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"sdp" #N "_tag7\">Reg/Coil 7:</label><select id=\"sdp" #N "_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"sdp" #N "_tag8\">Reg/Coil 8:</label><select id=\"sdp" #N "_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"sdp" #N "_tag9\">Reg/Coil 9:</label><select id=\"sdp" #N "_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
        "</div>"\
        "<button type=\"submit\">Save Memory Block " #N "</button>"\
    "</form>"

#endif /* _HTML_MODBUS_SERVER_H_ */
