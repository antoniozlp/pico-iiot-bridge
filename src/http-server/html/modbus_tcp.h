#ifndef _HTML_MODBUS_TCP_H_
#define _HTML_MODBUS_TCP_H_

// ============================================================================
// Modbus TCP Client Section
// ============================================================================

#define HTML_SECTION_MODBUS_TCP_CLIENT \
    "<section id=\"modbus_tcp_client\">"\
        "<h1>Modbus TCP Client Settings</h1>"\
        "<p class=\"desc\">Configure Modbus TCP client to poll a remote Modbus TCP server.</p>"\
        \
        "<!-- TCP Client Global Configuration -->"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_modbus_tcp_client.cgi\">"\
            "<h2>Modbus TCP Client</h2>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"tc_enable\">Enable Modbus TCP Client</label>"\
                    "<select id=\"tc_enable\" name=\"enable\">"\
                        "<option value=\"0\">Disabled</option>"\
                        "<option value=\"1\">Enabled</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"tc_remote_ip\">Remote Server IP</label>"\
                    "<input id=\"tc_remote_ip\" name=\"remote_ip\" type=\"text\" maxlength=\"15\" value=\"192.168.11.100\" placeholder=\"192.168.11.100\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"tc_remote_port\">Remote Port</label>"\
                    "<input id=\"tc_remote_port\" name=\"remote_port\" type=\"number\" min=\"1\" max=\"65535\" value=\"502\">"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Client Settings</button>"\
        "</form>"\
        \
        "<!-- TCP Requests Configuration -->"\
        "<h2 style=\"margin-top: 30px;\">Modbus TCP Requests</h2>"\
        "<p class=\"desc\" style=\"margin-bottom: 15px;\">Configure up to 10 Modbus requests for polling the remote server.</p>"\
        \
        "<!-- TCP Request Tabs -->"\
        "<div style=\"margin-bottom: 20px; display: flex; flex-wrap: wrap; gap: 5px;\">"\
            "<button onclick=\"showTcpRequest(0)\" id=\"tc_dp0-btn\" class=\"tab-btn active\" style=\"padding: 8px 15px; cursor: pointer;\">Request 0</button>"\
            "<button onclick=\"showTcpRequest(1)\" id=\"tc_dp1-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 1</button>"\
            "<button onclick=\"showTcpRequest(2)\" id=\"tc_dp2-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 2</button>"\
            "<button onclick=\"showTcpRequest(3)\" id=\"tc_dp3-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 3</button>"\
            "<button onclick=\"showTcpRequest(4)\" id=\"tc_dp4-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 4</button>"\
            "<button onclick=\"showTcpRequest(5)\" id=\"tc_dp5-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 5</button>"\
            "<button onclick=\"showTcpRequest(6)\" id=\"tc_dp6-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 6</button>"\
            "<button onclick=\"showTcpRequest(7)\" id=\"tc_dp7-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 7</button>"\
            "<button onclick=\"showTcpRequest(8)\" id=\"tc_dp8-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 8</button>"\
            "<button onclick=\"showTcpRequest(9)\" id=\"tc_dp9-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Request 9</button>"\
        "</div>"\
        \
        "<!-- Forms for each TCP request (0-9) -->"\
        "<form id=\"tc_dp0-form\" onsubmit=\"submitTcpRequestForm(event, this, 0)\" action=\"set_modbus_tcp_datapoint.cgi\" style=\"display:block;\">"\
            "<input type=\"hidden\" name=\"dp_idx\" value=\"0\">"\
            "<div class=\"row\"><div><label for=\"tc_dp0_enable\">Enable</label><select id=\"tc_dp0_enable\" name=\"enabled\"><option value=\"0\">Disabled</option><option value=\"1\">Enabled</option></select></div>"\
            "<div><label for=\"tc_dp0_slave\">Unit ID (1-247)</label><input id=\"tc_dp0_slave\" name=\"slave_address\" type=\"number\" min=\"1\" max=\"247\" value=\"1\"></div></div>"\
            "<div class=\"row\"><div><label for=\"tc_dp0_type\">Data Type</label><select id=\"tc_dp0_type\" name=\"data_type\"><option value=\"0\">Coil (R/W, 1-bit)</option><option value=\"1\">Discrete Input (RO, 1-bit)</option><option value=\"2\">Input Register (RO, 16-bit)</option><option value=\"3\">Holding Register (R/W, 16-bit)</option></select></div>"\
            "<div><label for=\"tc_dp0_op\">Operation</label><select id=\"tc_dp0_op\" name=\"operation\"><option value=\"0\">Read</option><option value=\"1\">Write</option></select></div></div>"\
            "<div class=\"row\"><div><label for=\"tc_dp0_addr\">Start Address</label><input id=\"tc_dp0_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\"></div>"\
            "<div><label for=\"tc_dp0_count\">Count (1-10)</label><input id=\"tc_dp0_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\"></div></div>"\
            "<div class=\"row\"><div><label for=\"tc_dp0_encoding\">32-bit Encoding</label><select id=\"tc_dp0_encoding\" name=\"encoding\"><option value=\"0\">ABCD (big-endian)</option><option value=\"1\">BADC (word swap)</option><option value=\"2\">CDAB (byte swap)</option><option value=\"3\">DCBA (word+byte swap)</option></select></div></div>"\
            "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
                "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers to tags):</label>"\
                "<div class=\"row\">"\
                    "<div><label for=\"tc_dp0_tag0\">Reg/Coil 0:</label><select id=\"tc_dp0_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"tc_dp0_tag1\">Reg/Coil 1:</label><select id=\"tc_dp0_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"tc_dp0_tag2\">Reg/Coil 2:</label><select id=\"tc_dp0_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"tc_dp0_tag3\">Reg/Coil 3:</label><select id=\"tc_dp0_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"tc_dp0_tag4\">Reg/Coil 4:</label><select id=\"tc_dp0_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"tc_dp0_tag5\">Reg/Coil 5:</label><select id=\"tc_dp0_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"tc_dp0_tag6\">Reg/Coil 6:</label><select id=\"tc_dp0_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"tc_dp0_tag7\">Reg/Coil 7:</label><select id=\"tc_dp0_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"tc_dp0_tag8\">Reg/Coil 8:</label><select id=\"tc_dp0_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"tc_dp0_tag9\">Reg/Coil 9:</label><select id=\"tc_dp0_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Request 0</button>"\
        "</form>"\
        HTML_TCP_REQUEST_FORM(1) \
        HTML_TCP_REQUEST_FORM(2) \
        HTML_TCP_REQUEST_FORM(3) \
        HTML_TCP_REQUEST_FORM(4) \
        HTML_TCP_REQUEST_FORM(5) \
        HTML_TCP_REQUEST_FORM(6) \
        HTML_TCP_REQUEST_FORM(7) \
        HTML_TCP_REQUEST_FORM(8) \
        HTML_TCP_REQUEST_FORM(9) \
    "</section>"

