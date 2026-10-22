# SPDX-License-Identifier: MIT
"""Check multi-client Digest on a real ESP32 without RF control commands.
只在真实 ESP32 上检查多客户端 Digest；不发送射频控制命令。
"""
import argparse
from datetime import datetime
import json
from pathlib import Path
import requests
from requests.auth import HTTPDigestAuth

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url",required=True)
    parser.add_argument("--access-file",type=Path,required=True)
    args=parser.parse_args()
    base=args.url.rstrip("/")
    url=base+"/api/status"
    access=json.loads(args.access_file.read_text(encoding="utf-8"))["access"]
    first=requests.Session();first.trust_env=False
    first.auth=HTTPDigestAuth(access["username"],access["access_password"])
    other=requests.Session();other.trust_env=False
    value=first.get(url,timeout=5);value.raise_for_status()
    checks=[]
    for i in range(5):
        header=first.auth.build_digest_header("GET",url)
        challenge=other.get(url,timeout=5)
        accepted=other.get(url,headers={"Authorization":header},timeout=5)
        checks.append({"name":"client_survives_other_challenge_"+str(i),"passed":challenge.status_code==401 and accepted.status_code==200})
    header=first.auth.build_digest_header("GET",url)
    rejected=other.get(base+"/api/session/trust",headers={"Authorization":header},timeout=5)
    checks.append({"name":"digest_bound_to_request_uri","passed":rejected.status_code==401})
    auth=first.auth
    nonce=auth._thread_local.chal["nonce"]
    auth._thread_local.chal["nonce"]=nonce[:-1]+("0" if nonce[-1]!="0" else "1")
    forged=auth.build_digest_header("GET",url)
    rejected=other.get(url,headers={"Authorization":forged},timeout=5)
    checks.append({"name":"invalid_nonce_signature_rejected","passed":rejected.status_code==401})
    auth._thread_local.chal["nonce"]=nonce
    wrong=requests.Session();wrong.trust_env=False
    wrong.auth=HTTPDigestAuth(access["username"],"intentionally-incorrect-test-password")
    checks.append({"name":"wrong_password_rejected","passed":wrong.get(url,timeout=5).status_code==401})
    summary={"checks":checks,"all_passed":all(c["passed"] for c in checks),"radio_commands_sent":0}
    path=ROOT/"docs/evidence"/(datetime.now().strftime("%Y%m%d_%H%M%S")+"_digest-checks.json")
    path.write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(summary))
    if not summary["all_passed"]: raise SystemExit(1)

if __name__=="__main__":
    main()
