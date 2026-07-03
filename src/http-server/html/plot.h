#ifndef _HTML_PLOT_H_
#define _HTML_PLOT_H_

#define HTML_SECTION_PLOT \
    "<section id=\"plot\" class=\"section\">"\
        "<h1>Data Plot</h1>"\
        \
        "<div class=\"card\">"\
            "<h2>Tag Selection</h2>"\
            "<div style=\"margin-bottom:10px;\">"\
                "<label style=\"display:inline-block; width:120px;\">Tag:</label>"\
                "<select id=\"plot-tag-select\" onchange=\"onPlotTagChange()\" style=\"padding:5px; min-width:220px;\">"\
                    "<option value=\"\">-- Select a tag --</option>"\
                "</select>"\
            "</div>"\
            "<div style=\"margin-bottom:10px;\">"\
                "<label style=\"display:inline-block; width:120px;\">Window:</label>"\
                "<select id=\"plot-window-select\" onchange=\"onPlotWindowChange()\" style=\"padding:5px;\">"\
                    "<option value=\"30\">1 minute (30 samples)</option>"\
                    "<option value=\"60\" selected>2 minutes (60 samples)</option>"\
                    "<option value=\"150\">5 minutes (150 samples)</option>"\
                    "<option value=\"300\">10 minutes (300 samples)</option>"\
                "</select>"\
            "</div>"\
            "<button onclick=\"clearPlotData()\" style=\"padding:8px 16px; background:#999; color:white; border:none; border-radius:4px; cursor:pointer;\">Clear</button>"\
        "</div>"\
        \
        "<div class=\"card\">"\
            "<h2 id=\"plot-tag-name\">No tag selected</h2>"\
            "<div style=\"display:flex; gap:30px; margin-bottom:15px; flex-wrap:wrap;\">"\
                "<div><span style=\"color:#666;\">Current:</span> <strong id=\"plot-current\">--</strong></div>"\
                "<div><span style=\"color:#666;\">Min:</span> <strong id=\"plot-min\">--</strong></div>"\
                "<div><span style=\"color:#666;\">Max:</span> <strong id=\"plot-max\">--</strong></div>"\
                "<div><span style=\"color:#666;\">Avg:</span> <strong id=\"plot-avg\">--</strong></div>"\
                "<div><span style=\"color:#666;\">Samples:</span> <strong id=\"plot-samples\">0</strong></div>"\
            "</div>"\
            "<canvas id=\"plot-canvas\" width=\"900\" height=\"350\" style=\"width:100%; max-width:900px; border:1px solid #ddd; display:block;\"></canvas>"\
        "</div>"\
    "</section>"

#endif // _HTML_PLOT_H_