// Macro to generate TCP request form HTML for requests 1-9
#define HTML_TCP_REQUEST_FORM(N) \
    "<form id=\"tc_dp" #N "-form\" onsubmit=\"submitTcpRequestForm(event, this, " #N ")\" action=\"set_modbus_tcp_datapoint.cgi\" style=\"display:none;\">"\
        "<input type=\"hidden\" name=\"dp_idx\" value=\"" #N "\">"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"tc_dp" #N "_enable\">Enable</label>"\
                "<select id=\"tc_dp" #N "_enable\" name=\"enabled\">"\
                    "<option value=\"0\">Disabled</option>"\
                    "<option value=\"1\">Enabled</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"tc_dp" #N "_slave\">Unit ID (1-247)</label>"\
                "<input id=\"tc_dp" #N "_slave\" name=\"slave_address\" type=\"number\" min=\"1\" max=\"247\" value=\"1\">"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"tc_dp" #N "_type\">Data Type</label>"\
                "<select id=\"tc_dp" #N "_type\" name=\"data_type\">"\
                    "<option value=\"0\">Coil (R/W, 1-bit)</option>"\
                    "<option value=\"1\">Discrete Input (RO, 1-bit)</option>"\
                    "<option value=\"2\">Input Register (RO, 16-bit)</option>"\
                    "<option value=\"3\">Holding Register (R/W, 16-bit)</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"tc_dp" #N "_op\">Operation</label>"\
                "<select id=\"tc_dp" #N "_op\" name=\"operation\">"\
                    "<option value=\"0\">Read</option>"\
                    "<option value=\"1\">Write</option>"\
                "</select>"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"tc_dp" #N "_addr\">Start Address</label>"\
                "<input id=\"tc_dp" #N "_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\">"\
            "</div>"\
            "<div>"\
                "<label for=\"tc_dp" #N "_count\">Count (1-10)</label>"\
                "<input id=\"tc_dp" #N "_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\">"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"tc_dp" #N "_encoding\">32-bit Encoding</label>"\
                "<select id=\"tc_dp" #N "_encoding\" name=\"encoding\">"\
                    "<option value=\"0\">ABCD (big-endian)</option>"\
                    "<option value=\"1\">BADC (word swap)</option>"\
                    "<option value=\"2\">CDAB (byte swap)</option>"\
                    "<option value=\"3\">DCBA (word+byte swap)</option>"\
                "</select>"\
            "</div>"\
        "</div>"\
        "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
            "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers to tags):</label>"\
            "<div class=\"row\">"\
                "<div><label for=\"tc_dp" #N "_tag0\">Reg/Coil 0:</label><select id=\"tc_dp" #N "_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"tc_dp" #N "_tag1\">Reg/Coil 1:</label><select id=\"tc_dp" #N "_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"tc_dp" #N "_tag2\">Reg/Coil 2:</label><select id=\"tc_dp" #N "_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"tc_dp" #N "_tag3\">Reg/Coil 3:</label><select id=\"tc_dp" #N "_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"tc_dp" #N "_tag4\">Reg/Coil 4:</label><select id=\"tc_dp" #N "_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"tc_dp" #N "_tag5\">Reg/Coil 5:</label><select id=\"tc_dp" #N "_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"tc_dp" #N "_tag6\">Reg/Coil 6:</label><select id=\"tc_dp" #N "_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"tc_dp" #N "_tag7\">Reg/Coil 7:</label><select id=\"tc_dp" #N "_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"tc_dp" #N "_tag8\">Reg/Coil 8:</label><select id=\"tc_dp" #N "_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"tc_dp" #N "_tag9\">Reg/Coil 9:</label><select id=\"tc_dp" #N "_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
        "</div>"\
        "<button type=\"submit\">Save Request " #N "</button>"\
    "</form>"

