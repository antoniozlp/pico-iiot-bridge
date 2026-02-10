#ifndef _HTML_TAGS_H_
#define _HTML_TAGS_H_

#define HTML_SECTION_TAGS \
    "<section id=\"tags\" class=\"section\">"\
        "<h1>Tag Database</h1>"\
        \
        "<div class=\"card\">"\
            "<h2>Tag Management</h2>"\
            "<button onclick=\"showCreateTagForm()\" style=\"margin-bottom:20px; padding:10px 20px; background:#4CAF50; color:white; border:none; border-radius:4px; cursor:pointer;\">Create New Tag</button>"\
            \
            "<div id=\"create-tag-form\" style=\"display:none; margin-bottom:20px; padding:15px; border:1px solid #ddd; border-radius:4px; background:#f9f9f9;\">"\
                "<h3>Create New Tag</h3>"\
                "<form onsubmit=\"submitCreateTag(event, this)\" action=\"create_tag.cgi\" method=\"POST\">"\
                    "<div style=\"margin-bottom:10px;\">"\
                        "<label style=\"display:inline-block; width:120px;\">Tag Name:</label>"\
                        "<input type=\"text\" name=\"tag_name\" maxlength=\"15\" placeholder=\"SENSOR_01\" required style=\"padding:5px;\">"\
                        "<span style=\"font-size:12px; color:#666; margin-left:10px;\">Max 15 characters</span>"\
                    "</div>"\
                    "<div style=\"margin-bottom:10px;\">"\
                        "<label style=\"display:inline-block; width:120px;\">Data Type:</label>"\
                        "<select name=\"data_type\" required style=\"padding:5px;\">"\
                            "<option value=\"0\">BOOL</option>"\
                            "<option value=\"1\">UINT8</option>"\
                            "<option value=\"2\" selected>UINT16</option>"\
                            "<option value=\"3\">UINT32</option>"\
                            "<option value=\"4\">INT16</option>"\
                            "<option value=\"5\">INT32</option>"\
                            "<option value=\"6\">FLOAT</option>"\
                        "</select>"\
                    "</div>"\
                    "<div>"\
                        "<button type=\"submit\" style=\"padding:8px 16px; background:#2196F3; color:white; border:none; border-radius:4px; cursor:pointer;\">Create Tag</button>"\
                        "<button type=\"button\" onclick=\"hideCreateTagForm()\" style=\"padding:8px 16px; background:#999; color:white; border:none; border-radius:4px; cursor:pointer; margin-left:10px;\">Cancel</button>"\
                    "</div>"\
                "</form>"\
            "</div>"\
        "</div>"\
        \
        "<div class=\"card\">"\
            "<h2>Active Tags</h2>"\
            "<div style=\"margin-bottom:10px;\">"\
                "<button onclick=\"refreshTags()\" style=\"padding:8px 16px; background:#2196F3; color:white; border:none; border-radius:4px; cursor:pointer;\">Refresh</button>"\
                "<span id=\"tag-count\" style=\"margin-left:20px; font-weight:bold;\">0 tags</span>"\
            "</div>"\
            "<div style=\"overflow-x:auto;\">"\
                "<table id=\"tags-table\" style=\"width:100%; border-collapse:collapse; margin-top:10px;\">"\
                    "<thead>"\
                        "<tr style=\"background:#f2f2f2; border-bottom:2px solid #ddd;\">"\
                            "<th style=\"padding:10px; text-align:left; border:1px solid #ddd;\">Handle</th>"\
                            "<th style=\"padding:10px; text-align:left; border:1px solid #ddd;\">Tag Name</th>"\
                            "<th style=\"padding:10px; text-align:left; border:1px solid #ddd;\">Type</th>"\
                            "<th style=\"padding:10px; text-align:left; border:1px solid #ddd;\">Value</th>"\
                            "<th style=\"padding:10px; text-align:left; border:1px solid #ddd;\">Quality</th>"\
                            "<th style=\"padding:10px; text-align:left; border:1px solid #ddd;\">Updated</th>"\
                            "<th style=\"padding:10px; text-align:center; border:1px solid #ddd;\">Actions</th>"\
                        "</tr>"\
                    "</thead>"\
                    "<tbody id=\"tags-tbody\">"\
                        "<tr><td colspan=\"7\" style=\"padding:20px; text-align:center; color:#999;\">Loading tags...</td></tr>"\
                    "</tbody>"\
                "</table>"\
            "</div>"\
        "</div>"\
    "</section>"

#endif // _HTML_TAGS_H_
