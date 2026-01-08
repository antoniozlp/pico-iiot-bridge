#ifndef _HTML_CSS_H_
#define _HTML_CSS_H_

#define HTML_CSS \
    "<style>"\
    "body{font-family:Arial,sans-serif;background:#f5f7fb;margin:0;padding:0;color:#222;display:flex;height:100vh;}"\
    ".sidebar{width:250px;background:#fff;border-right:1px solid #dce1eb;padding:24px;box-sizing:border-box;display:flex;flex-direction:column;}"\
    ".sidebar h2{margin:0 0 24px;font-size:20px;}"\
    ".menu-item{padding:12px;cursor:pointer;border-radius:6px;color:#555;margin-bottom:4px;}"\
    ".menu-item:hover{background:#f0f2f5;}"\
    ".menu-item.active{background:#e6f0ff;color:#1d78ff;font-weight:600;}"\
    ".content{flex:1;padding:32px;overflow-y:auto;}"\
    "h1{margin:0 0 8px;font-size:24px;}"\
    ".desc{color:#666;margin-bottom:24px;}"\
    "section{background:#fff;border:1px solid #dce1eb;border-radius:8px;padding:24px;margin-bottom:24px;display:none;}"\
    "section.visible{display:block;}"\
    "label{display:block;font-weight:600;margin:16px 0 6px;}"\
    "input,select{width:100%;padding:10px;border:1px solid #cbd3e3;border-radius:6px;box-sizing:border-box;font-size:14px;}"\
    ".row{display:flex;gap:16px;}"\
    ".row>div{flex:1;}"\
    "button{margin-top:24px;background:#1d78ff;color:#fff;border:none;border-radius:6px;padding:12px 20px;font-weight:600;cursor:pointer;font-size:14px;}"\
    "button:hover{background:#0f63d6;}"\
    ".note{font-size:12px;color:#888;margin-top:12px;}"\
    "</style>"

#endif // _HTML_CSS_H_