// ============================================================================
// Modbus TCP Server Section
// ============================================================================

#define HTML_SECTION_MODBUS_TCP_SERVER \
    "<section id=\"modbus_tcp_server\">"\
        "<h1>Modbus TCP Server Settings</h1>"\
        "<p class=\"desc\">Configure the Modbus TCP server and its memory map.</p>"\
        \
        "<!-- TCP Server Global Configuration -->"\
        "<form onsubmit=\"submitForm(event, this)\" action=\"set_modbus_tcp_server.cgi\">"\
            "<h2>Modbus TCP Server</h2>"\
            "<div class=\"row\">"\
                "<div>"\
                    "<label for=\"ts_enable\">Enable Modbus TCP Server</label>"\
                    "<select id=\"ts_enable\" name=\"enable\">"\
                        "<option value=\"0\">Disabled</option>"\
                        "<option value=\"1\">Enabled</option>"\
                    "</select>"\
                "</div>"\
                "<div>"\
                    "<label for=\"ts_port\">Listen Port</label>"\
                    "<input id=\"ts_port\" name=\"port\" type=\"number\" min=\"1\" max=\"65535\" value=\"502\">"\
                "</div>"\
                "<div>"\
                    "<label for=\"ts_address\">Server Unit ID (1-247)</label>"\
                    "<input id=\"ts_address\" name=\"server_address\" type=\"number\" min=\"1\" max=\"247\" value=\"1\">"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Server Settings</button>"\
        "</form>"\
        \
        "<!-- TCP Server Memory Blocks Configuration -->"\
        "<h2 style=\"margin-top: 30px;\">Server Memory Map (Memory Blocks)</h2>"\
        "<p class=\"desc\" style=\"margin-bottom: 15px;\">Map Modbus address ranges to Tag Database tags. Requests from a client must fall within one memory block.</p>"\
        \
        "<!-- TCP Server Memory Block Tabs -->"\
        "<div style=\"margin-bottom: 20px; display: flex; flex-wrap: wrap; gap: 5px;\">"\
            "<button onclick=\"showTcpServerBlock(0)\" id=\"ts_dp0-btn\" class=\"tab-btn active\" style=\"padding: 8px 15px; cursor: pointer;\">Block 0</button>"\
            "<button onclick=\"showTcpServerBlock(1)\" id=\"ts_dp1-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 1</button>"\
            "<button onclick=\"showTcpServerBlock(2)\" id=\"ts_dp2-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 2</button>"\
            "<button onclick=\"showTcpServerBlock(3)\" id=\"ts_dp3-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 3</button>"\
            "<button onclick=\"showTcpServerBlock(4)\" id=\"ts_dp4-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 4</button>"\
            "<button onclick=\"showTcpServerBlock(5)\" id=\"ts_dp5-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 5</button>"\
            "<button onclick=\"showTcpServerBlock(6)\" id=\"ts_dp6-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 6</button>"\
            "<button onclick=\"showTcpServerBlock(7)\" id=\"ts_dp7-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 7</button>"\
            "<button onclick=\"showTcpServerBlock(8)\" id=\"ts_dp8-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 8</button>"\
            "<button onclick=\"showTcpServerBlock(9)\" id=\"ts_dp9-btn\" class=\"tab-btn\" style=\"padding: 8px 15px; cursor: pointer;\">Block 9</button>"\
        "</div>"\
        \
        "<form id=\"ts_dp0-form\" onsubmit=\"submitTcpServerBlockForm(event, this, 0)\" action=\"set_modbus_tcp_server_memory_block.cgi\" style=\"display:block;\">"\
            "<input type=\"hidden\" name=\"block_idx\" value=\"0\">"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp0_enable\">Enable</label><select id=\"ts_dp0_enable\" name=\"enabled\"><option value=\"0\">Disabled</option><option value=\"1\">Enabled</option></select></div>"\
                "<div><label for=\"ts_dp0_type\">Data Type</label><select id=\"ts_dp0_type\" name=\"data_type\"><option value=\"0\">Coil (R/W, 1-bit)</option><option value=\"1\">Discrete Input (RO, 1-bit)</option><option value=\"2\">Input Register (RO, 16-bit)</option><option value=\"3\">Holding Register (R/W, 16-bit)</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp0_writable\">Allow Client Writes</label><select id=\"ts_dp0_writable\" name=\"writable\"><option value=\"0\">No (read-only)</option><option value=\"1\">Yes (read/write)</option></select></div>"\
                "<div><label for=\"ts_dp0_encoding\">32-bit Encoding</label><select id=\"ts_dp0_encoding\" name=\"encoding\"><option value=\"0\">ABCD (big-endian)</option><option value=\"1\">BADC (word swap)</option><option value=\"2\">CDAB (byte swap)</option><option value=\"3\">DCBA (word+byte swap)</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp0_addr\">Start Address</label><input id=\"ts_dp0_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\"></div>"\
                "<div><label for=\"ts_dp0_count\">Count (1-10)</label><input id=\"ts_dp0_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\"></div>"\
            "</div>"\
            "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
                "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers/coils to tags):</label>"\
                "<div class=\"row\">"\
                    "<div><label for=\"ts_dp0_tag0\">Reg/Coil 0:</label><select id=\"ts_dp0_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"ts_dp0_tag1\">Reg/Coil 1:</label><select id=\"ts_dp0_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"ts_dp0_tag2\">Reg/Coil 2:</label><select id=\"ts_dp0_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"ts_dp0_tag3\">Reg/Coil 3:</label><select id=\"ts_dp0_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"ts_dp0_tag4\">Reg/Coil 4:</label><select id=\"ts_dp0_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"ts_dp0_tag5\">Reg/Coil 5:</label><select id=\"ts_dp0_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"ts_dp0_tag6\">Reg/Coil 6:</label><select id=\"ts_dp0_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"ts_dp0_tag7\">Reg/Coil 7:</label><select id=\"ts_dp0_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
                "<div class=\"row\">"\
                    "<div><label for=\"ts_dp0_tag8\">Reg/Coil 8:</label><select id=\"ts_dp0_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                    "<div><label for=\"ts_dp0_tag9\">Reg/Coil 9:</label><select id=\"ts_dp0_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
                "</div>"\
            "</div>"\
            "<button type=\"submit\">Save Memory Block 0</button>"\
        "</form>"\
        HTML_TCP_SERVER_BLOCK_FORM(1) \
        HTML_TCP_SERVER_BLOCK_FORM(2) \
        HTML_TCP_SERVER_BLOCK_FORM(3) \
        HTML_TCP_SERVER_BLOCK_FORM(4) \
        HTML_TCP_SERVER_BLOCK_FORM(5) \
        HTML_TCP_SERVER_BLOCK_FORM(6) \
        HTML_TCP_SERVER_BLOCK_FORM(7) \
        HTML_TCP_SERVER_BLOCK_FORM(8) \
        HTML_TCP_SERVER_BLOCK_FORM(9) \
    "</section>"

