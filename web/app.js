// SPDX-License-Identifier: MIT
'use strict';
(() => {
  const buttons = Array.from(document.querySelectorAll('[data-command]'));
  const names = {power:'电源切换',brightness_up:'亮度增加',brightness_down:'亮度减少',cct_up:'偏冷',cct_down:'偏暖',night_mode:'夜灯模式',timer_10min:'10 分钟定时'};
  const byId = id => document.getElementById(id);
  let state = null, busy = false, blockedUntil = 0, polling = false, wifiBusy = false;
  let unlockTimer = 0, activeButton = '', trustBusy = false, loginBusy = false;
  let powerEffect = 'neutral';
  function setPowerEffect(effect) {
    powerEffect = effect;
    document.querySelector('.power-card').dataset.powerEffect = effect;
  }
  function showLogin(show) {
    byId('login-panel').hidden = !show;
    document.body.classList.toggle('login-mode', show);
    document.documentElement.classList.toggle('login-mode', show);
  }
  function updateTrust() {
    const trusted = !!(state && state.auth && state.auth.trusted_device);
    byId('trust-device').disabled = !state || trustBusy;
    byId('trust-device').classList.toggle('trusted', trusted);
    byId('trust-label').textContent = trusted ? '已信任 · 取消' : '信任此设备';
    byId('trust-device').setAttribute('aria-label', trusted ? '取消此设备的免登录信任' : '信任此设备，下次免登录');
  }
  function notice(text, error = false) { byId('notice').textContent = text; byId('notice').classList.toggle('error', error); }
  function updateButtons() {
    clearTimeout(unlockTimer);
    const remaining = Math.max(0, blockedUntil - Date.now());
    document.body.classList.toggle('rf-unavailable', !state || !state.rf.ready);
    buttons.forEach(button => {
      button.disabled = busy || !state || !state.rf.ready || remaining > 0;
      button.setAttribute('aria-busy', busy && button.dataset.command === activeButton ? 'true' : 'false');
    });
    if (remaining > 0) unlockTimer = setTimeout(updateButtons, remaining + 1);
  }
  async function request(path, options = {}) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 6000);
    try {
      const response = await fetch(path, {...options, headers:{'X-Lamp-Web':'1', ...(options.headers || {})}, credentials:'same-origin', cache:'no-store', signal:controller.signal});
      const data = await response.json();
      if (response.status === 401) { state = null; showLogin(true); }
      if (!response.ok) throw Object.assign(new Error(data.message || '请求未完成'), {data});
      return data;
    } finally { clearTimeout(timeout); }
  }
  async function refresh() {
    if (polling || busy) return;
    polling = true;
    try {
      const session = await request('/api/session');
      if (location.pathname === '/login' || !session.authenticated) {
        state = null; showLogin(true);
        byId('connection').textContent = '未登录'; byId('connection').className = 'badge';
        return;
      }
      state = session; showLogin(false);
      const badge = byId('connection');
      badge.textContent = state.rf.ready ? '网关在线' : '射频异常';
      badge.className = state.rf.ready ? 'badge online' : 'badge error';
      byId('radio-state').textContent = state.rf.ready ? '射频就绪' : '射频模块需要检查';
      byId('network-state').textContent = state.network.connected ? state.network.ip : '设备热点';
      byId('wifi-setup').hidden = !state.network.can_configure;
      document.body.classList.toggle('configuring', state.network.can_configure);
      document.documentElement.classList.toggle('configuring', state.network.can_configure);
      if (state.network.can_configure && state.network.connected) {
        byId('wifi-result').textContent = '已连接家庭 Wi-Fi。请将手机切回家庭 Wi-Fi，再打开下面的地址。设备热点将在 3 分钟后关闭。';
        const link = byId('home-link'); link.href = 'http://' + state.network.ip + '/'; link.textContent = link.href; link.hidden = false;
      } else if (state.network.can_configure && state.network.configured) {
        byId('wifi-result').textContent = '正在连接家庭 Wi-Fi；若一直未连接，请检查网络名称、密码及 2.4 GHz 支持。';
      }
      blockedUntil = Math.max(blockedUntil, Date.now() + state.cooldown_ms);
      if (byId('notice').textContent === '正在连接网关…' || byId('notice').textContent.startsWith('网关连接中断')) notice('准备就绪');
    } catch (error) {
      state = null;
      byId('connection').textContent = '未连接'; byId('connection').className = 'badge error';
      byId('radio-state').textContent = '等待网关恢复';
      notice('网关连接中断，请检查手机 Wi-Fi', true);
    } finally { polling = false; updateButtons(); updateTrust(); }
  }
  function requestId() {
    const values = new Uint32Array(3);
    crypto.getRandomValues(values);
    return Array.from(values, value => value.toString(16).padStart(8, '0')).join('');
  }
  async function send(button) {
    if (!state || !state.rf.ready || busy || Date.now() < blockedUntil) return;
    busy = true; activeButton = button; updateButtons(); notice('正在发送…');
    try {
      const data = await request('/api/command', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded','X-Lamp-CSRF':state.csrf},
        body:new URLSearchParams({button,request_id:requestId()}).toString()});
      blockedUntil = Date.now() + data.cooldown_ms;
      if (button === 'power') setPowerEffect(powerEffect === 'on' ? 'off' : 'on');
      notice('已发送 · ' + names[button]);
    } catch (error) {
      if (button === 'power') setPowerEffect('neutral');
      blockedUntil = Date.now() + (error.data && Number.isFinite(error.data.cooldown_ms) ? error.data.cooldown_ms : 2000);
      notice(error.data ? error.message : '结果未确认，请观察灯光后再操作', true);
      // Power toggles state: never retry a control command automatically after a network error.
      // 电源键具有切换语义：网络错误后不自动重试任何控制命令。
    } finally { busy = false; activeButton = ''; updateButtons(); }
  }
  buttons.forEach(button => button.addEventListener('click', () => send(button.dataset.command)));
  byId('wifi-form').addEventListener('submit', async event => {
    event.preventDefault();
    if (!state || wifiBusy) return;
    wifiBusy = true; byId('wifi-save').disabled = true;
    try {
      await request('/api/wifi', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded','X-Lamp-CSRF':state.csrf},
        body:new URLSearchParams({ssid:byId('ssid').value,password:byId('password').value}).toString()});
      byId('password').value = '';
      byId('wifi-result').textContent = '已保存，正在连接…';
      notice('连接信息已保存在设备');
    } catch (error) { notice(error.data ? error.message : '配网请求未完成，请检查连接', true); }
    finally { wifiBusy = false; byId('wifi-save').disabled = false; }
  });


  byId('login-form').addEventListener('submit', async event => {
    event.preventDefault();
    if (loginBusy) return;
    loginBusy = true; byId('login-submit').disabled = true;
    byId('login-result').textContent = '正在登录…';
    try {
      const challenge = await request('/api/login/challenge');
      const authorization = window.LampAuth.authorization(challenge, byId('login-username').value,
        byId('login-password').value, requestId());
      await request('/api/session/login', {method:'POST', headers:{
        'Content-Type':'application/x-www-form-urlencoded', 'X-Lamp-CSRF':challenge.csrf,
        'Authorization':authorization
      }, body:new URLSearchParams({remember:byId('login-remember').checked ? '1' : '0'}).toString()});
      const session = await request('/api/session');
      if (!session.authenticated) throw new Error('浏览器未保存登录信息，请检查隐私设置');
      // Navigate after login so password managers can recognize success; never store the password in app code.
      // 登录成功后导航，让系统密码管理器识别表单已完成；不自行保存密码。
      location.replace('/');
    } catch (error) { byId('login-result').textContent = error.message || '登录未完成，请重试'; }
    finally { loginBusy = false; byId('login-submit').disabled = false; }
  });
  byId('trust-device').addEventListener('click', async () => {
    if (!state || trustBusy) return;
    const wasTrusted = !!(state.auth && state.auth.trusted_device);
    trustBusy = true; updateTrust();
    try {
      await request(wasTrusted ? '/api/session/forget' : '/api/session/trust', {
        method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded','X-Lamp-CSRF':state.csrf}, body:''
      });
      // Read back to confirm the browser returns its Cookie; Set-Cookie alone does not prove persistence.
      // 重新读取，确认浏览器确实接收并回传 Cookie，不能仅凭 Set-Cookie 响应宣告记住成功。
      state = await request('/api/status');
      if (wasTrusted) notice('已取消信任；当前登录仍有效');
      else notice(state.auth.trusted_device ? '已信任此设备，下次可直接打开' : '浏览器未保存凭证，请检查隐私设置', !state.auth.trusted_device);
    } catch (error) { notice(error.data ? error.message : '信任设置未确认，请稍后重试', true); }
    finally { trustBusy = false; updateTrust(); }
  });
  // Other controllers may act while hidden; restore a neutral illustration when the page returns.
  // 离开页面后原遥控器或其他设备可能已操作，回来时恢复中性示意。
  document.addEventListener('visibilitychange', () => { if (!document.hidden) { setPowerEffect('neutral'); refresh(); } });
  document.addEventListener('keydown', () => document.body.classList.add('keyboard'));
  document.addEventListener('pointerdown', () => document.body.classList.remove('keyboard'), {passive:true});
  setInterval(refresh, 3000);
  refresh();
})();
