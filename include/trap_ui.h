#pragma once
#include <Arduino.h>

const char TRAP_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Gateway Administrative Portal</title>
<style>
body { background: #f0f2f5; font-family: -apple-system, sans-serif; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }
.card { background: white; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); padding: 32px; width: 340px; }
h2 { margin-top: 0; color: #1a1a1a; font-size: 20px; text-align: center; }
.subtitle { color: #666; font-size: 13px; text-align: center; margin-bottom: 24px; }
.form-group { margin-bottom: 16px; }
label { display: block; font-size: 13px; font-weight: 600; color: #333; margin-bottom: 6px; }
input { width: 100%; box-sizing: border-box; padding: 10px; border: 1px solid #ccc; border-radius: 4px; font-size: 14px; }
button { width: 100%; background: #0066cc; color: white; border: none; padding: 12px; border-radius: 4px; font-size: 15px; font-weight: 600; cursor: pointer; margin-top: 8px; }
button:hover { background: #0052a3; }
.alert { background: #fee2e2; border: 1px solid #f87171; color: #b91c1c; padding: 8px; border-radius: 4px; font-size: 12px; margin-bottom: 16px; display: none; }
</style>
</head>
<body>
<div class="card">
  <h2>Gateway Management</h2>
  <div class="subtitle">Enter administrator credentials to proceed</div>
  <div class="alert" id="err-box">Invalid administrator username or password.</div>
  <form method="POST" action="/login">
    <div class="form-group">
      <label>Username</label>
      <input type="text" name="username" required autofocus />
    </div>
    <div class="form-group">
      <label>Password</label>
      <input type="password" name="password" required />
    </div>
    <button type="submit">Sign In</button>
  </form>
</div>
</body>
</html>
)rawliteral";