// Macro to generate TCP server memory block form HTML for blocks 1-9
#define HTML_TCP_SERVER_BLOCK_FORM(N) \
    "<form id=\"ts_dp" #N "-form\" onsubmit=\"submitTcpServerBlockForm(event, this, " #N ")\" action=\"set_modbus_tcp_server_memory_block.cgi\" style=\"display:none;\">"\
        "<input type=\"hidden\" name=\"block_idx\" value=\"" #N "\">"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"ts_dp" #N "_enable\">Enable</label>"\
                "<select id=\"ts_dp" #N "_enable\" name=\"enabled\">"\
                    "<option value=\"0\">Disabled</option>"\
                    "<option value=\"1\">Enabled</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"ts_dp" #N "_type\">Data Type</label>"\
                "<select id=\"ts_dp" #N "_type\" name=\"data_type\">"\
                    "<option value=\"0\">Coil (R/W, 1-bit)</option>"\
                    "<option value=\"1\">Discrete Input (RO, 1-bit)</option>"\
                    "<option value=\"2\">Input Register (RO, 16-bit)</option>"\
                    "<option value=\"3\">Holding Register (R/W, 16-bit)</option>"\
                "</select>"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"ts_dp" #N "_writable\">Allow Client Writes</label>"\
                "<select id=\"ts_dp" #N "_writable\" name=\"writable\">"\
                    "<option value=\"0\">No (read-only)</option>"\
                    "<option value=\"1\">Yes (read/write)</option>"\
                "</select>"\
            "</div>"\
            "<div>"\
                "<label for=\"ts_dp" #N "_encoding\">32-bit Encoding</label>"\
                "<select id=\"ts_dp" #N "_encoding\" name=\"encoding\">"\
                    "<option value=\"0\">ABCD (big-endian)</option>"\
                    "<option value=\"1\">BADC (word swap)</option>"\
                    "<option value=\"2\">CDAB (byte swap)</option>"\
                    "<option value=\"3\">DCBA (word+byte swap)</option>"\
                "</select>"\
            "</div>"\
        "</div>"\
        "<div class=\"row\">"\
            "<div>"\
                "<label for=\"ts_dp" #N "_addr\">Start Address</label>"\
                "<input id=\"ts_dp" #N "_addr\" name=\"start_address\" type=\"number\" min=\"0\" max=\"65535\" value=\"0\">"\
            "</div>"\
            "<div>"\
                "<label for=\"ts_dp" #N "_count\">Count (1-10)</label>"\
                "<input id=\"ts_dp" #N "_count\" name=\"count\" type=\"number\" min=\"1\" max=\"10\" value=\"1\">"\
            "</div>"\
        "</div>"\
        "<div style=\"margin-top: 15px; padding: 10px; background: #f5f5f5; border-radius: 4px;\">"\
            "<label style=\"font-weight: bold; display: block; margin-bottom: 8px;\">Tag Mapping (map registers/coils to tags):</label>"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp" #N "_tag0\">Reg/Coil 0:</label><select id=\"ts_dp" #N "_tag0\" name=\"tag0\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"ts_dp" #N "_tag1\">Reg/Coil 1:</label><select id=\"ts_dp" #N "_tag1\" name=\"tag1\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp" #N "_tag2\">Reg/Coil 2:</label><select id=\"ts_dp" #N "_tag2\" name=\"tag2\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"ts_dp" #N "_tag3\">Reg/Coil 3:</label><select id=\"ts_dp" #N "_tag3\" name=\"tag3\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp" #N "_tag4\">Reg/Coil 4:</label><select id=\"ts_dp" #N "_tag4\" name=\"tag4\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"ts_dp" #N "_tag5\">Reg/Coil 5:</label><select id=\"ts_dp" #N "_tag5\" name=\"tag5\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp" #N "_tag6\">Reg/Coil 6:</label><select id=\"ts_dp" #N "_tag6\" name=\"tag6\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"ts_dp" #N "_tag7\">Reg/Coil 7:</label><select id=\"ts_dp" #N "_tag7\" name=\"tag7\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
            "<div class=\"row\">"\
                "<div><label for=\"ts_dp" #N "_tag8\">Reg/Coil 8:</label><select id=\"ts_dp" #N "_tag8\" name=\"tag8\"><option value=\"255\">Not Mapped</option></select></div>"\
                "<div><label for=\"ts_dp" #N "_tag9\">Reg/Coil 9:</label><select id=\"ts_dp" #N "_tag9\" name=\"tag9\"><option value=\"255\">Not Mapped</option></select></div>"\
            "</div>"\
        "</div>"\
        "<button type=\"submit\">Save Memory Block " #N "</button>"\
    "</form>"

#endif /* _HTML_MODBUS_TCP_H_ */
