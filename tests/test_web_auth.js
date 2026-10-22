// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const vm = require('node:vm');
const context = {};
context.window = context;
vm.createContext(context);
vm.runInContext(fs.readFileSync('web/vendor/md5-2.19.0.min.js', 'utf8'), context);
vm.runInContext(fs.readFileSync('web/auth.js', 'utf8'), context);
const md5 = value => crypto.createHash('md5').update(value, 'utf8').digest('hex');
let checks = 0;
for (const value of ['', 'a', 'abc', 'message digest', '中文']) {
  assert.equal(context.md5(value), md5(value));
  checks++;
}
// Fixed local vectors verify the real client proof; no device or HTTP server is simulated.
// 固定本地向量检查真实客户端证明，不模拟设备或 HTTP 服务器。
for (const password of ['test-password', 'colon:password', '测试密码']) {
  const challenge = {realm:'Lamp Bridge', nonce:'local-test-nonce', opaque:'local-test-opaque'};
  const cnonce = '0123456789abcdef01234567';
  const header = context.LampAuth.authorization(challenge, 'admin', password, cnonce);
  const ha1 = md5('admin:' + challenge.realm + ':' + password);
  const ha2 = md5('POST:/api/session/login');
  const expected = md5(ha1 + ':' + challenge.nonce + ':00000001:' + cnonce + ':auth:' + ha2);
  assert.ok(header.startsWith('Digest username="admin"'));
  assert.ok(header.includes('uri="/api/session/login"'));
  assert.ok(header.includes('response="' + expected + '"'));
  assert.ok(!header.includes(password));
  checks++;
}
console.log('PASS: ' + checks + ' offline MD5/Digest checks; no hardware simulation');
