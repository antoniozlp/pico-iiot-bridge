#ifndef _HTML_JS_H_
#define _HTML_JS_H_

#define HTML_JS \
    "<script>"\
    "function showPage(id) {"\
        "document.querySelectorAll('section').forEach(el => el.classList.remove('visible'));"\
        "document.getElementById(id).classList.add('visible');"\
        "document.querySelectorAll('.menu-item').forEach(el => el.classList.remove('active'));"\
        "event.target.classList.add('active');"\
    "}"\
    "function showSerial(id) {"\
        "document.getElementById('serial0-form').style.display = (id === 0) ? 'block' : 'none';"\
        "document.getElementById('serial1-form').style.display = (id === 1) ? 'block' : 'none';"\
        "document.getElementById('serial0-btn').classList.toggle('active', id === 0);"\
        "document.getElementById('serial1-btn').classList.toggle('active', id === 1);"\
    "}"\
    "function showDataPoint(idx) {"\
        "for(var i=0; i<10; i++){"\
            "document.getElementById('dp'+i+'-form').style.display = (i === idx) ? 'block' : 'none';"\
            "document.getElementById('dp'+i+'-btn').classList.toggle('active', i === idx);"\
        "}"\
    "}"\
    "function toggleClientFields() {"\
        "var mode = document.getElementById('s2tcp_mode').value;"\
        "document.getElementById('client-fields').style.display = (mode == '1') ? 'block' : 'none';"\
    "}"\
    "function loadConfig() {"\
        "fetch('get_config.cgi')"\
            ".then(function(response) { return response.json(); })"\
            ".then(function(data) {"\
                "if(data.net) {"\
                    "if(data.net.mac) document.getElementById('mac').value = data.net.mac;"\
                    "if(data.net.ip) document.getElementById('ip').value = data.net.ip;"\
                    "if(data.net.sn) document.getElementById('sn').value = data.net.sn;"\
                    "if(data.net.gw) document.getElementById('gw').value = data.net.gw;"\
                    "if(data.net.dns) document.getElementById('dns').value = data.net.dns;"\
                    "if(data.net.dhcp !== undefined) document.getElementById('dhcp').value = data.net.dhcp;"\
                "}"\
                "if(data.serial0) {"\
                    "if(data.serial0.baud) document.getElementById('baud0').value = data.serial0.baud;"\
                    "if(data.serial0.databits) document.getElementById('databits0').value = data.serial0.databits;"\
                    "if(data.serial0.parity) document.getElementById('parity0').value = data.serial0.parity;"\
                    "if(data.serial0.stopbits) document.getElementById('stopbits0').value = data.serial0.stopbits;"\
                    "if(data.serial0.flowcts !== undefined) document.getElementById('flowcts0').checked = (data.serial0.flowcts == 1);"\
                    "if(data.serial0.flowrts !== undefined) document.getElementById('flowrts0').checked = (data.serial0.flowrts == 1);"\
                "}"\
                "if(data.serial1) {"\
                    "if(data.serial1.baud) document.getElementById('baud1').value = data.serial1.baud;"\
                    "if(data.serial1.databits) document.getElementById('databits1').value = data.serial1.databits;"\
                    "if(data.serial1.parity) document.getElementById('parity1').value = data.serial1.parity;"\
                    "if(data.serial1.stopbits) document.getElementById('stopbits1').value = data.serial1.stopbits;"\
                    "if(data.serial1.flowcts !== undefined) document.getElementById('flowcts1').checked = (data.serial1.flowcts == 1);"\
                    "if(data.serial1.flowrts !== undefined) document.getElementById('flowrts1').checked = (data.serial1.flowrts == 1);"\
                "}"\
                "if(data.s2tcp) {"\
                    "if(data.s2tcp.enable !== undefined) document.getElementById('s2tcp_enable').value = data.s2tcp.enable;"\
                    "if(data.s2tcp.serial !== undefined) document.getElementById('s2tcp_serial').value = data.s2tcp.serial;"\
                    "if(data.s2tcp.mode !== undefined) document.getElementById('s2tcp_mode').value = data.s2tcp.mode;"\
                    "if(data.s2tcp.lport) document.getElementById('s2tcp_lport').value = data.s2tcp.lport;"\
                    "if(data.s2tcp.timeout) document.getElementById('s2tcp_timeout').value = data.s2tcp.timeout;"\
                    "if(data.s2tcp.keepalive) document.getElementById('s2tcp_keepalive').value = data.s2tcp.keepalive;"\
                    "if(data.s2tcp.maxconn) document.getElementById('s2tcp_maxconn').value = data.s2tcp.maxconn;"\
                    "if(data.s2tcp.remoteip) document.getElementById('s2tcp_remoteip').value = data.s2tcp.remoteip;"\
                    "if(data.s2tcp.remoteport) document.getElementById('s2tcp_remoteport').value = data.s2tcp.remoteport;"\
                    "toggleClientFields();"\
                "}"\
                "if(data.modbus) {"\
                    "if(data.modbus.enable !== undefined) document.getElementById('mb_enable').value = data.modbus.enable;"\
                    "if(data.modbus.serial_id !== undefined) document.getElementById('mb_serial').value = data.modbus.serial_id;"\
                    "if(data.modbus.data_points) {"\
                        "for(var i=0; i<data.modbus.data_points.length && i<10; i++){"\
                            "var dp = data.modbus.data_points[i];"\
                            "if(dp.enabled !== undefined) document.getElementById('dp'+i+'_enable').value = dp.enabled;"\
                            "if(dp.slave_address !== undefined) document.getElementById('dp'+i+'_slave').value = dp.slave_address;"\
                            "if(dp.data_type !== undefined) document.getElementById('dp'+i+'_type').value = dp.data_type;"\
                            "if(dp.operation !== undefined) document.getElementById('dp'+i+'_op').value = dp.operation;"\
                            "if(dp.start_address !== undefined) document.getElementById('dp'+i+'_addr').value = dp.start_address;"\
                            "if(dp.count !== undefined) document.getElementById('dp'+i+'_count').value = dp.count;"\
                        "}"\
                    "}"\
                "}"\
            "})"\
            ".catch(function(err) { console.log('Error fetching config: ', err); });"\
    "}"\
    "function submitForm(e, form) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('Settings saved successfully!');"\
                "loadConfig();"\
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
                "loadConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving settings: ' + err); });"\
    "}"\
    "function submitDataPointForm(e, form, idx) {"\
        "e.preventDefault();"\
        "var formData = new FormData(form);"\
        "var params = new URLSearchParams();"\
        "for(var pair of formData.entries()){ params.append(pair[0], pair[1]); }"\
        "fetch(form.action, { method:'POST', body:params })"\
            ".then(function(res){ return res.text(); })"\
            ".then(function(text){"\
                "alert('Data Point ' + idx + ' saved successfully!');"\
                "loadConfig();"\
            "})"\
            ".catch(function(err){ alert('Error saving data point: ' + err); });"\
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
            "})"\
            ".catch(function(err){ console.log('Error fetching tags: ', err); });"\
    "}"\
    "document.addEventListener('DOMContentLoaded', function(){"\
        "loadConfig();"\
        "setInterval(refreshTags, 2000);"\
    "});"\
    "</script>"

#endif // _HTML_JS_H_
