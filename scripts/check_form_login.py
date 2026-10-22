# SPDX-License-Identifier: MIT
"""Verify form login using a real ESP32 and the actual JavaScript Digest function; no RF transmission.
用真实 ESP32 及页面的 JavaScript Digest 函数验证标准表单登录；不发送射频。
"""
import argparse
from datetime import datetime
import json
from pathlib import Path
import subprocess
import requests
from requests.auth import HTTPDigestAuth

ROOT = Path(__file__).resolve().parent.parent
NODE = r"""
const fs=require('fs'),vm=require('vm');
const ctx={};ctx.window=ctx;vm.createContext(ctx);
vm.runInContext(fs.readFileSync('web/vendor/md5-2.19.0.min.js','utf8'),ctx);
vm.runInContext(fs.readFileSync('web/auth.js','utf8'),ctx);
if(ctx.md5('abc')!=='900150983cd24fb0d6963f7d28e17f72')throw Error('MD5 vector');
const x=JSON.parse(fs.readFileSync(0,'utf8'));
process.stdout.write(ctx.LampAuth.authorization(x.challenge,x.username,x.password,'0123456789abcdef01234567'));
"""

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url",required=True)
    parser.add_argument("--access-file",type=Path,required=True)
    parser.add_argument("--resume",action="store_true")
    args=parser.parse_args()
    base=args.url.rstrip("/")
    access=json.loads(args.access_file.read_text(encoding="utf-8"))["access"]
    checks=[]
    def check(name,passed):
        checks.append({"name":name,"passed":bool(passed)})
        if not passed:raise RuntimeError("检查失败："+name)
    def client():
        s=requests.Session();s.trust_env=False;return s
    def proof(challenge,password=None):
        data={"challenge":challenge,"username":access["username"],"password":password or access["access_password"]}
        return subprocess.check_output(["node","-e",NODE],input=json.dumps(data),text=True,cwd=ROOT).strip()
    def headers(challenge,password=None):
        return {"Content-Type":"application/x-www-form-urlencoded","X-Lamp-CSRF":challenge["csrf"],"Authorization":proof(challenge,password)}
    def challenge(s):
        r=s.get(base+"/api/login/challenge",timeout=5);r.raise_for_status();return r.json()
    def login(s,remember):
        ch=challenge(s);h=headers(ch)
        r=s.post(base+"/api/session/login",data={"remember":str(int(remember))},headers=h,timeout=5)
        check("login_remember_"+str(int(remember)),r.status_code==200)
        check("cookie_httponly_samesite",all(x in r.headers.get("Set-Cookie","") for x in ("HttpOnly","SameSite=Strict","Path=/")))
        check("cookie_lifetime_matches_choice",("Max-Age=31536000" in r.headers.get("Set-Cookie",""))==remember)
        replay=client().post(base+"/api/session/login",data={"remember":str(int(remember))},headers=h,timeout=5)
        check("challenge_is_single_use",replay.status_code==403 and "Set-Cookie" not in replay.headers)
        state=s.get(base+"/api/session",timeout=5).json()
        check("cookie_access_and_trust_status",state["authenticated"] and state["auth"]["trusted_device"]==remember)
        return state
    admin=client();admin.auth=HTTPDigestAuth(access["username"],access["access_password"])
    before=admin.get(base+"/api/status",timeout=5).json()
    a,b=client(),client()
    if args.resume:
        saved=json.loads((ROOT/"build/private/form-session-test.json").read_text(encoding="utf-8"))
        check("memory_session_does_not_survive_reboot",a.get(base+"/api/status",headers={"Cookie":"lamp_device="+saved["session"]},timeout=5).status_code==401)
        a.headers["Cookie"]="lamp_device="+saved["trusted"]
        state=a.get(base+"/api/session",timeout=5).json()
        check("remembered_form_login_survives_reboot",state["authenticated"] and state["auth"]["trusted_device"])
        r=a.post(base+"/api/session/forget",data="",headers={"Content-Type":"application/x-www-form-urlencoded","X-Lamp-CSRF":state["csrf"]},timeout=5)
        check("cleanup_remembered_form_token",r.status_code==200)
        a.headers.pop("Cookie",None)
        current=a.get(base+"/api/session",timeout=5).json()
        check("forget_keeps_current_memory_session",current["authenticated"] and not current["auth"]["trusted_device"])
        check("revoked_persistent_token_rejected",client().get(base+"/api/status",headers={"Cookie":"lamp_device="+saved["trusted"]},timeout=5).status_code==401)
    else:
        r=a.get(base+"/",timeout=5)
        check("public_login_shell_no_system_dialog",r.status_code==200 and "WWW-Authenticate" not in r.headers)
        check("ios_autofill_semantics",'autocomplete="username"' in r.text and 'autocomplete="current-password"' in r.text and 'id="login-form"' in r.text)
        r=a.get(base+"/api/session",timeout=5)
        check("unauthenticated_probe_no_system_dialog",r.status_code==200 and not r.json()["authenticated"] and "WWW-Authenticate" not in r.headers)
        ch=challenge(a)
        bad=a.post(base+"/api/session/login",data={"remember":"1"},headers=headers(ch,"intentionally-wrong"),timeout=5)
        check("wrong_password_inline_error_not_native_dialog",bad.status_code==403 and "WWW-Authenticate" not in bad.headers and "Set-Cookie" not in bad.headers)
        ch=challenge(a);h=headers(ch)
        bad=a.post(base+"/api/session/login",data={"remember":"1"},headers={**h,"X-Lamp-CSRF":"bad"},timeout=5)
        check("login_csrf_required",bad.status_code==403)
        bad=a.post(base+"/api/session/login",data={"remember":"1"},headers={**h,"Origin":"http://other.invalid"},timeout=5)
        check("cross_origin_login_rejected",bad.status_code==403)
        bad=a.post(base+"/api/session/login",json={"password":"not-a-real-password"},headers=h,timeout=5)
        check("plain_password_not_accepted",bad.status_code==403)
        session=login(a,False)
        r=a.post(base+"/api/command",data={"button":"unknown","request_id":"0123456789abcdef01234567"},headers={"X-Lamp-CSRF":session["csrf"]},timeout=5)
        check("session_api_still_whitelists_buttons",r.status_code==400)
        remembered=login(b,True)
        old_token=b.cookies.get("lamp_device")
        remembered=login(b,True)
        check("relogin_rotates_old_trust_token",client().get(base+"/api/status",headers={"Cookie":"lamp_device="+old_token},timeout=5).status_code==401)
        check("explicit_login_page_available",b.get(base+"/login",timeout=5).status_code==200)
        check("session_and_trusted_tokens_distinct",a.cookies.get("lamp_device")!=b.cookies.get("lamp_device"))
        (ROOT/"build/private/form-session-test.json").write_text(json.dumps({"session":a.cookies.get("lamp_device"),"trusted":b.cookies.get("lamp_device")})+"\n",encoding="utf-8")
    after=admin.get(base+"/api/status",timeout=5).json()
    check("no_rf_transmission",before["attempts"]==after["attempts"])
    name="form-login-reboot" if args.resume else "form-login"
    output=ROOT/"docs/evidence"/(datetime.now().strftime("%Y%m%d_%H%M%S")+"_"+name+"-checks.json")
    output.write_text(json.dumps({"checks":checks,"before_attempts":before["attempts"],"after_attempts":after["attempts"],"all_passed":True},ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    print(f"PASS: {len(checks)} real {name} checks -> {output}")

if __name__=="__main__":main()
