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
                "if(data.serial) {"\
                    "if(data.serial.baud) document.getElementById('baud').value = data.serial.baud;"\
                    "if(data.serial.databits) document.getElementById('databits').value = data.serial.databits;"\
                    "if(data.serial.parity) document.getElementById('parity').value = data.serial.parity;"\
                    "if(data.serial.stopbits) document.getElementById('stopbits').value = data.serial.stopbits;"\
                    "if(data.serial.flowcts !== undefined) document.getElementById('flowcts').checked = (data.serial.flowcts == 1);"\
                    "if(data.serial.flowrts !== undefined) document.getElementById('flowrts').checked = (data.serial.flowrts == 1);"\
                "}"\
                "if(data.tcp) {"\
                    "if(data.tcp.lport) document.getElementById('lport').value = data.tcp.lport;"\
                    "if(data.tcp.timeout) document.getElementById('timeout').value = data.tcp.timeout;"\
                    "if(data.tcp.keepalive) document.getElementById('keepalive').value = data.tcp.keepalive;"\
                    "if(data.tcp.maxconn) document.getElementById('maxconn').value = data.tcp.maxconn;"\
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
    "document.addEventListener('DOMContentLoaded', loadConfig);"\
    "</script>"

#endif // _HTML_JS_H_
