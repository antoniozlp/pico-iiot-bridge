#ifndef _HTML_JS_H_
#define _HTML_JS_H_

#define HTML_JS \
    "<script>"\
    "var configLoaded = {network:false, serial:false, s2tcp:false, modbus:false, modbusServer:false, modbusTcpClient:false, modbusTcpServer:false, tagsForMapping:false, tagsForServerMapping:false, tagsForTcpMapping:false, tagsForTcpServerMapping:false};"\
    "var plotState = {selectedHandle:-1, values:[], labels:[], maxPoints:60};"\
    "function showPage(id) {"\
        "document.querySelectorAll('section').forEach(el => el.classList.remove('visible'));"\
        "document.getElementById(id).classList.add('visible');"\
        "document.querySelectorAll('.menu-item').forEach(el => el.classList.remove('active'));"\
        "event.target.classList.add('active');"\
        "if(id === 'network' && !configLoaded.network) { configLoaded.network = true; loadNetworkConfig(); }"\
        "if(id === 'serial' && !configLoaded.serial) { configLoaded.serial = true; loadSerialConfig(); }"\
        "if(id === 's2tcp' && !configLoaded.s2tcp) { configLoaded.s2tcp = true; loadS2tcpConfig(); }"\
        "if(id === 'modbus') {"\
            "if(!configLoaded.tagsForMapping) { configLoaded.tagsForMapping = true; loadTagsForMapping(); }"\
            "if(!configLoaded.modbus) { configLoaded.modbus = true; loadModbusConfig(); }"\
        "}"\
        "if(id === 'modbus_server') {"\
            "if(!configLoaded.tagsForServerMapping) { configLoaded.tagsForServerMapping = true; loadTagsForServerMapping(); }"\
            "if(!configLoaded.modbusServer) { configLoaded.modbusServer = true; loadModbusServerConfig(); }"\
        "}"\
        "if(id === 'modbus_tcp_client') {"\
            "if(!configLoaded.tagsForTcpMapping) { configLoaded.tagsForTcpMapping = true; loadTagsForTcpMapping(); }"\
            "if(!configLoaded.modbusTcpClient) { configLoaded.modbusTcpClient = true; loadModbusTcpClientConfig(); }"\
        "}"\
        "if(id === 'modbus_tcp_server') {"\
            "if(!configLoaded.tagsForTcpServerMapping) { configLoaded.tagsForTcpServerMapping = true; loadTagsForTcpServerMapping(); }"\
            "if(!configLoaded.modbusTcpServer) { configLoaded.modbusTcpServer = true; loadModbusTcpServerConfig(); }"\
        "}"\
        "if(id === 'tags') refreshTags();"\
        "if(id === 'plot') { loadPlotTagSelect(); drawPlotChart(); updatePlotStats(); }"\
    "}"\
    "function showSerial(id) {"\
        "document.getElementById('serial0-form').style.display = (id === 0) ? 'block' : 'none';"\
        "document.getElementById('serial1-form').style.display = (id === 1) ? 'block' : 'none';"\
        "document.getElementById('serial0-btn').classList.toggle('active', id === 0);"\
        "document.getElementById('serial1-btn').classList.toggle('active', id === 1);"\
    "}"\
    "function showRequest(idx) {"\
        "for(var i=0; i<10; i++){"\
            "document.getElementById('dp'+i+'-form').style.display = (i === idx) ? 'block' : 'none';"\
            "document.getElementById('dp'+i+'-btn').classList.toggle('active', i === idx);"\
        "}"\
    "}"\
    "function showServerDp(idx) {"\
        "for(var i=0; i<10; i++){"\
            "document.getElementById('sdp'+i+'-form').style.display = (i === idx) ? 'block' : 'none';"\
            "document.getElementById('sdp'+i+'-btn').classList.toggle('active', i === idx);"\
        "}"\
    "}"\
    "function showTcpRequest(idx) {"\
        "for(var i=0; i<10; i++){"\
            "document.getElementById('tc_dp'+i+'-form').style.display = (i === idx) ? 'block' : 'none';"\
            "document.getElementById('tc_dp'+i+'-btn').classList.toggle('active', i === idx);"\
        "}"\
    "}"\
    "function showTcpServerBlock(idx) {"\
        "for(var i=0; i<10; i++){"\
            "document.getElementById('ts_dp'+i+'-form').style.display = (i === idx) ? 'block' : 'none';"\
            "document.getElementById('ts_dp'+i+'-btn').classList.toggle('active', i === idx);"\
        "}"\
    "}"\
    "function toggleClientFields() {"\
        "var mode = document.getElementById('s2tcp_mode').value;"\
        "document.getElementById('client-fields').style.display = (mode == '1') ? 'block' : 'none';"\
    "}"\
    "function rebootToApply() {"\
        "if(!confirm('Reboot device now to apply configuration changes?')) return;"\
        "fetch('reboot.cgi', { method:'POST', body:new URLSearchParams() })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('Device rebooting...');"\
            "})"\
            ".catch(function(err){ alert('Reboot request sent. Device may already be restarting.'); });"\
    "}"\
    "function loadNetworkConfig() {"\
        "fetch('get_network.cgi').then(function(r){return r.json();}).then(function(net){"\
            "if(net.mac) document.getElementById('mac').value = net.mac;"\
            "if(net.ip) document.getElementById('ip').value = net.ip;"\
            "if(net.sn) document.getElementById('sn').value = net.sn;"\
            "if(net.gw) document.getElementById('gw').value = net.gw;"\
            "if(net.dns) document.getElementById('dns').value = net.dns;"\
            "if(net.dhcp !== undefined) document.getElementById('dhcp').value = net.dhcp;"\
        "}).catch(function(e){console.log('Network:',e);});"\
    "}"\
    "function loadSerialConfig() {"\
        "fetch('get_serial.cgi').then(function(r){return r.json();}).then(function(serial){"\
            "if(serial && serial.serial0){"\
                "if(serial.serial0.baud) document.getElementById('baud0').value = serial.serial0.baud;"\
                "if(serial.serial0.databits) document.getElementById('databits0').value = serial.serial0.databits;"\
                "if(serial.serial0.parity) document.getElementById('parity0').value = serial.serial0.parity;"\
                "if(serial.serial0.stopbits) document.getElementById('stopbits0').value = serial.serial0.stopbits;"\
                "if(serial.serial0.flowcts !== undefined) document.getElementById('flowcts0').checked = (serial.serial0.flowcts == 1);"\
                "if(serial.serial0.flowrts !== undefined) document.getElementById('flowrts0').checked = (serial.serial0.flowrts == 1);"\
            "}"\
            "if(serial && serial.serial1){"\
                "if(serial.serial1.baud) document.getElementById('baud1').value = serial.serial1.baud;"\
                "if(serial.serial1.databits) document.getElementById('databits1').value = serial.serial1.databits;"\
                "if(serial.serial1.parity) document.getElementById('parity1').value = serial.serial1.parity;"\
                "if(serial.serial1.stopbits) document.getElementById('stopbits1').value = serial.serial1.stopbits;"\
                "if(serial.serial1.flowcts !== undefined) document.getElementById('flowcts1').checked = (serial.serial1.flowcts == 1);"\
                "if(serial.serial1.flowrts !== undefined) document.getElementById('flowrts1').checked = (serial.serial1.flowrts == 1);"\
            "}"\
        "}).catch(function(e){console.log('Serial:',e);});"\
    "}"\
    "function loadS2tcpConfig() {"\
        "fetch('get_s2tcp.cgi').then(function(r){return r.json();}).then(function(s2tcp){"\
            "if(s2tcp.enable !== undefined) document.getElementById('s2tcp_enable').value = s2tcp.enable;"\
            "if(s2tcp.serial !== undefined) document.getElementById('s2tcp_serial').value = s2tcp.serial;"\
            "if(s2tcp.mode !== undefined) document.getElementById('s2tcp_mode').value = s2tcp.mode;"\
            "if(s2tcp.lport) document.getElementById('s2tcp_lport').value = s2tcp.lport;"\
            "if(s2tcp.timeout) document.getElementById('s2tcp_timeout').value = s2tcp.timeout;"\
            "if(s2tcp.keepalive) document.getElementById('s2tcp_keepalive').value = s2tcp.keepalive;"\
            "if(s2tcp.maxconn) document.getElementById('s2tcp_maxconn').value = s2tcp.maxconn;"\
            "if(s2tcp.remoteip) document.getElementById('s2tcp_remoteip').value = s2tcp.remoteip;"\
            "if(s2tcp.remoteport) document.getElementById('s2tcp_remoteport').value = s2tcp.remoteport;"\
            "toggleClientFields();"\
        "}).catch(function(e){console.log('S2TCP:',e);});"\
    "}"\
    "function loadModbusConfig() {"\
        "fetch('get_modbus.cgi').then(function(r){return r.json();}).then(function(modbus){"\
            "if(modbus.enable !== undefined) document.getElementById('mb_enable').value = modbus.enable;"\
            "if(modbus.serial_id !== undefined) document.getElementById('mb_serial').value = modbus.serial_id;"\
            "if(modbus.requests){"\
                "for(var i=0; i<modbus.requests.length && i<10; i++){"\
                    "var req = modbus.requests[i];"\
                    "if(req.enabled !== undefined) document.getElementById('dp'+i+'_enable').value = req.enabled;"\
                    "if(req.slave_address !== undefined) document.getElementById('dp'+i+'_slave').value = req.slave_address;"\
                    "if(req.data_type !== undefined) document.getElementById('dp'+i+'_type').value = req.data_type;"\
                    "if(req.operation !== undefined) document.getElementById('dp'+i+'_op').value = req.operation;"\
                    "if(req.start_address !== undefined) document.getElementById('dp'+i+'_addr').value = req.start_address;"\
                    "if(req.count !== undefined) document.getElementById('dp'+i+'_count').value = req.count;"\
                    "if(req.encoding !== undefined) document.getElementById('dp'+i+'_encoding').value = req.encoding;"\
                    "if(req.tag_handles){"\
                        "for(var j=0; j<10; j++){"\
                            "var tagSelect = document.getElementById('dp'+i+'_tag'+j);"\
                            "if(tagSelect && req.tag_handles[j] !== undefined) tagSelect.value = req.tag_handles[j];"\
                        "}"\
                    "}"\
                "}"\
            "}"\
        "}).catch(function(e){console.log('Modbus:',e);});"\
    "}"\
    "function submitForm(e, form) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "var action = (form.action || '').toString();"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('Settings saved successfully!');"\
                "if(action.indexOf('set_network') >= 0) loadNetworkConfig();"\
                "else if(action.indexOf('set_s2tcp') >= 0) loadS2tcpConfig();"\
                "else if(action.indexOf('set_modbus_tcp_client') >= 0) loadModbusTcpClientConfig();"\
                "else if(action.indexOf('set_modbus_tcp_server') >= 0) loadModbusTcpServerConfig();"\
                "else if(action.indexOf('set_modbus_client') >= 0) loadModbusConfig();"\
                "else if(action.indexOf('set_modbus_server') >= 0) loadModbusServerConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving settings: ' + err); });"\
    "}"\
    "function submitSerialForm(e, form, id) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('Serial ' + id + ' settings saved successfully!');"\
                "loadSerialConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving settings: ' + err); });"\
    "}"\
    "function submitRequestForm(e, form, idx) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('Request ' + idx + ' saved successfully!');"\
                "loadModbusConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving request: ' + err); });"\
    "}"\
    "function showCreateTagForm() {"\
        "document.getElementById('create-tag-form').style.display = 'block';"\
    "}"\
    "function hideCreateTagForm() {"\
        "document.getElementById('create-tag-form').style.display = 'none';"\
    "}"\
    "function submitCreateTag(e, form) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.json(); })"\
            ".then(function(data){"\
                "if(data.success){"\
                    "alert('Tag created successfully!');"\
                    "hideCreateTagForm();"\
                    "refreshTags();"\
                "}else{"\
                    "alert('Error: ' + (data.error || 'Failed to create tag'));"\
                "}"\
            "})"\
            ".catch(function(err){ alert('Error creating tag: ' + err); });"\
    "}"\
    "function deleteTag(tagName) {"\
        "if(!confirm('Delete tag \"' + tagName + '\"?')) return;"\
        "var params = new URLSearchParams();"\
        "params.append('tag_name', tagName);"\
        "fetch('delete_tag.cgi', { method:'POST', body:params })"\
            ".then(function(res){ return res.json(); })"\
            ".then(function(data){"\
                "if(data.success){"\
                    "alert('Tag deleted successfully!');"\
                    "refreshTags();"\
                "}else{"\
                    "alert('Error: ' + (data.error || 'Failed to delete tag'));"\
                "}"\
            "})"\
            ".catch(function(err){ alert('Error deleting tag: ' + err); });"\
    "}"\
    "function refreshTags() {"\
        "fetch('get_tags.cgi')"\
            ".then(function(res){ return res.json(); })"\
            ".then(function(data){"\
                "var tbody = document.getElementById('tags-tbody');"\
                "tbody.innerHTML = '';"\
                "if(data.tags && data.tags.length > 0){"\
                    "var countText = data.tags.length + ' tags';"\
                    "if(data.truncated && data.truncated > 0){"\
                        "countText += ' (⚠️ ' + data.truncated + ' more not shown)';"\
                    "}"\
                    "document.getElementById('tag-count').textContent = countText;"\
                    "data.tags.forEach(function(tag){"\
                        "var row = document.createElement('tr');"\
                        "row.style.borderBottom = '1px solid #ddd';"\
                        "var typeNames = ['BOOL','UINT8','UINT16','UINT32','INT16','INT32','FLOAT'];"\
                        "var qualityNames = ['GOOD','BAD','UNCERTAIN'];"\
                        "var value = tag.value;"\
                        "if(tag.type === 'float' || tag.type === 6){ value = parseFloat(value).toFixed(2); }"\
                        "var age = tag.age || 0;"\
                        "var ageStr = age < 60 ? age + 's ago' : (age < 3600 ? Math.floor(age/60) + 'm ago' : Math.floor(age/3600) + 'h ago');"\
                        "row.innerHTML = "\
                            "'<td style=\"padding:8px; border:1px solid #ddd;\">' + tag.handle + '</td>' +"\
                            "'<td style=\"padding:8px; border:1px solid #ddd; font-weight:bold;\">' + tag.name + '</td>' +"\
                            "'<td style=\"padding:8px; border:1px solid #ddd;\">' + typeNames[tag.type] + '</td>' +"\
                            "'<td style=\"padding:8px; border:1px solid #ddd;\">' + value + '</td>' +"\
                            "'<td style=\"padding:8px; border:1px solid #ddd;\">' + qualityNames[tag.quality] + '</td>' +"\
                            "'<td style=\"padding:8px; border:1px solid #ddd;\">' + ageStr + '</td>' +"\
                            "'<td style=\"padding:8px; border:1px solid #ddd; text-align:center;\">' +"\
                                "'<button onclick=\"deleteTag(\\'' + tag.name + '\\')\" style=\"padding:4px 8px; background:#f44336; color:white; border:none; border-radius:3px; cursor:pointer; font-size:12px;\">Delete</button>' +"\
                            "'</td>';"\
                        "tbody.appendChild(row);"\
                    "});"\
                    "if(data.truncated && data.truncated > 0){"\
                        "var warningRow = document.createElement('tr');"\
                        "warningRow.innerHTML = '<td colspan=\"7\" style=\"padding:10px; text-align:center; background:#fff3cd; color:#856404; border:1px solid #ffeaa7;\">⚠️ Warning: ' + data.truncated + ' tags not displayed due to buffer size limit</td>';"\
                        "tbody.appendChild(warningRow);"\
                    "}"\
                "}else{"\
                    "document.getElementById('tag-count').textContent = '0 tags';"\
                    "tbody.innerHTML = '<tr><td colspan=\"7\" style=\"padding:20px; text-align:center; color:#999;\">No tags configured</td></tr>';"\
                "}"\
                "collectPlotSample(data.tags || []);"\
            "})"\
            ".catch(function(err){ console.log('Error fetching tags: ', err); });"\
    "}"\
    "function collectPlotSample(tags) {"\
        "if(plotState.selectedHandle === -1) return;"\
        "var tag = null;"\
        "for(var i=0; i<tags.length; i++){ if(tags[i].handle === plotState.selectedHandle){ tag = tags[i]; break; } }"\
        "if(!tag){ plotState.selectedHandle = -1; return; }"\
        "plotState.values.push(Number(tag.value));"\
        "plotState.labels.push(new Date().toLocaleTimeString());"\
        "while(plotState.values.length > plotState.maxPoints){ plotState.values.shift(); plotState.labels.shift(); }"\
        "var plotSection = document.getElementById('plot');"\
        "if(plotSection && plotSection.classList.contains('visible')){"\
            "drawPlotChart();"\
            "updatePlotStats();"\
        "}"\
    "}"\
    "function loadPlotTagSelect() {"\
        "fetch('get_tags.cgi').then(function(r){return r.json();}).then(function(data){"\
            "var select = document.getElementById('plot-tag-select');"\
            "var prevValue = select.value;"\
            "select.innerHTML = '<option value=\"\">-- Select a tag --</option>';"\
            "if(data.tags && data.tags.length > 0){"\
                "var typeNames = ['BOOL','UINT8','UINT16','UINT32','INT16','INT32','FLOAT'];"\
                "data.tags.forEach(function(tag){"\
                    "var opt = document.createElement('option');"\
                    "opt.value = tag.handle;"\
                    "opt.textContent = tag.name + ' (' + (typeNames[tag.type] || '?') + ')';"\
                    "select.appendChild(opt);"\
                "});"\
                "if(prevValue) select.value = prevValue;"\
            "}"\
        "}).catch(function(e){ console.log('PlotTags:', e); });"\
    "}"\
    "function onPlotTagChange() {"\
        "var select = document.getElementById('plot-tag-select');"\
        "var handle = select.value === '' ? -1 : parseInt(select.value, 10);"\
        "plotState.selectedHandle = handle;"\
        "plotState.values = [];"\
        "plotState.labels = [];"\
        "var opt = select.options[select.selectedIndex];"\
        "document.getElementById('plot-tag-name').textContent = (handle === -1) ? 'No tag selected' : opt.textContent;"\
        "drawPlotChart();"\
        "updatePlotStats();"\
    "}"\
    "function onPlotWindowChange() {"\
        "var select = document.getElementById('plot-window-select');"\
        "plotState.maxPoints = parseInt(select.value, 10);"\
        "while(plotState.values.length > plotState.maxPoints){ plotState.values.shift(); plotState.labels.shift(); }"\
        "drawPlotChart();"\
        "updatePlotStats();"\
    "}"\
    "function clearPlotData() {"\
        "plotState.values = [];"\
        "plotState.labels = [];"\
        "drawPlotChart();"\
        "updatePlotStats();"\
    "}"\
    "function updatePlotStats() {"\
        "var values = plotState.values;"\
        "var cur = values.length ? values[values.length-1] : null;"\
        "var min = values.length ? Math.min.apply(null, values) : null;"\
        "var max = values.length ? Math.max.apply(null, values) : null;"\
        "var avg = values.length ? (values.reduce(function(a,b){return a+b;}, 0) / values.length) : null;"\
        "document.getElementById('plot-current').textContent = cur !== null ? cur.toFixed(2) : '--';"\
        "document.getElementById('plot-min').textContent = min !== null ? min.toFixed(2) : '--';"\
        "document.getElementById('plot-max').textContent = max !== null ? max.toFixed(2) : '--';"\
        "document.getElementById('plot-avg').textContent = avg !== null ? avg.toFixed(2) : '--';"\
        "document.getElementById('plot-samples').textContent = values.length;"\
    "}"\
    "function drawPlotChart() {"\
        "var canvas = document.getElementById('plot-canvas');"\
        "if(!canvas || !canvas.getContext) return;"\
        "var ctx = canvas.getContext('2d');"\
        "var w = canvas.width, h = canvas.height;"\
        "ctx.clearRect(0, 0, w, h);"\
        "var pad = {left:55, right:15, top:15, bottom:25};"\
        "var plotW = w - pad.left - pad.right;"\
        "var plotH = h - pad.top - pad.bottom;"\
        "ctx.strokeStyle = '#ccc';"\
        "ctx.strokeRect(pad.left, pad.top, plotW, plotH);"\
        "var values = plotState.values;"\
        "if(!values || values.length === 0){"\
            "ctx.fillStyle = '#999';"\
            "ctx.font = '13px sans-serif';"\
            "ctx.fillText(plotState.selectedHandle === -1 ? 'Select a tag to begin plotting' : 'Waiting for samples...', pad.left + 10, pad.top + plotH/2);"\
            "return;"\
        "}"\
        "var minV = Math.min.apply(null, values);"\
        "var maxV = Math.max.apply(null, values);"\
        "if(minV === maxV){ minV -= 1; maxV += 1; }"\
        "var range = maxV - minV;"\
        "var divisions = 5;"\
        "ctx.font = '11px sans-serif';"\
        "for(var i=0; i<=divisions; i++){"\
            "var y = pad.top + plotH - (i/divisions) * plotH;"\
            "var val = minV + (i/divisions) * range;"\
            "ctx.strokeStyle = '#eee';"\
            "ctx.beginPath();"\
            "ctx.moveTo(pad.left, y);"\
            "ctx.lineTo(pad.left + plotW, y);"\
            "ctx.stroke();"\
            "ctx.fillStyle = '#666';"\
            "ctx.fillText(val.toFixed(2), 2, y + 4);"\
        "}"\
        "var n = values.length;"\
        "ctx.strokeStyle = '#2196F3';"\
        "ctx.lineWidth = 2;"\
        "ctx.beginPath();"\
        "for(var i=0; i<n; i++){"\
            "var x = pad.left + (n === 1 ? plotW/2 : (i/(n-1)) * plotW);"\
            "var y = pad.top + plotH - ((values[i] - minV)/range) * plotH;"\
            "if(i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);"\
        "}"\
        "ctx.stroke();"\
        "ctx.lineWidth = 1;"\
        "ctx.fillStyle = '#2196F3';"\
        "for(var i=0; i<n; i++){"\
            "var x = pad.left + (n === 1 ? plotW/2 : (i/(n-1)) * plotW);"\
            "var y = pad.top + plotH - ((values[i] - minV)/range) * plotH;"\
            "ctx.beginPath();"\
            "ctx.arc(x, y, 2.5, 0, Math.PI*2);"\
            "ctx.fill();"\
        "}"\
    "}"\
    "function loadTagsForMapping() {"\
        "return fetch('get_tags.cgi')"\
            ".then(function(res){ return res.json(); })"\
            ".then(function(data){"\
                "if(data.tags && data.tags.length > 0){"\
                    "var typeNames = ['BOOL','UINT8','UINT16','UINT32','INT16','INT32','FLOAT'];"\
                    "for(var dpIdx = 0; dpIdx < 10; dpIdx++){"\
                        "for(var regIdx = 0; regIdx < 10; regIdx++){"\
                            "var select = document.getElementById('dp' + dpIdx + '_tag' + regIdx);"\
                            "if(select){"\
                                "select.innerHTML = '<option value=\"255\">Not Mapped</option>';"\
                                "data.tags.forEach(function(tag){"\
                                    "var opt = document.createElement('option');"\
                                    "opt.value = tag.handle;"\
                                    "opt.textContent = tag.name + ' (' + (typeNames[tag.type] || '?') + ')';"\
                                    "select.appendChild(opt);"\
                                "});"\
                            "}"\
                        "}"\
                    "}"\
                "}"\
                "return data;"\
            "})"\
            ".catch(function(err){ console.log('Error loading tags for mapping: ', err); return {}; });"\
    "}"\
    "function loadTagsForServerMapping() {"\
        "return fetch('get_tags.cgi')"\
            ".then(function(res){ return res.json(); })"\
            ".then(function(data){"\
                "if(data.tags && data.tags.length > 0){"\
                    "var typeNames = ['BOOL','UINT8','UINT16','UINT32','INT16','INT32','FLOAT'];"\
                    "for(var dpIdx = 0; dpIdx < 10; dpIdx++){"\
                        "for(var regIdx = 0; regIdx < 10; regIdx++){"\
                            "var select = document.getElementById('sdp' + dpIdx + '_tag' + regIdx);"\
                            "if(select){"\
                                "select.innerHTML = '<option value=\"255\">Not Mapped</option>';"\
                                "data.tags.forEach(function(tag){"\
                                    "var opt = document.createElement('option');"\
                                    "opt.value = tag.handle;"\
                                    "opt.textContent = tag.name + ' (' + (typeNames[tag.type] || '?') + ')';"\
                                    "select.appendChild(opt);"\
                                "});"\
                            "}"\
                        "}"\
                    "}"\
                "}"\
                "return data;"\
            "})"\
            ".catch(function(err){ console.log('Error loading tags for server mapping: ', err); return {}; });"\
    "}"\
    "function loadModbusServerConfig() {"\
        "fetch('get_modbus_server.cgi').then(function(r){return r.json();}).then(function(srv){"\
            "if(srv.enable !== undefined) document.getElementById('mbs_enable').value = srv.enable;"\
            "if(srv.serial_id !== undefined) document.getElementById('mbs_serial').value = srv.serial_id;"\
            "if(srv.server_address !== undefined) document.getElementById('mbs_address').value = srv.server_address;"\
            "if(srv.memory_blocks){"\
                "for(var i=0; i<srv.memory_blocks.length && i<10; i++){"\
                    "var dp = srv.memory_blocks[i];"\
                    "if(dp.enabled !== undefined) document.getElementById('sdp'+i+'_enable').value = dp.enabled;"\
                    "if(dp.data_type !== undefined) document.getElementById('sdp'+i+'_type').value = dp.data_type;"\
                    "if(dp.writable !== undefined) document.getElementById('sdp'+i+'_writable').value = dp.writable;"\
                    "if(dp.start_address !== undefined) document.getElementById('sdp'+i+'_addr').value = dp.start_address;"\
                    "if(dp.count !== undefined) document.getElementById('sdp'+i+'_count').value = dp.count;"\
                    "if(dp.encoding !== undefined) document.getElementById('sdp'+i+'_encoding').value = dp.encoding;"\
                    "if(dp.tag_handles){"\
                        "for(var j=0; j<10; j++){"\
                            "var tagSelect = document.getElementById('sdp'+i+'_tag'+j);"\
                            "if(tagSelect && dp.tag_handles[j] !== undefined) tagSelect.value = dp.tag_handles[j];"\
                        "}"\
                    "}"\
                "}"\
            "}"\
        "}).catch(function(e){console.log('ModbusServer:',e);});"\
    "}"\
    "function submitServerBlockForm(e, form, idx) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('Server memory block ' + idx + ' saved successfully!');"\
                "loadModbusServerConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving memory block: ' + err); });"\
    "}"\
    "function loadTagsForTcpMapping() {"\
        "return fetch('get_tags.cgi')"\
            ".then(function(res){ return res.json(); })"\
            ".then(function(data){"\
                "if(data.tags && data.tags.length > 0){"\
                    "var typeNames = ['BOOL','UINT8','UINT16','UINT32','INT16','INT32','FLOAT'];"\
                    "for(var dpIdx = 0; dpIdx < 10; dpIdx++){"\
                        "for(var regIdx = 0; regIdx < 10; regIdx++){"\
                            "var select = document.getElementById('tc_dp' + dpIdx + '_tag' + regIdx);"\
                            "if(select){"\
                                "select.innerHTML = '<option value=\"255\">Not Mapped</option>';"\
                                "data.tags.forEach(function(tag){"\
                                    "var opt = document.createElement('option');"\
                                    "opt.value = tag.handle;"\
                                    "opt.textContent = tag.name + ' (' + (typeNames[tag.type] || '?') + ')';"\
                                    "select.appendChild(opt);"\
                                "});"\
                            "}"\
                        "}"\
                    "}"\
                "}"\
                "return data;"\
            "})"\
            ".catch(function(err){ console.log('Error loading tags for TCP mapping: ', err); return {}; });"\
    "}"\
    "function loadTagsForTcpServerMapping() {"\
        "return fetch('get_tags.cgi')"\
            ".then(function(res){ return res.json(); })"\
            ".then(function(data){"\
                "if(data.tags && data.tags.length > 0){"\
                    "var typeNames = ['BOOL','UINT8','UINT16','UINT32','INT16','INT32','FLOAT'];"\
                    "for(var dpIdx = 0; dpIdx < 10; dpIdx++){"\
                        "for(var regIdx = 0; regIdx < 10; regIdx++){"\
                            "var select = document.getElementById('ts_dp' + dpIdx + '_tag' + regIdx);"\
                            "if(select){"\
                                "select.innerHTML = '<option value=\"255\">Not Mapped</option>';"\
                                "data.tags.forEach(function(tag){"\
                                    "var opt = document.createElement('option');"\
                                    "opt.value = tag.handle;"\
                                    "opt.textContent = tag.name + ' (' + (typeNames[tag.type] || '?') + ')';"\
                                    "select.appendChild(opt);"\
                                "});"\
                            "}"\
                        "}"\
                    "}"\
                "}"\
                "return data;"\
            "})"\
            ".catch(function(err){ console.log('Error loading tags for TCP server mapping: ', err); return {}; });"\
    "}"\
    "function loadModbusTcpClientConfig() {"\
        "fetch('get_modbus_tcp_client.cgi').then(function(r){return r.json();}).then(function(tcp){"\
            "if(tcp.enable !== undefined) document.getElementById('tc_enable').value = tcp.enable;"\
            "if(tcp.remote_ip !== undefined) document.getElementById('tc_remote_ip').value = tcp.remote_ip;"\
            "if(tcp.remote_port !== undefined) document.getElementById('tc_remote_port').value = tcp.remote_port;"\
            "if(tcp.requests){"\
                "for(var i=0; i<tcp.requests.length && i<10; i++){"\
                    "var req = tcp.requests[i];"\
                    "if(req.enabled !== undefined) document.getElementById('tc_dp'+i+'_enable').value = req.enabled;"\
                    "if(req.slave_address !== undefined) document.getElementById('tc_dp'+i+'_slave').value = req.slave_address;"\
                    "if(req.data_type !== undefined) document.getElementById('tc_dp'+i+'_type').value = req.data_type;"\
                    "if(req.operation !== undefined) document.getElementById('tc_dp'+i+'_op').value = req.operation;"\
                    "if(req.start_address !== undefined) document.getElementById('tc_dp'+i+'_addr').value = req.start_address;"\
                    "if(req.count !== undefined) document.getElementById('tc_dp'+i+'_count').value = req.count;"\
                    "if(req.encoding !== undefined) document.getElementById('tc_dp'+i+'_encoding').value = req.encoding;"\
                    "if(req.tag_handles){"\
                        "for(var j=0; j<10; j++){"\
                            "var tagSelect = document.getElementById('tc_dp'+i+'_tag'+j);"\
                            "if(tagSelect && req.tag_handles[j] !== undefined) tagSelect.value = req.tag_handles[j];"\
                        "}"\
                    "}"\
                "}"\
            "}"\
        "}).catch(function(e){console.log('ModbusTcpClient:',e);});"\
    "}"\
    "function loadModbusTcpServerConfig() {"\
        "fetch('get_modbus_tcp_server.cgi').then(function(r){return r.json();}).then(function(srv){"\
            "if(srv.enable !== undefined) document.getElementById('ts_enable').value = srv.enable;"\
            "if(srv.port !== undefined) document.getElementById('ts_port').value = srv.port;"\
            "if(srv.server_address !== undefined) document.getElementById('ts_address').value = srv.server_address;"\
            "if(srv.memory_blocks){"\
                "for(var i=0; i<srv.memory_blocks.length && i<10; i++){"\
                    "var dp = srv.memory_blocks[i];"\
                    "if(dp.enabled !== undefined) document.getElementById('ts_dp'+i+'_enable').value = dp.enabled;"\
                    "if(dp.data_type !== undefined) document.getElementById('ts_dp'+i+'_type').value = dp.data_type;"\
                    "if(dp.writable !== undefined) document.getElementById('ts_dp'+i+'_writable').value = dp.writable;"\
                    "if(dp.start_address !== undefined) document.getElementById('ts_dp'+i+'_addr').value = dp.start_address;"\
                    "if(dp.count !== undefined) document.getElementById('ts_dp'+i+'_count').value = dp.count;"\
                    "if(dp.encoding !== undefined) document.getElementById('ts_dp'+i+'_encoding').value = dp.encoding;"\
                    "if(dp.tag_handles){"\
                        "for(var j=0; j<10; j++){"\
                            "var tagSelect = document.getElementById('ts_dp'+i+'_tag'+j);"\
                            "if(tagSelect && dp.tag_handles[j] !== undefined) tagSelect.value = dp.tag_handles[j];"\
                        "}"\
                    "}"\
                "}"\
            "}"\
        "}).catch(function(e){console.log('ModbusTcpServer:',e);});"\
    "}"\
    "function submitTcpRequestForm(e, form, idx) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('TCP request ' + idx + ' saved successfully!');"\
                "loadModbusTcpClientConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving TCP request: ' + err); });"\
    "}"\
    "function submitTcpServerBlockForm(e, form, idx) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('TCP server memory block ' + idx + ' saved successfully!');"\
                "loadModbusTcpServerConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving TCP memory block: ' + err); });"\
    "}"\
    "document.addEventListener('DOMContentLoaded', function(){"\
        "if(window.location.protocol === 'file:'){"\
            "console.warn('Page loaded from file - fetch will fail. Access from device: http://<device-ip>/');"\
        "}"\
        "refreshTags();"\
        "setInterval(refreshTags, 2000);"\
    "});"\
    "</script>"

#endif // _HTML_JS_H_
