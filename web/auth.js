// SPDX-License-Identifier: MIT
'use strict';
window.LampAuth = {
  /**
   * Build a standard UTF-8 Digest proof without sending the password or HA1.
   * 生成标准 UTF-8 Digest 证明，不发送密码或 HA1。
   * @param {{realm: string, nonce: string, opaque: string}} challenge Server challenge.
   * 服务端签发的挑战。
   * @param {string} username Expected device username. / 设备预期账号。
   * @param {string} password Transient form value, never stored by this helper. / 临时表单密码，不保存。
   * @param {string} cnonce Fresh hexadecimal client nonce. / 新的十六进制客户端随机数。
   * @returns {string} Authorization header bound to POST /api/session/login, qop=auth.
   * 绑定 POST /api/session/login、qop=auth 的 Authorization 头。
   */
  authorization(challenge, username, password, cnonce) {
    const uri = '/api/session/login', nc = '00000001';
    const hash = window.md5;
    const ha1 = hash(username + ':' + challenge.realm + ':' + password);
    const ha2 = hash('POST:' + uri);
    const response = hash(ha1 + ':' + challenge.nonce + ':' + nc + ':' + cnonce + ':auth:' + ha2);
    const quote = value => JSON.stringify(value);
    return 'Digest username=' + quote(username) + ', realm=' + quote(challenge.realm) +
      ', nonce=' + quote(challenge.nonce) + ', uri=' + quote(uri) +
      ', response=' + quote(response) + ', opaque=' + quote(challenge.opaque) +
      ', qop=auth, nc=' + nc + ', cnonce=' + quote(cnonce);
  }
};
